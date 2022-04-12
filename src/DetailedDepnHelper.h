//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_DETAILEDDEPNHELPER_H
#define WFG_GENERATOR_DETAILEDDEPNHELPER_H

#include "AbstractDepnHelper.h"
#include <clang/AST/ASTContext.h>

namespace wfg {
    class DetailedDepnHelper : public AbstractDepnHelper {
    private:
        using RefPair = DepnMapper::RefPair;
        using AssignPair = DepnMapper::AssignPair;
        template<typename T>
        using VarMap = DepnMapper::VarMap<T>;

        const ASTContext &_context;
        DepnMapper &_depnPredMapper;

        vector<VarMap<int>> _writtenVarVec;
        VarMap<int> _readVarMap{};

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _depnPredMapper.getVarNameByIds(ids);
        }

        unsigned _getLineNumber(const SourceLocation &loc) const {
            return _context.getFullLoc(loc).getSpellingLineNumber();
        }

        void _insertVarIds(const VarIdPair &ids, const string &varName) {
            _depnPredMapper.pushVar(ids, varName);
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
            auto it1 = _writtenVarVec.at(nodeID).find(varIds);
            auto it2 = _writtenVarVec.at(nodeID).find(memIds);
            auto end = _writtenVarVec.at(nodeID).end();
            if (it1 != end && it2 != end) {
                return max(it1->second, it2->second);
            } else if (it1 != end) {
                return it1->second;
            } else if (it2 != end) {
                return it2->second;
            }
            return -1;
        }

        void _traceReadVar(unsigned searchNode, const VarIdPair &ids, vector<RefPair> &refFrom);

        void _traceReadVar(const VarIdPair &ids, unsigned lineNum) {
            vector<RefPair> refFrom{};
            int predIdx = -1;
            if ((predIdx = _hasWrittenVarInNode(_nodeID, ids)) == -1) {
                _traceReadVar(_nodeID, ids, refFrom);
            } else {
                refFrom.emplace_back(predIdx, _nodeID);
            }
            int rightIdx = _depnPredMapper.pushRefInfo(ids, _nodeID, move(refFrom));
            _readVarMap.emplace(ids, rightIdx);
            if (_customCPG.inSensitiveLine(lineNum)) {
                _depnPredMapper.pushSensitiveRVar(ids, rightIdx);
            }
        }

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                 const VarIdPair &memIds, vector<RefPair> &refFrom);


        void _traceReadStructVar(const VarIdPair &memIds, const string &name, unsigned lineNum) {
            if (memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, name);
            VarIdPair varIds = make_pair(0, memIds.first);
            vector<RefPair> refFrom{};
            int predIdx = -1;
            if ((predIdx = _hasWrittenStructInNode(_nodeID, varIds, memIds)) == -1) {
                _traceReadStructVar(_nodeID, varIds, memIds, refFrom);
            } else {
                refFrom.emplace_back(predIdx, _nodeID);
            }
            int rightIdx = _depnPredMapper.pushRefInfo(memIds, _nodeID, move(refFrom));
            _readVarMap.emplace(memIds, rightIdx);
            if (_customCPG.inSensitiveLine(lineNum)) {
                _depnPredMapper.pushSensitiveRVar(memIds, rightIdx);
            }
        }

        void _collectRVarsOfWVar(const Stmt *stmt, vector<AssignPair> &assignFrom) const {
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                if (isa<VarDecl>(refExpr->getDecl())) {
                    VarIdPair ids = _getRefVarIds(refExpr);
                    assignFrom.emplace_back(ids, _getReadVarIdx(ids));
                }
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                VarIdPair memIds = _getStructIds(memberExpr);
                assignFrom.emplace_back(memIds, _getReadVarIdx(memIds));
            } else {
                for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
                    _collectRVarsOfWVar(*it, assignFrom);
                }
            }
        }

        void _recordWrittenVar(const VarIdPair &ids, vector<AssignPair> &&assignFrom, unsigned lineNum) {
            int leftIdx = _depnPredMapper.pushAssignInfo(ids, move(assignFrom));
            _writtenVarVec.at(_nodeID).emplace(ids, leftIdx);
            if (_customCPG.inSensitiveLine(lineNum)) {
                _depnPredMapper.pushSensitiveWVar(ids, leftIdx);
            }
        }

        void _recordWrittenStruct(const VarIdPair &memIds, const string &name, vector<AssignPair> &&assignFrom,
                                  unsigned lineNum) {
            if (memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, name);
            int leftIdx = _depnPredMapper.pushAssignInfo(memIds, move(assignFrom));
            _writtenVarVec.at(_nodeID).emplace(memIds, leftIdx);
            if (_customCPG.inSensitiveLine(lineNum)) {
                _depnPredMapper.pushSensitiveWVar(memIds, leftIdx);
            }
        }

        static string _getStructIdsAndName(const MemberExpr *memberExpr, VarIdPair &ids) {
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
                ids = make_pair(refExpr->getDecl()->getID(), memberExpr->getMemberDecl()->getID());
            } else if (isa<MemberExpr>(childStmt)) {
                const MemberExpr *childMem = cast<MemberExpr>(childStmt);
                name.append(_getStructIdsAndName(childMem, ids));
            }
            name.append(memberExpr->isArrow() ? "->" : ".");
            name.append(memberExpr->getMemberDecl()->getNameAsString());
            return name;
        }

        void _doDepnOfReadRef(const DeclRefExpr *refExpr) override {
            _traceReadVar(_getRefVarIds(refExpr), _getLineNumber(refExpr->getLocation()));
            llvm::outs() << "R_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
        }

        void _doDepnOfReadMember(const MemberExpr *memberExpr) override {
            VarIdPair ids{};
            string name = _getStructIdsAndName(memberExpr, ids);
            _traceReadStructVar(ids, name, _getLineNumber(memberExpr->getMemberLoc()));
            llvm::outs() << "R_Mem:" << name << '\n';
        }

        void _doDepnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) override {
            vector<AssignPair> assignFrom{};
            // 必须在record之前,防止同一变量被覆盖
            _collectRVarsOfWVar(readExpr, assignFrom);
            VarIdPair ids{};
            if (isa<DeclRefExpr>(writtenExpr)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(writtenExpr);
                ids = _getRefVarIds(refExpr);
                _recordWrittenVar(ids, move(assignFrom), _getLineNumber(refExpr->getLocation()));
                llvm::outs() << "W_Ref:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(writtenExpr)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
                string name = _getStructIdsAndName(memberExpr, ids);
                _recordWrittenStruct(ids, name, move(assignFrom), _getLineNumber(memberExpr->getMemberLoc()));
                llvm::outs() << "W_Mem:" << name << '\n';
            }
        }

        void _doDepnOfRWVar(const Stmt *stmt) override {
            VarIdPair ids{};
            unsigned lineNum{};
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                ids = _getRefVarIds(refExpr);
                lineNum = _getLineNumber(refExpr->getLocation());
                _traceReadVar(ids, lineNum);
                llvm::outs() << "RW_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                string name = _getStructIdsAndName(memberExpr, ids);
                lineNum = _getLineNumber(memberExpr->getMemberLoc());
                _traceReadStructVar(ids, name, lineNum);
                llvm::outs() << "RW_Mem:" << name << '\n';
            }
            _recordWrittenVar(ids, {make_pair(ids, _depnPredMapper.rightVecSize() - 1)}, lineNum);
        }

    public:
        DetailedDepnHelper(CustomCPG &customCPG, unsigned nodeCnt,
                           unsigned nodeID, const ASTContext &context)
                : AbstractDepnHelper(customCPG, nodeID),
                  _context(context), _depnPredMapper(customCPG.getDepnMapper()),
                  _writtenVarVec(nodeCnt) {}

        void depnOfDecl(const VarDecl *varDecl) override {
            VarIdPair ids = make_pair(0, varDecl->getID());
            _insertVarIds(ids, varDecl->getNameAsString());
            vector<AssignPair> assignFrom{};
            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
                _collectRVarsOfWVar(initExpr, assignFrom);
            }
            _recordWrittenVar(ids, move(assignFrom), _getLineNumber(varDecl->getLocation()));
            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
        }

        void updateNodeID(unsigned nodeID) override {
            _nodeID = nodeID;
            _readVarMap.clear();
        }
    };
}


#endif //WFG_GENERATOR_DETAILEDDEPNHELPER_H
