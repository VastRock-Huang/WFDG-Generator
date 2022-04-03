//
// Created by Unravel on 2022/3/14.
//


#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfg {

    vector<pair<unsigned, unsigned>>
    FuncInfoGenConsumer::_findSensitiveLines(const FunctionDecl* functionDecl,
                                             const pair<unsigned, unsigned> &lineRange) const {
        unsigned sensitiveLine = _config.getSensitiveLine();
        if (sensitiveLine != 0 && Util::numInRange(sensitiveLine, lineRange) == 0) {
            return {{sensitiveLine, 0}};
        }
        const SourceLocation &beginLoc = functionDecl->getLocation();
        const SourceLocation &endLoc = functionDecl->getEndLoc();

        FileID fileId = _manager.getMainFileID();
        StringRef funcContent{_manager.getCharacterData(beginLoc),
                              _manager.getCharacterData(endLoc) - _manager.getCharacterData(beginLoc) + 1UL};
        unsigned fileOffset = _manager.getFileOffset(beginLoc);
        vector<pair<unsigned, unsigned>> result;
        for (size_t i = 1; i < _config.keyWords.size(); ++i) {
            StringRef keyword{_config.keyWords[i]};
            size_t pos = 0;
            pos = funcContent.find(keyword, pos);
            while (pos != StringRef::npos) {
                unsigned line = _manager.getLineNumber(fileId, fileOffset + pos);
                result.emplace_back(line, i);
                pos += keyword.size();
                pos = funcContent.find(keyword, pos);
            }
        }
        return result;
    }

    void FuncInfoGenConsumer::_traverseCFGStmt(const Stmt *stmt, CustomCPG::CPGNode &node) const {
        assert(stmt);
        node.lineRanges.push_back(move(_getLineRange(stmt->getBeginLoc(), stmt->getEndLoc())));
        node.updateStmtVec(stmt->getStmtClassName());
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _traverseCFGStmt(*it, node);
        }
    }

    void FuncInfoGenConsumer::_catchSpecialStmt(const Stmt *stmt, CustomCPG::CPGNode &node) const {
        if (stmt) {
            node.lineRanges.push_back(move(_getLineRange(stmt->getBeginLoc(), stmt->getEndLoc())));
            node.updateStmtVec(stmt->getStmtClassName());
        }
    }

    pair<unsigned, unsigned> FuncInfoGenConsumer::_getLineRange(const SourceLocation &beginLoc,const SourceLocation &endLoc) const {
        unsigned startLine = _context.getFullLoc(beginLoc).getSpellingLineNumber();
        unsigned endLine = _context.getFullLoc(endLoc).getSpellingLineNumber();
        if (startLine <= endLine) {
            return make_pair(startLine, endLine);
        }
        return make_pair(endLine, startLine);
    }


    void FuncInfoGenConsumer::_buildCustomCPG(const unique_ptr<CFG>& wholeCFG, CustomCPG& customCPG) const {
        customCPG.initNodeCnt(wholeCFG->size());
        for (auto &block: *wholeCFG) {
            unsigned cur = block->getBlockID();

            for (auto it = block->succ_begin(); it != block->succ_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                customCPG.addSuccEdge(cur, b->getBlockID());
            }
            customCPG.finishSuccEdges();

            for (auto it = block->pred_begin(); it != block->pred_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                customCPG.addPredEdge(cur, b->getBlockID());
            }
            customCPG.finishPredEdges();

            CustomCPG::CPGNode& node = customCPG.getNode(cur);
            for (const CFGElement &element: *block) {
                if (Optional<CFGStmt> cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    assert(stmt);
                    _traverseCFGStmt(stmt, node);
                }
            }
            _catchSpecialStmt(block->getTerminatorStmt(), node);
            _catchSpecialStmt(block->getLoopTarget(), node);
            _catchSpecialStmt(block->getLabel(), node);
            node.mergeLineRanges();
        }
    }



    bool FuncInfoGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getLocation()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                pair<unsigned, unsigned> lineRange = _getLineRange(funcDecl->getLocation(), funcDecl->getEndLoc());

                FuncInfo funcInfo(funcDecl->getQualifiedNameAsString(), lineRange, _config.ASTStmtKindMap);
                funcInfo.setSensitiveLines(move(_findSensitiveLines(funcDecl, lineRange)));

                Stmt *funcBody = funcDecl->getBody();
                unique_ptr<CFG> wholeCFG = CFG::buildCFG(funcDecl, funcBody, &_context, CFG::BuildOptions());
                _buildCustomCPG(wholeCFG, funcInfo.getCustomCPG());
                _funcInfoList.push_back(funcInfo);
            }
        }
        return true;
    }

    void FuncInfoGenAction::_lexToken() const {
        Preprocessor &preprocessor = getCompilerInstance().getPreprocessor();
        Token token;
        preprocessor.EnterMainSourceFile();
        size_t funcCnt = _funcInfoList.size();
        size_t i = 0;
        string preId{};
        unsigned preLine{0};
        do {
            preprocessor.Lex(token);
            if ((token.is(tok::arrow) || token.is(tok::period)) && preLine != 0) {
                preId.append(preprocessor.getSpelling(token));
            } else if (token.isAnyIdentifier()) {
                string idName = preprocessor.getSpelling(token);
                if (preId.empty() && _varDeclSet.count(idName) == 0) {
                    continue;
                }
                preId.append(idName);
                unsigned lineNo = getCompilerInstance().getASTContext()
                        .getFullLoc(token.getLocation()).getSpellingLineNumber();
                while (i < funcCnt) {
                    int ret = Util::numInRange(lineNo, _funcInfoList[i].getLineRange());
                    if (ret == 0) {
                        preLine = lineNo;
                        break;
                    } else if (ret < 0) {
                        preLine = 0;
                        break;
                    }
                    ++i;
                }
            } else if(preLine != 0) {
                _funcInfoList[i].insertIdentifier(preId, preLine);
                preId.clear();
                preLine = 0;
            } else {
                preId.clear();
                preLine = 0;
            }
        } while (token.isNot(tok::eof));
    }

}