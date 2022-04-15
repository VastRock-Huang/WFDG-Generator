//
// Created by Unravel on 2022/3/14.
//


#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include "SimplifiedDepnHelper.h"
#include "DetailedDepnHelper.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfdg {

    map<unsigned, int> FuncInfoGenConsumer::_findSensitiveLines(const FunctionDecl *functionDecl,
                                                                const pair<unsigned, unsigned> &lineRange) const {
        unsigned sensitiveLine = _config.getSensitiveLine();
        if (sensitiveLine != 0 && util::numInRange(sensitiveLine, lineRange) == 0) {
            return {{sensitiveLine, 0}};
        }
        const SourceLocation &beginLoc = functionDecl->getLocation();
        const SourceLocation &endLoc = functionDecl->getEndLoc();

        FileID fileId = _manager.getMainFileID();
        StringRef funcContent{_manager.getCharacterData(beginLoc),
                              _manager.getCharacterData(endLoc) - _manager.getCharacterData(beginLoc) + 1UL};
        unsigned fileOffset = _manager.getFileOffset(beginLoc);
        map<unsigned, int> res{};
        int idx = 0;
        for (size_t i = 1; i < _config.keyWords.size(); ++i) {
            StringRef keyword{_config.keyWords[i]};
            size_t pos = 0;
            pos = funcContent.find(keyword, pos);
            while (pos != StringRef::npos) {
                unsigned line = _manager.getSpellingLineNumber(_manager.getComposedLoc(fileId, fileOffset + pos));
                bool insert{};
//                llvm::outs() << keyword << ": " << line << '\n';
                tie(std::ignore, insert) = res.emplace(line, idx);
                if (insert) {
                    ++idx;
                }
                pos += keyword.size();
                pos = funcContent.find(keyword, pos);
            }
        }
        return res;
    }

    void FuncInfoGenConsumer::_traverseCFGStmtToUpdateStmtVec(const Stmt *stmt, CustomCPG &customCPG,
                                                              unsigned nodeID) const {
        assert(stmt);
        customCPG.getNode(nodeID).lineRanges.push_back(_getLineRange(stmt->getBeginLoc(), stmt->getEndLoc()));
        customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _traverseCFGStmtToUpdateStmtVec(*it, customCPG, nodeID);
        }
    }

    void FuncInfoGenConsumer::_catchSpecialStmt(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const {
        if (stmt) {
            customCPG.getNode(nodeID).lineRanges.push_back(_getLineRange(stmt->getBeginLoc(), stmt->getEndLoc()));
            customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        }
    }

    pair<unsigned, unsigned>
    FuncInfoGenConsumer::_getLineRange(const SourceLocation &beginLoc, const SourceLocation &endLoc) const {
        unsigned startLine = _context.getFullLoc(beginLoc).getSpellingLineNumber();
        unsigned endLine = _context.getFullLoc(endLoc).getSpellingLineNumber();
        if (startLine <= endLine) {
            return make_pair(startLine, endLine);
        }
        return make_pair(endLine, startLine);
    }


    void FuncInfoGenConsumer::_buildCustomCPG(const FunctionDecl *funcDecl, CustomCPG &customCPG) const {
        Stmt *funcBody = funcDecl->getBody();
        unique_ptr<CFG> wholeCFG = CFG::buildCFG(funcDecl, funcBody, &_context, CFG::BuildOptions());
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

            // set attributes of node
            CustomCPG::CPGNode &node = customCPG.getNode(cur);
            for (const CFGElement &element: *block) {
                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    _traverseCFGStmtToUpdateStmtVec(stmt, customCPG, cur);
                }
            }
            _catchSpecialStmt(block->getTerminatorStmt(), customCPG, cur);
            _catchSpecialStmt(block->getLoopTarget(), customCPG, cur);
            _catchSpecialStmt(block->getLabel(), customCPG, cur);
            node.mergeLineRanges();
        }

        _buildDepnInCPG(funcDecl, wholeCFG, customCPG);
    }


    bool FuncInfoGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getLocation()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                if (_config.debug())
                    llvm::outs() << "\nFUNC: " << funcName << '\n';
                pair<unsigned, unsigned> lineRange = _getLineRange(funcDecl->getLocation(), funcDecl->getEndLoc());

                FuncInfo funcInfo(funcDecl->getQualifiedNameAsString(), lineRange, _config.ASTStmtKindMap,
                                  move(_findSensitiveLines(funcDecl, lineRange)));

                _buildCustomCPG(funcDecl, funcInfo.getCustomCPG());
                _funcInfoList.push_back(funcInfo);
            }
        }
        return true;
    }

    void FuncInfoGenConsumer::_buildDepnInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                              CustomCPG &customCPG) const {
        unique_ptr<AbstractDepnHelper> depnHelper{};
        if (customCPG.getSensitiveLineMap().empty()) {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new SimplifiedDepnHelper(wholeCFG, customCPG, _config.debug()));
        } else {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new DetailedDepnHelper(wholeCFG, _context, customCPG, _config.debug()));
        }

        depnHelper->depnOfParamDecl(funcDecl->parameters());
        depnHelper->buildDepnInCPG();

//        for (auto it = wholeCFG->rbegin(); it != wholeCFG->rend(); ++it) {
//            CFGBlock *block = *it;
//            block->print(llvm::outs(), wholeCFG.get(), LangOptions(), false);
//            unsigned nodeID = block->getBlockID();
//            depnHelper->_updateNodeID(nodeID);
//            for (const CFGElement &element: *block) {
//                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
//                    const Stmt *stmt = cfgStmt->getStmt();
//                    depnHelper->buildDepn(stmt);
//                }
//            }
//
//        }
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
                    int ret = util::numInRange(lineNo, _funcInfoList[i].getLineRange());
                    if (ret == 0) {
                        preLine = lineNo;
                        break;
                    } else if (ret < 0) {
                        preLine = 0;
                        break;
                    }
                    ++i;
                }
            } else if (preLine != 0) {
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