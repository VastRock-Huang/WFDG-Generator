//
// Created by Unravel on 2022/3/14.
//


#include "ContrDepn.h"
#include "CPGGen.h"
#include "SimplifiedDepnHelper.h"
#include "DetailedDepnHelper.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfdg {

    map<unsigned, int> CPGGenConsumer::_findSensitiveLines(const FunctionDecl *functionDecl,
                                                           const pair<unsigned, unsigned> &lineRange) const {
        if(_config.noSensitive) {
            return {};
        }
        unsigned sensitiveLine = _config.sensitiveLine;
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

    void CPGGenConsumer::_traverseCFGStmtToUpdateStmtVec(const Stmt *stmt, CustomCPG &customCPG,
                                                         unsigned nodeID) const {
        customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _traverseCFGStmtToUpdateStmtVec(*it, customCPG, nodeID);
        }
    }

    void CPGGenConsumer::_catchSpecialStmt(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const {
        if (stmt) {
            customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        }
    }

    pair<unsigned, unsigned>
    CPGGenConsumer::_getLineRange(const SourceLocation &beginLoc, const SourceLocation &endLoc) const {
        unsigned startLine = _context.getFullLoc(beginLoc).getSpellingLineNumber();
        unsigned endLine = _context.getFullLoc(endLoc).getSpellingLineNumber();
        if (startLine <= endLine) {
            return make_pair(startLine, endLine);
        }
        return make_pair(endLine, startLine);
    }


    void CPGGenConsumer::_buildContrFlowInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                              CustomCPG &customCPG) const {
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
            for (const CFGElement &element: *block) {
                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    _traverseCFGStmtToUpdateStmtVec(stmt, customCPG, cur);
                }
            }
            const Stmt *terStmt = block->getTerminatorStmt();
            _catchSpecialStmt(terStmt, customCPG, cur);
            if (terStmt) {
                customCPG.setHasCondition(cur);
                if (isa<ForStmt>(terStmt) || isa<DoStmt>(terStmt) || isa<WhileStmt>(terStmt)) {
                    customCPG.setIsLoop(cur);
                }
            }
            _catchSpecialStmt(block->getLoopTarget(), customCPG, cur);
            _catchSpecialStmt(block->getLabel(), customCPG, cur);
        }
    }


    bool CPGGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getLocation()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                if (_config.debug)
                    llvm::outs() << "\nFUNC: " << funcName << '\n';

                pair<unsigned, unsigned> lineRange = _getLineRange(funcDecl->getLocation(), funcDecl->getEndLoc());
                if(_config.useOptimization && lineRange.second - lineRange.first <= 10) {
                    return true;
                }
                unique_ptr<CFG> wholeCFG = CFG::buildCFG(funcDecl, funcDecl->getBody(), &_context, CFG::BuildOptions());
                if(_config.useOptimization && wholeCFG->size() <= 5) {
                    return true;
                }

                CustomCPG customCPG(funcName, _config.ASTStmtKindMap, move(_findSensitiveLines(funcDecl, lineRange)));
                _buildContrFlowInCPG(funcDecl, wholeCFG, customCPG);
                _buildDepnInCPG(funcDecl, wholeCFG, customCPG);
                _customCPGList.push_back(customCPG);
            }
        }
        return true;
    }

    void CPGGenConsumer::_buildDepnInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                         CustomCPG &customCPG) const {
        unique_ptr<AbstractDepnHelper> depnHelper{};
        if (customCPG.getSensitiveLineMap().empty()) {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new SimplifiedDepnHelper(wholeCFG, customCPG, customCPG.getDataDepnEdges(), _config.debug));
        } else {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new DetailedDepnHelper(wholeCFG, _context, customCPG, customCPG.getDepnMapper(), _config.debug));
        }

        depnHelper->depnOfParamDecl(funcDecl->parameters());
        depnHelper->buildDepnInCPG();

        ContrDepn contrDepn(customCPG, wholeCFG->size());
        auto res = contrDepn.gen();
        customCPG.setContrDepn(res);
//        llvm::outs() << "dom:" << util::vecToString(res) << '\n';
    }

}