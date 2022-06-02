//
// Created by Unravel on 2022/4/12.
//

#include "AbstractDepnHelper.h"

namespace wfdg {
    // FIXME: 结构体处理不完善:未考虑结构体变量中数组的情况,如:structVar[i].mem,会忽略变量i;以及返回结构体的函数memFunc(structVar).mem
    //! 处理语句的依赖关系
    void AbstractDepnHelper::_buildDepn(const Stmt *stmt) {
        if (_stmtHelper.skipStmt(stmt)) {
            return;
        }
//        llvm::outs() << stmt->getStmtClassName() << '\n';
        // 判断语句类型
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass: {   // 成员表达式
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                _doDepnOfReadMember(memberExpr);
                return;
            }

            case Stmt::DeclRefExprClass: {  // 变量引用表达式
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                if (isa<VarDecl>(refExpr->getDecl())) {
                    _doDepnOfReadRef(refExpr);
                }
                return;
            }

            case Stmt::CallExprClass: { // 函数调用表达式
                const CallExpr *callExpr = cast<CallExpr>(stmt);
                if (const ImplicitCastExpr *castExpr = dyn_cast<ImplicitCastExpr>(callExpr->getCallee())) {
                    _buildDepn(castExpr->getSubExpr());
                } else {
                    _buildDepn(callExpr->getCallee());
                }
                // 遍历函数参数
                for (unsigned i = 0; i < callExpr->getNumArgs(); ++i) {
                    const Expr *argExpr = callExpr->getArg(i);
                    if (const ImplicitCastExpr *castExpr = dyn_cast<ImplicitCastExpr>(argExpr)) {
                        const Expr *expr = castExpr->getSubExpr();
                        _buildDepn(expr);   // 处理每个参数的表达式
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

            case Stmt::UnaryOperatorClass: {    // 一元运算符
                const UnaryOperator *op = cast<UnaryOperator>(stmt);
                if (op->isIncrementDecrementOp()) { // 针对自增自减
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

            case Stmt::BinaryOperatorClass: {   // 二元运算符
                const BinaryOperator *binOp = cast<BinaryOperator>(stmt);
                // 针对赋值运算符
                if (binOp->getOpcode() == BinaryOperatorKind::BO_Assign) {
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), binOp->getRHS());
                    return;
                }
                break;
            }

            case Stmt::CompoundAssignOperatorClass: {   // 复合运算符
                const CompoundAssignOperator *binOp = cast<CompoundAssignOperator>(stmt);
                if (binOp->isAssignmentOp()) {  // 是赋值语句
                    _buildDepn(binOp->getLHS());
                    _buildDepn(binOp->getRHS());
                    _depnOfWrittenVar(binOp->getLHS(), stmt);
                    return;
                }
                break;
            }

            case Stmt::DeclStmtClass: { // 声明语句
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
        // 对于其它类型则依次处理其每个子语句
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _buildDepn(*it);
        }
    }

    //! 处理写入变量
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
