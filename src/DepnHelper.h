//
// Created by Unravel on 2022/4/4.
//

#ifndef WFG_GENERATOR_DEPNHELPER_H
#define WFG_GENERATOR_DEPNHELPER_H

#include "CustomCPG.h"
#include "util.h"
#include <clang/AST/AST.h>
#include <unordered_map>
#include <utility>

using namespace clang;

namespace wfg {
    class DepnHelper {
    public:
        using VarIdType = CustomCPG::VarIdType;
        using VarIdPair = CustomCPG::VarIdPair;
        using VarVec = vector<unordered_map<VarIdPair, int, pair_hash>>;

    private:
        CustomCPG &_customCPG;
        VarVec &_writtenVarVec;
        unsigned _nodeID;

        unordered_map<VarIdPair, string, pair_hash> &_varMap;

//        unordered_map<VarIdPair, CustomCPG::DepnMap, pair_hash> &_depnPredMap;
//        static unsigned _varIdx;

        CustomCPG::DepnMapper &_depnPredMapper;

//        void
//        _insertWVarInDepnMap(const VarIdPair &wVarIds, unsigned wVarIdx, vector<pair<VarIdPair, unsigned>> &&rVars) {
////            _depnPredMap.at(wVarIds).wVarMap.emplace(wVarIdx, rVars);
//            _depnPredMapper.predMap.at(wVarIds).first.emplace(_depnPredMapper.wVarVec.size());
//            _depnPredMapper.wVarVec.emplace_back(rVars);
//
//        }

//        void _insertRVarInDepnMap(const VarIdPair &rVarIds, unsigned rVarIdx, vector<pair<unsigned, unsigned>> &&pres) {
////            _depnPredMap.at(rVarIds).rVarMap.emplace(rVarIdx, pres);
//            _depnPredMapper.predMap.at(rVarIds).second.emplace(_depnPredMapper.rVarVec.size());
//            _depnPredMapper.rVarVec.emplace_back(pres);
//        }

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
            _depnPredMapper.predMap.at(ids).second.emplace(_depnPredMapper.rVarVec.size());
            _depnPredMapper.rVarVec.emplace_back(pres);

//            if (_noneWrittenVarInNode(_nodeID, ids)) {
//                _traceReadVar(_nodeID, ids);
//            }
        }

        void _traceReadStructVar(const VarIdPair &memIds, string name) {
            if (memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, move(name));
            VarIdPair varIds = make_pair(0, memIds.first);
            vector<pair<int, unsigned>> pres{};
            int predIdx = -1;
            if ((predIdx = _hasWrittenStructInNode(_nodeID, varIds, memIds)) == -1) {
                _traceReadStructVar(_nodeID, varIds, memIds, pres);
            } else {
                pres.emplace_back(predIdx, _nodeID);
            }
            _depnPredMapper.predMap.at(memIds).second.emplace(_depnPredMapper.rVarVec.size());
            _depnPredMapper.rVarVec.emplace_back(pres);
//            if (_noneWrittenVarInNode(_nodeID, varIds)
//                && _noneWrittenVarInNode(_nodeID, memIds)) {
//                _traceReadStructVar(_nodeID, varIds, memIds);
//            }
        }

        void _recordWrittenVar(const VarIdPair &ids, vector<pair<VarIdPair, int>> &&rVars) {
            _writtenVarVec.at(_nodeID).emplace(ids, _depnPredMapper.wVarVec.size());
            _depnPredMapper.predMap.at(ids).first.emplace(_depnPredMapper.wVarVec.size());
            _depnPredMapper.wVarVec.emplace_back(rVars);
        }

        void _recordWrittenStruct(const VarIdPair &memIds, string name, vector<pair<VarIdPair, int>> &&rVars) {
            if (memIds.first != 0) {
                _insertVarIds(memIds, move(name));
                _writtenVarVec.at(_nodeID).emplace(memIds, _depnPredMapper.wVarVec.size());
                _depnPredMapper.predMap.at(memIds).first.emplace(_depnPredMapper.wVarVec.size());
                _depnPredMapper.wVarVec.emplace_back(rVars);
            }
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfMemberExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr);

        void _insertVarIds(VarIdPair ids, string varName) {
            _depnPredMapper.predMap.emplace(ids, make_pair(unordered_set<int>(),unordered_set<int>()));
            _varMap.emplace(move(ids), move(varName));
        }

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _varMap.at(ids);
        }

        static string getVarNameByIds(const DepnHelper &helper, const VarIdPair &ids) {
            return helper._varMap.at(ids);
        }

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

        void _collectRVarsOfWVar(const Stmt *stmt, vector<pair<VarIdPair, int>> &res) const {
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                VarIdPair ids = _getRefVarIds(refExpr);
                // FIXME: 变量的读idx错误
                res.emplace_back(ids, _hasWrittenVarInNode(_nodeID, ids));
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                VarIdPair memIds = _getStructIds(memberExpr);
                VarIdPair varIds = make_pair(0, memIds.first);
                res.emplace_back(memIds, _hasWrittenStructInNode(_nodeID, varIds, memIds));
            } else {
                for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
                    _collectRVarsOfWVar(*it, res);
                }
            }
        }

//        static bool _isStructDecl(const VarDecl *varDecl) {
//            const Type *type = varDecl->getType().getTypePtr();
//            while (type->isPointerType()) {
//                type = type->getPointeeOrArrayElementType();
//            }
//            return type->isStructureType();
//        }

        void _buildDepn(const Stmt *stmt, bool canVisitCall = true);

    public:
        DepnHelper(CustomCPG &customCPG, VarVec &writtenVarVec, unsigned nodeID)
                : _customCPG(customCPG), _writtenVarVec(writtenVarVec), _nodeID(nodeID),
                  _varMap(customCPG.varMap),
                  _depnPredMapper(customCPG.depnPredMapper) {}

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
        }

        string depnMapToString() const {
            return "depnPredMapper: " + _depnPredMapper.toString(_varMap);
        }
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
