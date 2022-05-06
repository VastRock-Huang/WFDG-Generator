//
// Created by Unravel on 2022/4/12.
//

#include "AbstractDepnHelper.h"

namespace wfdg {
    // FIXME: 结构体处理不完善:未考虑结构体变量中数组的情况,如:structVar[i].mem,会忽略变量i;以及返回结构体的函数memFunc(structVar).mem
    void AbstractDepnHelper::_buildDepn(const Stmt *stmt) {
        if (_stmtHelper.skipStmt(stmt)) {
            return;
        }
//        llvm::outs() << stmt->getStmtClassName() << '\n';
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass: {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                _doDepnOfReadMember(memberExpr);
                return;
            }

            case Stmt::DeclRefExprClass: {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                if (isa<VarDecl>(refExpr->getDecl())) {
                    _doDepnOfReadRef(refExpr);
                }
                return;
            }

            case Stmt::CallExprClass: {
                const CallExpr *callExpr = cast<CallExpr>(stmt);
                if (const ImplicitCastExpr *castExpr = dyn_cast<ImplicitCastExpr>(callExpr->getCallee())) {
                    _buildDepn(castExpr->getSubExpr());
                } else {
                    _buildDepn(callExpr->getCallee());
                }
                for (unsigned i = 0; i < callExpr->getNumArgs(); ++i) {
                    const Expr *argExpr = callExpr->getArg(i);
                    if (const ImplicitCastExpr *castExpr = dyn_cast<ImplicitCastExpr>(argExpr)) {
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
                        _buildDepn(argExpr);
                    }
                }
                return;
            }

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
                break;
            }

            case Stmt::BinaryOperatorClass: {
                const BinaryOperator *binOp = cast<BinaryOperator>(stmt);
                if (binOp->getOpcode() == BinaryOperatorKind::BO_Assign) {
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), binOp->getRHS());
                    return;
                }
                break;
            }

            case Stmt::CompoundAssignOperatorClass: {
                const CompoundAssignOperator *binOp = cast<CompoundAssignOperator>(stmt);
                if (binOp->isAssignmentOp()) {
                    _buildDepn(binOp->getLHS());
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), stmt);
                    return;
                }
                break;
            }

            case Stmt::DeclStmtClass: {
                const DeclStmt *declStmt = cast<DeclStmt>(stmt);
                for (auto &decl: declStmt->decls()) {
                    if (isa<VarDecl>(decl)) {
                        const VarDecl *varDecl = cast<VarDecl>(decl);
                        _depnOfDecl(varDecl);
                    }
                }
                return;
            }

            default:;
        }
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
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

    AbstractDepnHelper::StmtHelper::StmtHelper(const CFG *cfg) {
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
                }
            }
        }
    }
}
