//
// Created by Unravel on 2022/4/12.
//

#include "AbstractDepnHelper.h"

namespace wfdg {
    void AbstractDepnHelper::_buildDepn(const Stmt *stmt) {
        auto it = stmt->child_begin();
        if(_stmtHelper.skipStmt(stmt)) {
            return;
        }
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass: {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                _doDepnOfReadMember(memberExpr);
            }
                return;


            case Stmt::DeclRefExprClass: {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                if (isa<VarDecl>(refExpr->getDecl())) {
                    _doDepnOfReadRef(refExpr);
                }
            }
                return;

            case Stmt::CallExprClass: {
                // 跳过函数类型转换
                for (++it; it != stmt->child_end(); ++it) {
                    if (isa<ImplicitCastExpr>(*it)) {
                        const ImplicitCastExpr *castExpr = cast<ImplicitCastExpr>(*it);
                        const Expr *expr = castExpr->getSubExpr();
                        _buildDepn(expr);
                        // 对于非const指针的处理
                        if (!castExpr->getType().isConstQualified() && castExpr->getType().getTypePtr()->isPointerType()
                            && isa<UnaryOperator>(expr)) {
                            const UnaryOperator *op = cast<UnaryOperator>(expr);
                            if (op->getOpcode() == UnaryOperatorKind::UO_AddrOf) {
                                _depnOfWrittenVar(op->getSubExpr(), op->getSubExpr());
                            }
                        }
                    } else {
                        _buildDepn(*it);
                    }
                }
            }
                return;

            case Stmt::UnaryOperatorClass: {
                const UnaryOperator *op = cast<UnaryOperator>(stmt);
                if (op->isIncrementDecrementOp()) {
                    const Stmt *childExpr = op->getSubExpr();
                    while (!isa<DeclRefExpr>(childExpr) && !isa<MemberExpr>(childExpr)) {
                        if (childExpr->child_begin() == childExpr->child_end()) {
                            return;
                        }
                        childExpr = *(childExpr->child_begin());
                    }
                    _doDepnOfRWVar(childExpr);
                    return;
                }
            }
                break;

            case Stmt::BinaryOperatorClass: {
                const BinaryOperator *binOp = cast<BinaryOperator>(stmt);
                if (binOp->isAssignmentOp()) {
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), binOp->getRHS());
                    return;
                }
            }
                break;

            case Stmt::CompoundAssignOperatorClass: {
                const CompoundAssignOperator *binOp = cast<CompoundAssignOperator>(stmt);
                if (binOp->isAssignmentOp()) {
                    _buildDepn(binOp->getLHS());
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), stmt);
                    return;
                }
            }
                break;

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
            _buildDepn(*it);
        }
    }

    void AbstractDepnHelper::_depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) {
        while (!isa<DeclRefExpr>(writtenExpr) && !isa<MemberExpr>(writtenExpr)) {
            if (writtenExpr->child_begin() == writtenExpr->child_end()) {
                return;
            }
            writtenExpr = *(writtenExpr->child_begin());
        }
        _doDepnOfWrittenVar(writtenExpr, readExpr);
    }
}
