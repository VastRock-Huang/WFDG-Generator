//
// Created by Unravel on 2022/4/4.
//

#ifndef WFG_GENERATOR_DEPNHELPER_H
#define WFG_GENERATOR_DEPNHELPER_H

#include "CustomCPG.h"
#include "DepnMapper.h"
#include "util.h"
#include <clang/AST/AST.h>
#include <unordered_map>
#include <utility>

using namespace clang;

namespace wfg {
    class DepnHelper {
    public:
        using VarIdType = DepnMapper::VarIdType;
        using VarIdPair = DepnMapper::VarIdPair;
        using RVarVec = DepnMapper::RVarVec;
        using WVarVec = DepnMapper::WVarVec;
        template<typename T>
        using VarMap = DepnMapper::VarMap<T>;

    private:
        CustomCPG &_customCPG;
        DepnMapper &_depnPredMapper;

        vector<VarMap<int>> _writtenVarVec;
        VarMap<int> _readVarMap{};
        unsigned _nodeID;


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

        void _traceReadVar(unsigned searchNode, const VarIdPair &ids, vector<pair<int, unsigned>> &pres);

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                 const VarIdPair &memIds, vector<pair<int, unsigned>> &pres);

        void _traceReadVar(const VarIdPair &ids) {
            vector<pair<int, unsigned>> pres{};
            int predIdx = -1;
            if ((predIdx = _hasWrittenVarInNode(_nodeID, ids)) == -1) {
                _traceReadVar(_nodeID, ids, pres);
            } else {
                pres.emplace_back(predIdx, _nodeID);
            }
            _readVarMap.emplace(ids, _depnPredMapper.pushRVarDetails(ids, move(pres)));
        }

        void _traceReadStructVar(const VarIdPair &memIds, const string &name) {
            if (memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, name);
            VarIdPair varIds = make_pair(0, memIds.first);
            RVarVec pres{};
            int predIdx = -1;
            if ((predIdx = _hasWrittenStructInNode(_nodeID, varIds, memIds)) == -1) {
                _traceReadStructVar(_nodeID, varIds, memIds, pres);
            } else {
                pres.emplace_back(predIdx, _nodeID);
            }
            _readVarMap.emplace(memIds, _depnPredMapper.pushRVarDetails(memIds, move(pres)));
        }

        void _collectRVarsOfWVar(const Stmt *stmt, WVarVec &res) const {
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                if (isa<VarDecl>(refExpr->getDecl())) {
                    VarIdPair ids = _getRefVarIds(refExpr);
                    res.emplace_back(ids, _getReadVarIdx(ids));
                }
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                VarIdPair memIds = _getStructIds(memberExpr);
                res.emplace_back(memIds, _getReadVarIdx(memIds));
            } else {
                for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
                    _collectRVarsOfWVar(*it, res);
                }
            }
        }

        void _recordWrittenVar(const VarIdPair &ids, WVarVec &&vars) {
            _writtenVarVec.at(_nodeID)
                    .emplace(ids, _depnPredMapper.pushWVarDetails(ids, move(vars)));
        }

        void _recordWrittenStruct(const VarIdPair &memIds, const string &name, WVarVec &&vars) {
            if (memIds.first != 0) {
                _insertVarIds(memIds, name);
                _writtenVarVec.at(_nodeID)
                        .emplace(memIds, _depnPredMapper.pushWVarDetails(memIds, move(vars)));
            }
        }

        void _insertVarIds(const VarIdPair &ids, const string &varName) {
            _depnPredMapper.pushVar(ids, varName);
        }

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _depnPredMapper.getVarNameByIds(ids);
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfMemberExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr);

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
//                llvm::outs() << "ids:" << ids.first << "," << ids.second << '\n';
            } else if (isa<MemberExpr>(childStmt)) {
                const MemberExpr *childMem = cast<MemberExpr>(childStmt);
                name.append(_getStructIdsAndName(childMem, ids));
            }
            name.append(memberExpr->isArrow() ? "->" : ".");
            name.append(memberExpr->getMemberDecl()->getNameAsString());
            return name;
        }

//        static bool _isStructDecl(const VarDecl *varDecl) {
//            const Type *type = varDecl->getType().getTypePtr();
//            while (type->isPointerType()) {
//                type = type->getPointeeOrArrayElementType();
//            }
//            return type->isStructureType();
//        }

        void _buildDepn(const Stmt *stmt, bool canVisitCall = false);

    public:
        DepnHelper(CustomCPG &customCPG, unsigned nodeCnt, unsigned nodeID)
                : _customCPG(customCPG), _depnPredMapper(customCPG.getDepnMapper()),
                  _writtenVarVec(nodeCnt), _nodeID(nodeID) {}

        void buildDepn(const Stmt *stmt) {
            _buildDepn(stmt, true);
        }

        void depnOfDecl(const VarDecl *varDecl) {
            VarIdPair ids = make_pair(0, varDecl->getID());
            _insertVarIds(ids, varDecl->getNameAsString());
            vector<pair<VarIdPair, int>> res{};
            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
                _collectRVarsOfWVar(initExpr, res);
            }
            _recordWrittenVar(ids, move(res));
            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
        }

        void updateNodeID(unsigned nodeID) {
            _nodeID = nodeID;
            _readVarMap.clear();
        }

        string depnMapToString() const {
            return "depnPredMapper: " + _depnPredMapper.toString();
        }
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
