//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_DETAILEDDEPNHELPER_H
#define WFG_GENERATOR_DETAILEDDEPNHELPER_H

#include "AbstractDepnHelper.h"
#include "CustomCPG.h"
#include <clang/AST/ASTContext.h>
#include <memory>
#include <queue>
#include <stack>

namespace wfdg {
    class DetailedDepnHelper : public AbstractDepnHelper {
    private:
        using RefPair = DepnMapper::RefPair;
        template<typename T>
        using VarMap = DepnMapper::VarMap<T>;

        const ASTContext &_context;
        const CustomCPG &_customCPG;
        DepnMapper &_depnMapper;

        vector<VarMap<int>> _writtenVarVec;
        VarMap<int> _readVarMap{};

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _depnMapper.getVarNameByIds(ids);
        }

        unsigned _getLineNumber(const SourceLocation &loc) const {
            return _context.getFullLoc(loc).getSpellingLineNumber();
        }

        void _insertVarIds(const VarIdPair &ids, const string &varName) {
            _depnMapper.pushVar(ids, varName);
        }

        int _getReadVarIdx(const VarIdPair &ids) const {
            auto it = _readVarMap.find(ids);
            if (it != _readVarMap.end()) {
                return it->second;
            }
            return -1;
        }

        int _hasWrittenVarInNode(unsigned nodeID, const VarIdPair &ids) const {
            auto it = _writtenVarVec.at(nodeID).find(ids);
            if (it != _writtenVarVec.at(nodeID).end()) {
                return it->second;
            }
            return -1;
        }

        int _hasWrittenStructInNode(unsigned nodeID, const VarIdPair &varIds, const VarIdPair &memIds) const {
            auto it1 = _writtenVarVec[nodeID].find(varIds);
            auto it2 = _writtenVarVec[nodeID].find(memIds);
            auto end = _writtenVarVec[nodeID].end();
            if (it1 != end && it2 != end) {
                // 结点序号越小越靠近出口越靠后
                return min(it1->second, it2->second);
            } else if (it1 != end) {
                return it1->second;
            } else if (it2 != end) {
                return it2->second;
            }
            return -1;
        }

        void _traceReadVar(const VarIdPair &ids, unsigned lineNum);

        void _traceReadStructVar(const VarIdPair &memIds, const string &name, unsigned lineNum);

        void _collectRVarsOfWVar(const Stmt *stmt, VarMap<int> &assignFrom) const {
            queue<const Stmt *> stmtQue{};
            stmtQue.push(stmt);
            while (!stmtQue.empty()) {
                const Stmt *s = stmtQue.front();
                stmtQue.pop();
                if (const DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(s)) {
                    if (isa<VarDecl>(refExpr->getDecl())) {
                        VarIdPair ids = _getRefVarIds(refExpr);
//                    llvm::outs() << "push" << DepnMapper::varIdPairToString(ids) <<'\n';
                        int rightIdx = _getReadVarIdx(ids);
                        if (rightIdx == -1) {
                            rightIdx = _depnMapper.getVarLastRightIdx(ids);
                        }
                        assignFrom.emplace(ids, rightIdx);
                    }
                    continue;
                }
                if (const MemberExpr *memberExpr = dyn_cast<MemberExpr>(s)) {
                    VarIdPair memIds = _getStructIds(memberExpr);
                    if (memIds.first == 0) {
                        continue;
                    }
//                llvm::outs() << "push" << DepnMapper::varIdPairToString(memIds) <<'\n';
                    int rightIdx = _getReadVarIdx(memIds);
                    if (rightIdx == -1) {
                        rightIdx = _depnMapper.getVarLastRightIdx(memIds);
                    }
                    assignFrom.emplace(memIds, rightIdx);
                    continue;
                }
                if (const BinaryOperator *binOp = dyn_cast<BinaryOperator>(s)) {
                    if (binOp->getOpcode() == BinaryOperatorKind::BO_Assign) {
                        stmtQue.push(binOp->getRHS());
                        continue;
                    }
                }

                for (auto it = s->child_begin(); it != s->child_end(); ++it) {
                    stmtQue.push(*it);
                }
            }
        }

        void _recordWrittenVar(const VarIdPair &ids, const VarMap<int> &assignFrom, unsigned lineNum) {
            int leftIdx = _depnMapper.pushAssignInfo(ids, assignFrom);
            _writtenVarVec.at(_nodeID).emplace(ids, leftIdx);
            int sensitiveIdx = _customCPG.inSensitiveLine(lineNum);
            if (sensitiveIdx != -1) {
                _depnMapper.pushSensitiveWVar(sensitiveIdx, ids, leftIdx, _nodeID);
            }
        }

        void _recordWrittenStruct(const VarIdPair &memIds, const string &name, const VarMap<int> &assignFrom,
                                  unsigned lineNum) {
            if (memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, name);
            int leftIdx = _depnMapper.pushAssignInfo(memIds, assignFrom);
            _writtenVarVec.at(_nodeID).emplace(memIds, leftIdx);
            // 结构体变量成员的写操作影响变量本身
            _writtenVarVec.at(_nodeID).emplace(make_pair(0, memIds.first), leftIdx);
            int sensitiveIdx = _customCPG.inSensitiveLine(lineNum);
            if (sensitiveIdx != -1) {
                _depnMapper.pushSensitiveWVar(sensitiveIdx, memIds, leftIdx, _nodeID);
            }
        }

        static string _getStructIdsAndName(const MemberExpr *memberExpr, VarIdPair &ids) {
            stack<const MemberExpr *> memStk;
            memStk.push(memberExpr);
            const Stmt *stmt = memberExpr;
            string name{};
            while (stmt->child_begin() != stmt->child_end()) {
                stmt = *(stmt->child_begin());
                if (!isa<DeclRefExpr>(stmt) && !isa<MemberExpr>(stmt)) {
                    continue;
                }
                if (const DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(stmt)) {
                    name.append(refExpr->getNameInfo().getAsString());
                    ids = make_pair(refExpr->getDecl()->getID(), memberExpr->getMemberDecl()->getID());
                    break;
                } else if (isa<MemberExpr>(stmt)) {
                    const MemberExpr *memExpr = cast<MemberExpr>(stmt);
                    memStk.push(memExpr);
                }
            }

            while (!memStk.empty()) {
                const MemberExpr *memExpr = memStk.top();
                memStk.pop();
                name.append(memExpr->isArrow() ? "->" : ".");
                name.append(memExpr->getMemberDecl()->getNameAsString());
            }

            return name;
        }

        void _doDepnOfReadRef(const DeclRefExpr *refExpr)
        override {
            _traceReadVar(_getRefVarIds(refExpr), _getLineNumber(refExpr->getLocation()));
            if (_debug)
                llvm::outs() << "R_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
        }

        void _doDepnOfReadMember(const MemberExpr *memberExpr)
        override {
            VarIdPair ids{};
            string name = _getStructIdsAndName(memberExpr, ids);
            if (ids.first == 0) {
                return;
            }
            _traceReadStructVar(ids, name, _getLineNumber(memberExpr->getMemberLoc()));
            if (_debug)
                llvm::outs() << "R_Mem:" << name << '\n';
        }

        void _doDepnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) override {
            VarMap<int> assignFrom{};
            // 必须在record之前,防止同一变量被覆盖
            _collectRVarsOfWVar(readExpr, assignFrom);
            VarIdPair ids{};
            if (isa<DeclRefExpr>(writtenExpr)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(writtenExpr);
                ids = _getRefVarIds(refExpr);
                _recordWrittenVar(ids, assignFrom, _getLineNumber(refExpr->getLocation()));
                if (_debug)
                    llvm::outs() << "W_Ref:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(writtenExpr)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
                string name = _getStructIdsAndName(memberExpr, ids);
                _recordWrittenStruct(ids, name, assignFrom, _getLineNumber(memberExpr->getMemberLoc()));
                if (_debug)
                    llvm::outs() << "W_Mem:" << name << '\n';
            }
        }

        void _doDepnOfRWVar(const Stmt *stmt)
        override {
            VarIdPair ids{};
            unsigned lineNum{};
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                ids = _getRefVarIds(refExpr);
                lineNum = _getLineNumber(refExpr->getLocation());
                _traceReadVar(ids, lineNum);
                if (_debug)
                    llvm::outs() << "RW_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                string name = _getStructIdsAndName(memberExpr, ids);
                lineNum = _getLineNumber(memberExpr->getMemberLoc());
                _traceReadStructVar(ids, name, lineNum);
                if (_debug)
                    llvm::outs() << "RW_Mem:" << name << '\n';
            }
            _recordWrittenVar(ids, {make_pair(ids, _depnMapper.rightVecSize() - 1)}, lineNum);
        }

        void _depnOfDecl(const VarDecl *varDecl)
        override {
            VarIdPair ids = make_pair(0, varDecl->getID());
            _insertVarIds(ids, varDecl->getNameAsString());
            VarMap<int> assignFrom{};
            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
                _collectRVarsOfWVar(initExpr, assignFrom);
            }
            _recordWrittenVar(ids, assignFrom, _getLineNumber(varDecl->getLocation()));
            if (_debug)
                llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
        }

        void _doTerminatorCondition(const Stmt *stmt)
        override {
            if (!stmt) {
                return;
            }
            queue<const Stmt *> q{};
            q.push(stmt);
            while (!q.empty()) {
                const Stmt *s = q.front();
                q.pop();
                if (isa<DeclRefExpr>(s)) {
                    const DeclRefExpr *refExpr = cast<DeclRefExpr>(s);
                    if (isa<VarDecl>(refExpr->getDecl())) {
                        VarIdPair ids = _getRefVarIds(refExpr);
                        int rightIdx = _getReadVarIdx(ids);
                        if (rightIdx == -1 && (rightIdx = _depnMapper.getVarLastRightIdx(ids)) == -1) {
                            continue;
                        }
                        _depnMapper.pushContrVar(_nodeID, ids, rightIdx);
                    }
                    continue;
                } else if (isa<MemberExpr>(s)) {
                    const MemberExpr *memberExpr = cast<MemberExpr>(s);
                    VarIdPair memIds = _getStructIds(memberExpr);
                    if (memIds.first == 0) {
                        continue;
                    }
                    int rightIdx = _getReadVarIdx(memIds);
                    if (rightIdx == -1 && (rightIdx = _depnMapper.getVarLastRightIdx(memIds)) == -1) {
                        continue;
                    }
                    _depnMapper.pushContrVar(_nodeID, memIds, rightIdx);
                    continue;
                } else if (const BinaryOperator *binOp = dyn_cast<BinaryOperator>(s)) {
                    if (binOp->isAssignmentOp()) {
                        q.push(binOp->getRHS());
                        continue;
                    }
                }
                for (auto it = s->child_begin(); it != s->child_end(); ++it) {
                    q.push(*it);
                }
            }

        }

        void _doAtNodeEnding()
        override {
            if (_debug)
                llvm::outs() << _depnMapper.toString() << '\n';
        }

        void _doNodeIdUpdate(unsigned nodeID)
        override {
            _nodeID = nodeID;
            _readVarMap.clear();
        }

    public:
        DetailedDepnHelper(
                const unique_ptr<CFG> &cfg,
                const ASTContext &context,
                const CustomCPG &customCPG, DepnMapper
                &depnMapper, bool
                debug)
                : AbstractDepnHelper(cfg, debug), _context(context), _customCPG(customCPG),
                  _depnMapper(depnMapper), _writtenVarVec(cfg->size()) {}

    };
}


#endif //WFG_GENERATOR_DETAILEDDEPNHELPER_H
