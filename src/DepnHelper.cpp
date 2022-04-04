//
// Created by Unravel on 2022/4/4.
//

#include "DepnHelper.h"

namespace wfg {

    // TODO: 使用getID替换
    // TODO: 函数定义的变量放在入口结点
    void DepnHelper::buildDepn(const Stmt *stmt) {
        auto it = stmt->child_begin();
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass: {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                string name = _getStructWholeName(memberExpr);
                VarIdType readWholeVar = _customCPG.getVarPointer(name);
                VarIdType structVar = _customCPG.getVarPointer(_getStructVar(name));
                if (_noneWrittenVarInCurNode(readWholeVar) && _noneWrittenVarInCurNode(structVar)) {
                    _traceWrittenVar(_nodeID, readWholeVar, structVar);
                }
                llvm::outs() << "RMem:" << name << '\n';
            }
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
                        VarIdType writtenVar = _customCPG.getVarPointer(varDecl->getNameAsString());
                        if (const Expr *initExpr = varDecl->getInit()) {
/*                            if (varDecl->getTypeSourceInfo()->getType().getTypePtr()->isPointerType()
                                && isa<UnaryOperator>(initExpr)) {
                                const UnaryOperator *op = cast<UnaryOperator>(initExpr);
                                if (op->getOpcode() == UnaryOperatorKind::UO_AddrOf) {
                                    const Stmt* childStmt = *(op->child_begin());
                                    while (childStmt && !isa<DeclRefExpr>(childStmt) && !isa<MemberExpr>(childStmt)) {
                                        childStmt = *(childStmt->child_begin());
                                    }
                                    if(const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt)) {
                                        VarIdType refId = _customCPG.getVarPointer(refExpr->getNameInfo().getAsString());
                                        _bindPointer(writtenVar, refId);
                                    } else if(const MemberExpr *memExpr = cast<MemberExpr>(childStmt)) {
                                        VarIdType memId = _customCPG.getVarPointer(_getStructWholeName(memExpr));
                                        _bindPointer(writtenVar, memId);
                                    }
                                }
                            }*/
                            buildDepn(initExpr);
                        }

                        llvm::outs() << "WDefDecl: " << varDecl->getNameAsString() << '\n';
                        _insertWrittenVar(writtenVar);
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

    string DepnHelper::_getStructWholeName(const MemberExpr *memberExpr) {
        string name{};
        const Stmt *childStmt = *(memberExpr->child_begin());
        while (!isa<DeclRefExpr>(childStmt) && !isa<MemberExpr>(childStmt)) {
            childStmt = *(childStmt->child_begin());
        }
        if (isa<DeclRefExpr>(childStmt)) {
            const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt);
            name.append(refExpr->getNameInfo().getAsString());
        } else if (isa<MemberExpr>(childStmt)) {
            const MemberExpr *childMem = cast<MemberExpr>(childStmt);
            name.append(_getStructWholeName(childMem));
        }
        name.append(memberExpr->isArrow() ? "->" : ".");
        name.append(memberExpr->getMemberDecl()->getNameAsString());
        return name;
    }

// TODO: 递归的struct结构a.b.c
    void DepnHelper::_traceWrittenVar(unsigned searchNode, VarIdType writtenVar, VarIdType structVar) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode); vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (_noneWrittenVarInNode(predNode, writtenVar)
                && (hasVarId(structVar) || _noneWrittenVarInNode(predNode, structVar))) {
                _traceWrittenVar(predNode, writtenVar, structVar);
            } else {
                llvm::outs() << "find " << *writtenVar << " at " << predNode << '\n';
                _customCPG.addDepnEdge(predNode, _nodeID);
            }
        }
    }

    void DepnHelper::_depnOfDeclRefExpr(const Stmt *stmt) {
        const DeclRefExpr *declRefExpr = cast<DeclRefExpr>(stmt);
        if (isa<VarDecl>(declRefExpr->getDecl())) {
            VarIdType readVar = _customCPG.getVarPointer(declRefExpr->getNameInfo().getAsString());
            if (_noneWrittenVarInCurNode(readVar)) {
                _traceWrittenVar(_nodeID, readVar);
            }
            llvm::outs() << "RDecl:" << declRefExpr->getNameInfo().getAsString() << '\n';
        }
    }

    void DepnHelper::_depnOfIncDecOp(const UnaryOperator *op) {
        const Stmt *writtenExpr = *(op->child_begin());
        while (!isa<DeclRefExpr>(writtenExpr) && !isa<MemberExpr>(writtenExpr)) {
            if (writtenExpr->child_begin() == writtenExpr->child_end()) {
                return;
            }
            writtenExpr = *(writtenExpr->child_begin());
        }

        if (isa<DeclRefExpr>(writtenExpr)) {
            const DeclRefExpr *writtenRefDecl = cast<DeclRefExpr>(writtenExpr);
            VarIdType writtenVar = _customCPG.getVarPointer(writtenRefDecl->getNameInfo().getAsString());
            if (_noneWrittenVarInCurNode(writtenVar)) {
                _traceWrittenVar(_nodeID, writtenVar);
            }
            _insertWrittenVar(writtenVar);
            llvm::outs() << "RWDecl" << writtenRefDecl->getNameInfo().getAsString() << '\n';
        } else if (isa<MemberExpr>(writtenExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
            string name = _getStructWholeName(memberExpr);
            VarIdType writtenWholeVar = _customCPG.getVarPointer(name);
            VarIdType structVar = _customCPG.getVarPointer(_getStructVar(name));
            if (_noneWrittenVarInCurNode(writtenWholeVar) && _noneWrittenVarInCurNode(structVar)) {
                _traceWrittenVar(_nodeID, writtenWholeVar, structVar);
            }
            _insertWrittenVar(writtenWholeVar);
            llvm::outs() << "RWMem" << _getStructWholeName(memberExpr) << '\n';
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
            llvm::outs() << "WRef:" << writtenRefDecl->getNameInfo().getAsString() << '\n';
            VarIdType writtenVar = _customCPG.getVarPointer(writtenRefDecl->getNameInfo().getAsString());
            _insertWrittenVar(writtenVar);
        } else if (isa<MemberExpr>(writtenExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
            llvm::outs() << "WMem:" << _getStructWholeName(memberExpr) << '\n';
            VarIdType writtenVar = _customCPG.getVarPointer(_getStructWholeName(memberExpr));
            _insertWrittenVar(writtenVar);
        }
    }

}
