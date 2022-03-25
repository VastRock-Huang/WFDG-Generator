//
// Created by Unravel on 2022/3/14.
//


#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfg {

    vector<pair<unsigned, unsigned>>
    FuncInfoGenConsumer::_findSensitiveLines(const SourceLocation &beginLoc, const SourceLocation &endLoc) const {
        if (_config.hasSensitiveLine) {
            return {{_config.sensitiveLine, 0}};
        }

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

    void FuncInfoGenConsumer::_travelCFGStmt(const Stmt *stmt, CFGNode &node) const {
        assert(stmt);
//        llvm::errs() << stmt->getStmtClassName() <<'\n';
        node.lineRanges.push_back(move(_getStmtLineRange(stmt->getSourceRange())));
        _config.updateStmtVec(node.stmtVec, stmt->getStmtClassName());
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _travelCFGStmt(*it, node);
        }
    }

    void FuncInfoGenConsumer::_catchSpecialStmt(const Stmt *stmt, CFGNode &node) const {
        if (stmt) {
//            llvm::errs() <<"**"<< stmt->getStmtClassName() <<'\n';
            node.lineRanges.push_back(move(_getStmtLineRange(stmt->getSourceRange())));
            _config.updateStmtVec(node.stmtVec, stmt->getStmtClassName());
        }
    }

    pair<unsigned, unsigned> FuncInfoGenConsumer::_getStmtLineRange(const SourceRange &sourceRange) const {
        unsigned startLine = _context.getFullLoc(sourceRange.getBegin()).getSpellingLineNumber();
        unsigned endLine = _context.getFullLoc(sourceRange.getEnd()).getSpellingLineNumber();
        if (startLine <= endLine) {
            return make_pair(startLine, endLine);
        }
        return make_pair(endLine, startLine);
    }

    MiniCFG FuncInfoGenConsumer::_buildMiniCFG(const FunctionDecl *funcDecl) const {
        Stmt *funcBody = funcDecl->getBody();
        unique_ptr <CFG> wholeCFG = CFG::buildCFG(funcDecl, funcBody, &_context, CFG::BuildOptions());
        MiniCFG miniCFG(funcDecl->getQualifiedNameAsString(), wholeCFG->size(),
                        _config.ASTStmtKindMap);

        for (auto &block: *wholeCFG) {
            unsigned cur = block->getBlockID();

            for (auto it = block->succ_begin(); it != block->succ_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                miniCFG.addSuccEdge(cur, b->getBlockID());
            }
            miniCFG.finishSuccEdges();

            for (auto it = block->pred_begin(); it != block->pred_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                miniCFG.addPredEdge(cur, b->getBlockID());
            }
            miniCFG.finishPredEdges();

            CFGNode node(vector<unsigned>(_config.ASTStmtKindMap.size()));
            for (const CFGElement &element: *block) {
                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    assert(stmt);
                    _travelCFGStmt(stmt, node);
                }
            }
            _catchSpecialStmt(block->getTerminatorStmt(), node);
            _catchSpecialStmt(block->getLoopTarget(), node);
            _catchSpecialStmt(block->getLabel(), node);
            Util::mergeLineRanges(node.lineRanges);

            miniCFG.setCFGNode(cur, node);
        }
        return miniCFG;
    }

    bool FuncInfoGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getSourceRange().getBegin()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                FullSourceLoc beginLoc = _context.getFullLoc(funcDecl->getSourceRange().getBegin());
                FullSourceLoc endLoc = _context.getFullLoc(funcDecl->getSourceRange().getEnd());

                FuncInfo funcInfo(funcDecl->getQualifiedNameAsString(), beginLoc.getSpellingLineNumber(),
                                  endLoc.getSpellingLineNumber(), move(_buildMiniCFG(funcDecl)));
                funcInfo.setSensitiveLines(move(_findSensitiveLines(beginLoc, endLoc)));
                GlobalInstance::getInstance().FuncInfoList.push_back(funcInfo);
            }
        }
        return true;
    }

    void FuncInfoGenAction::_lexToken() const {
        Preprocessor &preprocessor = getCompilerInstance().getPreprocessor();
        // TODO: 处理结构体的变量,带->和.的标识符
        Token token;
        preprocessor.EnterMainSourceFile();
        vector<FuncInfo> &funInfoList = GlobalInstance::getInstance().FuncInfoList;
        size_t funcCnt = funInfoList.size();
        size_t i = 0;
        do {
            preprocessor.Lex(token);
            if (token.isAnyIdentifier()) {
                string idName = preprocessor.getSpelling(token);
                if (_varDeclSet.count(idName) == 0) {
                    continue;
                }

                unsigned lineNo = getCompilerInstance().getASTContext()
                        .getFullLoc(token.getLocation()).getSpellingLineNumber();
                while (i < funcCnt) {
                    int ret = Util::numInRange(lineNo, funInfoList[i].getLineRange());
                    if (ret == 0) {
                        funInfoList[i].insertIdentifier(idName, lineNo);
                        break;
                    } else if (ret < 0) {
                        break;
                    }
                    ++i;
                }
            }
        } while (token.isNot(tok::eof));
    }

}