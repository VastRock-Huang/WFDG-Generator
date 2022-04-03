//
// Created by Unravel on 2022/3/14.
//


#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfg {

    vector<pair<unsigned, unsigned>>
    FuncInfoGenConsumer::_findSensitiveLines(const FunctionDecl *functionDecl,
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
                    assert(stmt);
                    _traverseCFGStmtToUpdateStmtVec(stmt, customCPG, cur);
                }
            }
            _catchSpecialStmt(block->getTerminatorStmt(), customCPG, cur);
            _catchSpecialStmt(block->getLoopTarget(), customCPG, cur);
            _catchSpecialStmt(block->getLabel(), customCPG, cur);
            node.mergeLineRanges();
        }

        _buildDepnInCPG(wholeCFG, customCPG);
    }


    bool FuncInfoGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getLocation()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                pair<unsigned, unsigned> lineRange = _getLineRange(funcDecl->getLocation(), funcDecl->getEndLoc());

                FuncInfo funcInfo(funcDecl->getQualifiedNameAsString(), lineRange, _config.ASTStmtKindMap);
                funcInfo.setSensitiveLines(move(_findSensitiveLines(funcDecl, lineRange)));

                _buildCustomCPG(funcDecl, funcInfo.getCustomCPG());
                _funcInfoList.push_back(funcInfo);
            }
        }
        return true;
    }

    void FuncInfoGenConsumer::_buildDepnInCPG(const unique_ptr<CFG> &wholeCFG, CustomCPG &customCPG) {
        vector<unordered_set<const string *>> writtenVarVec{wholeCFG->size()};
        for (auto it = wholeCFG->rbegin(); it != wholeCFG->rend(); ++it) {
            CFGBlock *block = *it;
            block->dump();
            unsigned nodeID = block->getBlockID();
            for (const CFGElement &element: *block) {
                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    _traverseCFGStmtToBuildDepn(stmt, customCPG, nodeID, writtenVarVec);
                }
            }
            llvm::outs() << "depE:"
                         << Util::setToString(customCPG.getDepnEdges(), Util::numPairToString<unsigned, unsigned>)
                         << '\n';
        }
    }

    void FuncInfoGenConsumer::_traverseCFGStmtToBuildDepn(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID,
                                                          vector<unordered_set<const string *>> &writtenVarVec) {
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            switch (stmt->getStmtClass()) {
                case Stmt::BinaryOperatorClass: {
                    const BinaryOperator *binOp = static_cast<const BinaryOperator *>(stmt);
                    if (binOp->isAssignmentOp()) {
                        ++it;
                    }
                }
                    break;
                default:;
            }
            _traverseCFGStmtToBuildDepn(*it, customCPG, nodeID, writtenVarVec);
        }

        switch (stmt->getStmtClass()) {
            case Stmt::DeclRefExprClass: {
                const DeclRefExpr *declRefExpr = static_cast<const DeclRefExpr *>(stmt);
                llvm::outs() << "declref:" << declRefExpr->getNameInfo().getAsString() << '\n';
                const string *readVar = customCPG.getVarPointer(declRefExpr->getNameInfo().getAsString());
                if (writtenVarVec.at(nodeID).count(readVar) == 0) {
                    _traceWrittenVar(customCPG, nodeID, nodeID,  readVar, writtenVarVec);
                }
            }
                break;
            case Stmt::BinaryOperatorClass:
            case Stmt::CompoundAssignOperatorClass: {
                const BinaryOperator *binOp = static_cast<const BinaryOperator *>(stmt);
                if (binOp->isAssignmentOp()) {
                    const Stmt *writtenExpr = *(binOp->child_begin());
                    if (writtenExpr->getStmtClass() == Stmt::DeclRefExprClass) {
                        const DeclRefExpr *writtenRefDecl = static_cast<const DeclRefExpr *>(writtenExpr);
                        const string *writtenVar = customCPG.getVarPointer(writtenRefDecl->getNameInfo().getAsString());
                        llvm::outs() << "bin:" << writtenRefDecl->getNameInfo().getAsString() << '\n';
                        writtenVarVec.at(nodeID).insert(writtenVar);
                    }
                }
            }
                break;
            case Stmt::DeclStmtClass: {
                const DeclStmt *declStmt = static_cast<const DeclStmt *>(stmt);
                for (auto &decl: declStmt->decls()) {
                    if (decl->getKind() == Decl::Kind::Var) {
                        const VarDecl *varDecl = static_cast<const VarDecl *>(decl);
                        const string *writtenVar = customCPG.getVarPointer(varDecl->getNameAsString());
                        llvm::outs() << "decl: " << varDecl->getNameAsString() << '\n';
                        writtenVarVec.at(nodeID).insert(writtenVar);
                    }
                }
            }
                break;
            default:;
        }

    }

    //! \param[in] curNode 当前查找的变量`writtenVar`所在的结点
    //! \param[in] searchNode 当前查找该结点的前驱结点中有无`writtenVar`变量被修改,初始为`curNode`
    void FuncInfoGenConsumer::_traceWrittenVar(CustomCPG &customCPG, unsigned curNode, unsigned searchNode,
                                               const string *writtenVar,
                                               vector<unordered_set<const string *>> &writtenVarVec) {
        for (unsigned vecIdx = customCPG.pred_begin(searchNode); vecIdx != customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = customCPG.pred_at(vecIdx);
            if (writtenVarVec.at(predNode).count(writtenVar) != 0) {
                llvm::outs() << "find" << *writtenVar <<" at " << predNode << '\n';
                customCPG.addDepnEdge(predNode, curNode);
            } else {
                llvm::outs() << "not find" << *writtenVar <<" at " << predNode << '\n';
                _traceWrittenVar(customCPG, curNode, predNode, writtenVar, writtenVarVec);
            }
        }
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