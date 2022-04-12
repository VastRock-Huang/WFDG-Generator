//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H
#define WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H

#include "AbstractDepnHelper.h"

namespace wfg {
    class SimplifiedDepnHelper : public AbstractDepnHelper {
    private:
        vector<unordered_set<VarIdPair, util::pair_hash>> _writtenVarVec;

        bool _noneWrittenVarInNode(unsigned nodeID, const VarIdPair &ids) const {
            return _writtenVarVec.at(nodeID).count(ids) == 0;
        }

        bool _noneWrittenStructInNode(unsigned nodeID, const VarIdPair &varIds, const VarIdPair &memIds) const {
            return _writtenVarVec.at(nodeID).count(varIds) == 0 &&
                   _writtenVarVec.at(nodeID).count(memIds) == 0;
        }

        void _traceReadVar(unsigned searchNode, const VarIdPair &ids);

        void _traceReadVar(const VarIdPair &ids) {
            if (_noneWrittenVarInNode(_nodeID, ids)) {
                _traceReadVar(_nodeID, ids);
            }
        }

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                 const VarIdPair &memIds);

        void _traceReadStructVar(const VarIdPair &memIds) {
            if (memIds.first == 0) {
                return;
            }
            VarIdPair varIds = make_pair(0, memIds.first);
            if (_noneWrittenStructInNode(_nodeID, varIds, memIds)) {
                _traceReadStructVar(_nodeID, varIds, memIds);
            }
        }

        void _recordWrittenVar(const VarIdPair &ids) {
            _writtenVarVec.at(_nodeID).emplace(ids);
        }

        void _recordWrittenStruct(const VarIdPair &memIds, const string &name) {
            if (memIds.first != 0) {
                _writtenVarVec.at(_nodeID).emplace(memIds);
            }
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

        static string _getStructName(const MemberExpr *memberExpr) {
            const Stmt *childStmt = *(memberExpr->child_begin());
            while (!isa<DeclRefExpr>(childStmt) && !isa<MemberExpr>(childStmt)) {
                if (childStmt->child_begin() == childStmt->child_end()) {
                    return {};
                }
                childStmt = *(childStmt->child_begin());
            }
            string name{};
            if (isa<DeclRefExpr>(childStmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt);
                name.append(refExpr->getNameInfo().getAsString());
            } else if (isa<MemberExpr>(childStmt)) {
                const MemberExpr *childMem = cast<MemberExpr>(childStmt);
                name.append(_getStructName(childMem));
            }
            name.append(memberExpr->isArrow() ? "->" : ".");
            name.append(memberExpr->getMemberDecl()->getNameAsString());
            return name;
        }

        virtual void _doDepnOfReadRef(const DeclRefExpr *refExpr) override {
            _traceReadVar(_getRefVarIds(refExpr));
            llvm::outs() << "R_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
        };

        virtual void _doDepnOfReadMember(const MemberExpr *memberExpr) override {
            VarIdPair ids = _getStructIds(memberExpr);
            _traceReadStructVar(ids);
            llvm::outs() << "R_Mem:" << _getStructName(memberExpr) << '\n';
        }

        virtual void _doDepnOfWrittenRef(const DeclRefExpr *refExpr) override {
            VarIdPair ids = _getRefVarIds(refExpr);
            _recordWrittenVar(ids);
            llvm::outs() << "W_Ref:" << refExpr->getNameInfo().getAsString() << '\n';
        }

        virtual void _doDepnOfWrittenMember(const MemberExpr *memberExpr) override {
            VarIdPair ids = _getStructIds(memberExpr);
            if (ids.second != 0) {
                _recordWrittenVar(ids);
            }
            llvm::outs() << "W_Mem:" << _getStructName(memberExpr) << '\n';
        }

        virtual void _doDepnOfRWVar(const Stmt *stmt) override {
            pair<VarIdType, VarIdType> ids{};
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                ids = _getRefVarIds(refExpr);
                _traceReadVar(ids);
                llvm::outs() << "RW_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                ids = _getStructIds(memberExpr);
                _traceReadStructVar(ids);
                llvm::outs() << "RW_Mem:" << _getStructName(memberExpr) << '\n';
            }
            _recordWrittenVar(ids);
        }

    public:
        SimplifiedDepnHelper(CustomCPG &customCPG, unsigned nodeCnt, unsigned nodeID)
                : AbstractDepnHelper(customCPG, nodeID), _writtenVarVec(nodeCnt) {}

        virtual void depnOfDecl(const VarDecl *varDecl) override {
            VarIdPair ids = make_pair(0, varDecl->getID());
            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
            }
            _recordWrittenVar(ids);
            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
        }
    };
}

#endif //WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H
