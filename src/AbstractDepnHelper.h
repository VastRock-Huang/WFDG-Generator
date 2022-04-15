//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_ABSTRACTDEPNHELPER_H
#define WFG_GENERATOR_ABSTRACTDEPNHELPER_H

#include "CustomCPG.h"
#include "DepnMapper.h"
#include "util.h"
#include <clang/AST/AST.h>
#include <clang/Analysis/CFG.h>
#include <unordered_map>
#include <utility>

using namespace clang;

namespace wfdg {
    class AbstractDepnHelper {
    public:
        using VarIdPair = DepnMapper::VarIdPair;

    protected:
        class StmtHelper {
        private:
            using StmtMapTy = llvm::DenseMap<const Stmt *, pair<unsigned, unsigned>>;
            using DeclMapTy = llvm::DenseMap<const Decl *, pair<unsigned, unsigned>>;

            StmtMapTy _StmtMap;
            DeclMapTy _DeclMap;
            unsigned _currentBlock = 0;
            unsigned _currStmt = 0;

        public:
            StmtHelper(const CFG *cfg) {
                if (!cfg) {
                    return;
                }
                for (CFG::const_iterator I = cfg->begin(), E = cfg->end(); I != E; ++I) {
                    unsigned j = 1;
                    for (CFGBlock::const_iterator BI = (*I)->begin(), BEnd = (*I)->end();
                         BI != BEnd; ++BI, ++j) {
                        if (Optional < CFGStmt > SE = BI->getAs<CFGStmt>()) {
                            const Stmt *stmt = SE->getStmt();
                            pair<unsigned, unsigned> P((*I)->getBlockID(), j);
                            _StmtMap[stmt] = P;

                            switch (stmt->getStmtClass()) {
                                case Stmt::DeclStmtClass:
                                    _DeclMap[cast<DeclStmt>(stmt)->getSingleDecl()] = P;
                                    break;
                                case Stmt::IfStmtClass: {
                                    const VarDecl *var = cast<IfStmt>(stmt)->getConditionVariable();
                                    if (var)
                                        _DeclMap[var] = P;
                                    break;
                                }
                                case Stmt::ForStmtClass: {
                                    const VarDecl *var = cast<ForStmt>(stmt)->getConditionVariable();
                                    if (var)
                                        _DeclMap[var] = P;
                                    break;
                                }
                                case Stmt::WhileStmtClass: {
                                    const VarDecl *var =
                                            cast<WhileStmt>(stmt)->getConditionVariable();
                                    if (var)
                                        _DeclMap[var] = P;
                                    break;
                                }
                                case Stmt::SwitchStmtClass: {
                                    const VarDecl *var =
                                            cast<SwitchStmt>(stmt)->getConditionVariable();
                                    if (var)
                                        _DeclMap[var] = P;
                                    break;
                                }
                                case Stmt::CXXCatchStmtClass: {
                                    const VarDecl *var =
                                            cast<CXXCatchStmt>(stmt)->getExceptionDecl();
                                    if (var)
                                        _DeclMap[var] = P;
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            void setBlockID(unsigned i) { _currentBlock = i; }

            void setStmtID(unsigned i) { _currStmt = i; }

            bool skipStmt(const Stmt *S) {
                StmtMapTy::iterator I = _StmtMap.find(S);
                if (I == _StmtMap.end()) {
                    return false;
                }
                if (_currentBlock >= 0 && I->second.first == _currentBlock
                    && I->second.second == _currStmt) {
                    return false;
                }
                return true;
            }

            bool skipDecl(const Decl *D) {
                DeclMapTy::iterator I = _DeclMap.find(D);
                if (I == _DeclMap.end()) {
                    return false;
                }
                if (_currentBlock >= 0 && I->second.first == _currentBlock
                    && I->second.second == _currStmt) {
                    return false;
                }
                return true;
            }
        };

        const unique_ptr <CFG> &_cfg;
        StmtHelper _stmtHelper;
        CustomCPG &_customCPG;
        unsigned _nodeID{};
        bool _debug{false};

        virtual void _doDepnOfReadRef(const DeclRefExpr *refExpr) = 0;

        virtual void _doDepnOfReadMember(const MemberExpr *memberExpr) = 0;

        virtual void _doDepnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) = 0;

        virtual void _doDepnOfRWVar(const Stmt *stmt) = 0;

        virtual void _depnOfDecl(const VarDecl *varDecl) = 0;

        virtual void _runAtNodeEnding() = 0;

        virtual void _updateNodeID(unsigned nodeID) {
            _nodeID = nodeID;
        }

        void _depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr);

        void _buildDepn(const Stmt *stmt);

        static VarIdPair _getRefVarIds(const DeclRefExpr *refExpr) {
            return make_pair(0, refExpr->getDecl()->getID());
        }

        static VarIdPair _getStructIds(const MemberExpr *memberExpr) {
            const Stmt *childStmt = *(memberExpr->child_begin());
            while (!isa<DeclRefExpr>(childStmt)) {
                if (childStmt->child_begin() == childStmt->child_end()) {
                    return {};
                }
                childStmt = *(childStmt->child_begin());
            }
            if (isa<DeclRefExpr>(childStmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt);
                return make_pair(refExpr->getDecl()->getID(), memberExpr->getMemberDecl()->getID());
            }
            return {};
        }

    public:
        AbstractDepnHelper(const unique_ptr <CFG> &cfg, CustomCPG &customCPG)
                : _cfg(cfg), _stmtHelper(cfg.get()), _customCPG(customCPG) {}

        AbstractDepnHelper(const unique_ptr <CFG> &cfg, CustomCPG &customCPG, bool debug)
                : _cfg(cfg), _stmtHelper(cfg.get()), _customCPG(customCPG), _debug(debug) {}

        void buildDepnInCPG() {
            for (auto it = _cfg->rbegin(); it != _cfg->rend(); ++it) {
                CFGBlock *block = *it;
                if (_debug)
                    block->print(llvm::outs(), _cfg.get(), LangOptions(), false);
                unsigned nodeID = block->getBlockID();
                _stmtHelper.setBlockID(nodeID);
                _updateNodeID(nodeID);
                int j = 1;
                for (const CFGElement &element: *block) {
                    _stmtHelper.setStmtID(j);
                    if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                        const Stmt *stmt = cfgStmt->getStmt();
                        _buildDepn(stmt);
                    }
                    ++j;
                }
                _runAtNodeEnding();
            }
        }

        void depnOfParamDecl(llvm::ArrayRef<ParmVarDecl *> params) {
            _updateNodeID(_cfg->size() - 1);
            for (const ParmVarDecl *paramVarDecl: params) {
                _depnOfDecl(paramVarDecl);
            }
        }
    };
}


#endif //WFG_GENERATOR_ABSTRACTDEPNHELPER_H
