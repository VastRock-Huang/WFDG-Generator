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
        using VarVec = vector<unordered_map<VarIdPair,unsigned, pair_hash>>;

    private:
        CustomCPG &_customCPG;
        VarVec &_writtenVarVec;
        unsigned _nodeID;

        unordered_map<VarIdPair, string, pair_hash>& _varMap;

        unordered_map<VarIdPair, CustomCPG::DepnMap, pair_hash>& _depnPredMap;
        static unsigned _varIdx;

        void _insertWVarInDepnMap(const VarIdPair& wVarIds, unsigned wVarIdx, vector<pair<VarIdPair, unsigned>> &&rVars) {
            _depnPredMap.at(wVarIds).wVarMap.emplace(wVarIdx, rVars);
        }

        void _insertRVarInDepnMap(const VarIdPair& rVarIds, unsigned rVarIdx, vector<pair<unsigned, unsigned>> &&pres) {
            _depnPredMap.at(rVarIds).rVarMap.emplace(rVarIdx, pres);
        }

        unsigned _hasWrittenVarInNode(unsigned nodeID, const VarIdPair &ids) const {
            auto it = _writtenVarVec.at(nodeID).find(ids);
            if(it != _writtenVarVec.at(nodeID).end()) {
                return it->second;
            }
            return 0;
        }

        unsigned _hasWrittenStructInNode(unsigned nodeID, const VarIdPair& varIds, const VarIdPair& memIds) const {
            auto it1 = _writtenVarVec.at(nodeID).find(varIds);
            auto it2 = _writtenVarVec.at(nodeID).find(memIds);
            auto end = _writtenVarVec.at(nodeID).end();
            if(it1 != end && it2 != end) {
                return max(it1->second, it2->second);
            } else if(it1 != end) {
                return it1->second;
            } else if(it2 != end) {
                return it2->second;
            }
            return 0;
        }

        void _traceReadVar(unsigned searchNode, const VarIdPair &ids,vector<pair<unsigned,unsigned>>& pres);

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                 const VarIdPair &memIds, vector<pair<unsigned,unsigned>>& pres);

        void _traceReadVar(const VarIdPair& ids) {
            vector<pair<unsigned,unsigned>> pres{};
            unsigned predIdx = 0;
            if((predIdx = _hasWrittenVarInNode(_nodeID, ids)) == 0) {
                _traceReadVar(_nodeID, ids, pres);
            } else {
                pres.emplace_back(predIdx,_nodeID);
            }
            if(!pres.empty()) {
                _insertRVarInDepnMap(ids, ++_varIdx, move(pres));
            }
//            if (_noneWrittenVarInNode(_nodeID, ids)) {
//                _traceReadVar(_nodeID, ids);
//            }
        }

        void _traceReadStructVar(const VarIdPair &memIds, string name) {
            if(memIds.first == 0) {
                return;
            }
            _insertVarIds(memIds, move(name));
            VarIdPair varIds = make_pair(0, memIds.first);
            vector<pair<unsigned,unsigned>> pres{};
            unsigned predIdx = 0;
            if((predIdx = _hasWrittenStructInNode(_nodeID, varIds, memIds)) == 0) {
                _traceReadStructVar(_nodeID, varIds, memIds, pres);
            } else {
                pres.emplace_back(predIdx, _nodeID);
            }
            if(!pres.empty()) {
                _insertRVarInDepnMap(memIds,++_varIdx, move(pres));
            }
//            if (_noneWrittenVarInNode(_nodeID, varIds)
//                && _noneWrittenVarInNode(_nodeID, memIds)) {
//                _traceReadStructVar(_nodeID, varIds, memIds);
//            }
        }

        unsigned _recordWrittenVar(const VarIdPair& ids) {
            _writtenVarVec.at(_nodeID).emplace(ids, ++_varIdx);
            return _varIdx;
        }

        unsigned _recordWrittenStruct(const VarIdPair &memIds, string name) {
            if (memIds.first != 0) {
                _insertVarIds(memIds, move(name));
                _writtenVarVec.at(_nodeID).emplace(memIds, ++_varIdx);
                return _varIdx;
            }
            return 0;
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfMemberExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr);

        void _insertVarIds(VarIdPair ids, string varName) {
            _depnPredMap.emplace(ids, CustomCPG::DepnMap());
            _varMap.emplace(move(ids), move(varName));
        }

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _varMap.at(ids);
        }

        static string getVarNameByIds(const DepnHelper& helper, const VarIdPair &ids) {
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

        void _collectRVarsOfWVar(const Stmt* stmt, vector<pair<VarIdPair, unsigned>>& res) const {
            if(isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                VarIdPair ids = _getRefVarIds(refExpr);
                res.emplace_back(ids, _hasWrittenVarInNode(_nodeID, ids));
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                VarIdPair memIds = _getStructIds(memberExpr);
                VarIdPair varIds = make_pair(0, memIds.first);
                res.emplace_back(memIds, _hasWrittenStructInNode(_nodeID, varIds, memIds));
            } else {
                for(auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
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

        void _buildDepn(const Stmt *stmt, bool canVisitCall = false);

    public:
        DepnHelper(CustomCPG &customCPG, VarVec &writtenVarVec, unsigned nodeID)
                : _customCPG(customCPG), _writtenVarVec(writtenVarVec), _nodeID(nodeID),
                  _varMap(customCPG.varMap),_depnPredMap(customCPG.depnPredMap) {}

        void buildDepn(const Stmt *stmt) {
            _buildDepn(stmt, true);
        }

        void depnOfDecl(const VarDecl *varDecl) {
            VarIdPair ids = make_pair(0,varDecl->getID());
            _insertVarIds(ids, varDecl->getNameAsString());

            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
            }

            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
            unsigned idx = _recordWrittenVar(ids);
            vector<pair<VarIdPair, unsigned>> res{};
            if (const Expr *initExpr = varDecl->getInit()) {
                _collectRVarsOfWVar(initExpr, res);
            }
            _insertWVarInDepnMap(ids,idx, move(res));
        }

        void updateNodeID(unsigned nodeID) {
            _nodeID = nodeID;
        }

        string depnMapToString() const {
            return "depnPredMap: " + Util::hashmapToString(_depnPredMap,
                                                           [this](const VarIdPair &p)-> string { return getVarNameByIds(*this, p); },
                                                           CustomCPG::DepnMap::toString);
        }
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
