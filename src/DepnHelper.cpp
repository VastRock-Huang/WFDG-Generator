//
// Created by Unravel on 2022/4/4.
//

#include "DepnHelper.h"

namespace wfg {
    void DepnHelper::_buildDepn(const Stmt *stmt, bool canVisitCall) {
        auto it = stmt->child_begin();
        switch (stmt->getStmtClass()) {
            case Stmt::MemberExprClass:
                _depnOfMemberExpr(stmt);
                return;

            case Stmt::DeclRefExprClass:
                _depnOfDeclRefExpr(stmt);
                return;

            case Stmt::CallExprClass: {
                if (!canVisitCall) {
                    return;
                }
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
                    _depnOfIncDecOp(op);
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

    void DepnHelper::_traceReadVar(unsigned searchNode, const VarIdPair &ids, vector<RefPair> &refFrom) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (predNode <= searchNode) {
                continue;
            }
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenVarInNode(predNode, ids)) != -1) {
                llvm::outs() << "find " << _getVarNameByIds(ids) << " at " << predNode << '\n';
                _customCPG.addDepnEdge(predNode, _nodeID);
                refFrom.emplace_back(leftIdx, predNode);
            } else {
                _traceReadVar(predNode, ids, refFrom);
            }
        }
    }


    void DepnHelper::_traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                         const VarIdPair &memIds, vector<RefPair> &refFrom) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenStructInNode(predNode, varIds, memIds)) != -1) {
                llvm::outs() << "find " << _getVarNameByIds(memIds) << " at " << predNode << '\n';
                _customCPG.addDepnEdge(predNode, _nodeID);
                refFrom.emplace_back(leftIdx, predNode);
            } else {
                _traceReadStructVar(predNode, varIds, memIds, refFrom);
            }
        }
    }

    void DepnHelper::_depnOfDeclRefExpr(const Stmt *stmt) {
        const DeclRefExpr *declRefExpr = cast<DeclRefExpr>(stmt);
        if (isa<VarDecl>(declRefExpr->getDecl())) {
            _traceReadVar(_getRefVarIds(declRefExpr), _getLineNumber(declRefExpr->getLocation()));
            llvm::outs() << "R_Decl:" << declRefExpr->getNameInfo().getAsString() << '\n';
        }
    }

    void DepnHelper::_depnOfMemberExpr(const Stmt *stmt) {
        const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
        pair<VarIdType, VarIdType> ids{0, 0};
        string name = _getStructIdsAndName(memberExpr, ids);
        _traceReadStructVar(ids, name, _getLineNumber(memberExpr->getMemberLoc()));
        llvm::outs() << "R_Mem:" << name << '\n';
    }

    void DepnHelper::_depnOfIncDecOp(const UnaryOperator *op) {
        const Stmt *childExpr = op->getSubExpr();
        while (!isa<DeclRefExpr>(childExpr) && !isa<MemberExpr>(childExpr)) {
            if (childExpr->child_begin() == childExpr->child_end()) {
                return;
            }
            childExpr = *(childExpr->child_begin());
        }

        pair<VarIdType, VarIdType> ids{};
        unsigned lineNum{};
        if (isa<DeclRefExpr>(childExpr)) {
            const DeclRefExpr *declRefExpr = cast<DeclRefExpr>(childExpr);
            ids = _getRefVarIds(declRefExpr);
            lineNum = _getLineNumber(declRefExpr->getLocation());
            _traceReadVar(ids, lineNum);
//            _insertWVarInDepnMap(ids, idx,
//                                 {make_pair(ids, _hasWrittenVarInNode(_nodeID, ids))});
            llvm::outs() << "RW_Decl:" << declRefExpr->getNameInfo().getAsString() << '\n';
        } else if (isa<MemberExpr>(childExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(childExpr);
            string name = _getStructIdsAndName(memberExpr, ids);
            lineNum = _getLineNumber(memberExpr->getMemberLoc());
            _traceReadStructVar(ids, name, lineNum);
//            _insertWVarInDepnMap(
//                    ids, idx, {
//                            make_pair(ids,
//                                      _hasWrittenStructInNode(_nodeID, make_pair(0, ids.first), ids)
//                            )
//                    });
            llvm::outs() << "RW_Mem:" << name << '\n';
        }
        _recordWrittenVar(ids, {make_pair(ids, _depnPredMapper.rightVecSize() - 1)}, lineNum);
    }

    void DepnHelper::_depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) {
        while (!isa<DeclRefExpr>(writtenExpr) && !isa<MemberExpr>(writtenExpr)) {
            if (writtenExpr->child_begin() == writtenExpr->child_end()) {
                return;
            }
            writtenExpr = *(writtenExpr->child_begin());
        }
        vector<pair<VarIdPair, int>> res{};
        // 必须在record之前,防止同一变量被覆盖
        _collectRVarsOfWVar(readExpr, res);
        pair<VarIdType, VarIdType> ids{};
        if (isa<DeclRefExpr>(writtenExpr)) {
            const DeclRefExpr *writtenRefDecl = cast<DeclRefExpr>(writtenExpr);
            llvm::outs() << "W_Ref:" << writtenRefDecl->getNameInfo().getAsString() << '\n';
            ids = _getRefVarIds(writtenRefDecl);
            _recordWrittenVar(ids, move(res), _getLineNumber(writtenRefDecl->getLocation()));
        } else if (isa<MemberExpr>(writtenExpr)) {
            const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
            string name = _getStructIdsAndName(memberExpr, ids);
            _recordWrittenStruct(ids, name, move(res), _getLineNumber(memberExpr->getMemberLoc()));
            llvm::outs() << "W_Mem:" << name << '\n';
        }
    }
}
