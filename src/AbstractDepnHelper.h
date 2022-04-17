//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_ABSTRACTDEPNHELPER_H
#define WFG_GENERATOR_ABSTRACTDEPNHELPER_H

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
//            using DeclMapTy = llvm::DenseMap<const Decl *, pair<unsigned, unsigned>>;

            StmtMapTy _StmtMap;
//            DeclMapTy _DeclMap;
            unsigned _currentBlock = 0;
            unsigned _currStmt = 0;

        public:
            StmtHelper(const CFG *cfg);

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
        };

        const unique_ptr <CFG> &_cfg;
        StmtHelper _stmtHelper;
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
        AbstractDepnHelper(const unique_ptr <CFG> &cfg)
                : _cfg(cfg), _stmtHelper(cfg.get()) {}

        AbstractDepnHelper(const unique_ptr <CFG> &cfg, bool debug)
                : _cfg(cfg), _stmtHelper(cfg.get()), _debug(debug) {}

        void buildDepnInCPG() {
            for (auto it = _cfg->rbegin(); it != _cfg->rend(); ++it) {
                CFGBlock *block = *it;
//                if (_debug)
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
