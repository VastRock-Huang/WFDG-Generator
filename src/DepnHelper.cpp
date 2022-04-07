//
// Created by Unravel on 2022/4/4.
//

#include "DepnHelper.h"

namespace wfg {

    void DepnHelper::buildDepn(const Stmt *stmt) {
        auto it = stmt->child_begin();
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass:
                _depnOfMemberExpr(stmt);
                return;

            case Stmt::DeclRefExprClass:
                _depnOfDeclRefExpr(stmt);
                return;

            case Stmt::CallExprClass: {
                for (; it != stmt->child_end(); ++it) {
                    buildDepn(*it);
                    if (isa<UnaryOperator>(*it)) {
                        const UnaryOperator *op = cast<UnaryOperator>(*it);
                        if (op->getOpcode() == UnaryOperatorKind::UO_AddrOf) {
                            _depnOfWrittenVar(op);
                        }
                    }
                }
            }
                return;

            case Stmt::UnaryOperatorClass: {
                const UnaryOperator *op = cast<UnaryOperator>(stmt);
                if (op->isIncrementDecrementOp()) {
                    _depnOfIncDecOp(op);
                    return;
                }
            }
                break;

            case Stmt::BinaryOperatorClass: {
                const BinaryOperator *binOp = cast<BinaryOperator>(stmt);
                if (binOp->isAssignmentOp()) {
                    ++it;
                }
            }
            case Stmt::CompoundAssignOperatorClass: {
                for (; it != stmt->child_end(); ++it) {
                    buildDepn(*it);
                }
                const BinaryOperator *binOp = cast<BinaryOperator>(stmt);
                if (binOp->isAssignmentOp()) {
                    const Stmt *writtenExpr = *(binOp->child_begin());
                    _depnOfWrittenVar(writtenExpr);
                }
            }
                return;
            case Stmt::DeclStmtClass: {
                const DeclStmt *declStmt = cast<DeclStmt>(stmt);
                for (auto &decl: declStmt->decls()) {
                    if (isa<VarDecl>(decl)) {
                        const VarDecl *varDecl = cast<VarDecl>(decl);
                        depnOfDecl(varDecl);
                    }
                }
            }
                return;
            default:;
        }
        for (; it != stmt->child_end(); ++it) {
            buildDepn(*it);
        }
    }

// TODO: 递归的struct结构a.b.c
    void DepnHelper::_traceReadVar(unsigned searchNode, VarIdType varId) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode); vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (_noneWrittenVarInNode(predNode, varId)) {
                _traceReadVar(predNode, varId);
            } else {
                llvm::outs() << "find " << _getVarNameById(varId) << " at " << predNode << '\n';
                _customCPG.addDepnEdge(predNode, _nodeID);
            }
        }
    }


    void DepnHelper::_traceReadStructVar(unsigned int searchNode, const VarIdPair &ids) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (_noneWrittenVarInNode(predNode, ids.first)
                && _noneWrittenStructInNode(predNode, ids)) {
                _traceReadStructVar(predNode, ids);
            } else {
                llvm::outs() << "find " << _getStructNameByIds(ids) << " at " << predNode << '\n';
                _customCPG.addDepnEdge(predNode, _nodeID);
            }
        }
    }

    void DepnHelper::_depnOfDeclRefExpr(const Stmt *stmt) {
        const DeclRefExpr *declRefExpr = cast<DeclRefExpr>(stmt);
        if (isa<VarDecl>(declRefExpr->getDecl())) {
            VarIdType rVarId = _getRefVarId(declRefExpr);
            _traceReadVar(rVarId);
            llvm::outs() << "R_Decl:" << declRefExpr->getNameInfo().getAsString() << '\n';
        }
    }

    void DepnHelper::_depnOfMemberExpr(const Stmt *stmt) {
        const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
        pair<VarIdType, VarIdType> ids{0, 0};
        string name = _getStructIdsAndName(memberExpr, ids);
        _traceReadStructVar(ids, name);
        llvm::outs() << "R_Mem:" << name << '\n';
    }

    void DepnHelper::_depnOfIncDecOp(const UnaryOperator *op) {
        const Stmt *childExpr = *(op->child_begin());
        while (!isa<DeclRefExpr>(childExpr) && !isa<MemberExpr>(childExpr)) {
            if (childExpr->child_begin() == childExpr->child_end()) {
                return;
            }
            childExpr = *(childExpr->child_begin());
        }

        if (isa<DeclRefExpr>(childExpr)) {
            const DeclRefExpr *declRefExpr = cast<DeclRefExpr>(childExpr);
            VarIdType varId = _getRefVarId(declRefExpr);
            _traceReadVar(varId);
            _recordWrittenVar(varId);
            llvm::outs() << "RW_Decl:" << declRefExpr->getNameInfo().getAsString() << '\n';
        } else if (isa<MemberExpr>(childExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(childExpr);
            pair<VarIdType, VarIdType> ids{0, 0};
            string name = _getStructIdsAndName(memberExpr, ids);
            _traceReadStructVar(ids, name);
            _recordWrittenStruct(ids, name);
            llvm::outs() << "RW_Mem:" << name << '\n';
        }
    }

    void DepnHelper::_depnOfWrittenVar(const Stmt *writtenExpr) {
        while (!isa<DeclRefExpr>(writtenExpr) && !isa<MemberExpr>(writtenExpr)) {
            if (writtenExpr->child_begin() == writtenExpr->child_end()) {
                return;
            }
            writtenExpr = *(writtenExpr->child_begin());
        }

        if (isa<DeclRefExpr>(writtenExpr)) {
            const DeclRefExpr *writtenRefDecl = cast<DeclRefExpr>(writtenExpr);
            llvm::outs() << "W_Ref:" << writtenRefDecl->getNameInfo().getAsString() << '\n';
            VarIdType wVarId = _getRefVarId(writtenRefDecl);
            _recordWrittenVar(wVarId);
        } else if (isa<MemberExpr>(writtenExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
            pair<VarIdType, VarIdType> ids{0, 0};
            string name = _getStructIdsAndName(memberExpr, ids);
            _recordWrittenStruct(ids, name);
            llvm::outs() << "W_Mem:" << name << '\n';
        }
    }
}
