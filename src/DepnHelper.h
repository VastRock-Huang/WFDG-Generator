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
        using VarVec = vector<unordered_set<VarIdPair, pair_hash>>;

    private:
        CustomCPG &_customCPG;
        VarVec &_writtenVarVec;
        unsigned _nodeID;

        unordered_map<VarIdPair, string, pair_hash>& _hashMap;

        bool _noneWrittenVarInNode(unsigned nodeID, const VarIdPair  &ids) const {
            return _writtenVarVec.at(nodeID).count(ids) == 0;
        }

        void _traceReadVar(unsigned searchNode, const VarIdPair &ids);

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &varIds, const VarIdPair &memIds);

        void _traceReadVar(VarIdType varId) {
            VarIdPair ids = make_pair(0, varId);
            if (_noneWrittenVarInNode(_nodeID, ids)) {
                _traceReadVar(_nodeID, ids);
            }
        }

        void _traceReadStructVar(const VarIdPair &memIds, string name) {
            if(memIds.first == 0) {
                return;
            }
            _hashMap.emplace(memIds, move(name));
            VarIdPair varIds = make_pair(0, memIds.first);
            if (_noneWrittenVarInNode(_nodeID, varIds)
                && _noneWrittenVarInNode(_nodeID, memIds)) {
                _traceReadStructVar(_nodeID, varIds, memIds);
            }
        }

        void _recordWrittenVar(VarIdType varId) {
            _writtenVarVec.at(_nodeID).emplace(0, varId);
        }

        void _recordWrittenStruct(const VarIdPair &ids, string name) {
            if (ids.first != 0) {
                _hashMap.emplace(ids, move(name));
                _writtenVarVec.at(_nodeID).emplace(ids);
            }
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfMemberExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt *writtenExpr);

        void _insertVarId(VarIdType varId, string varName) {
            _hashMap.emplace(make_pair(0, varId), move(varName));
        }

        string _getVarNameByIds(const VarIdPair &ids) const {
            return _hashMap.at(ids);
        }

        static VarIdType _getRefVarId(const DeclRefExpr *refExpr) {
            return refExpr->getDecl()->getID();
        }

        static string _getStructIdsAndName(const MemberExpr *memberExpr, VarIdPair &ids) {
            const Stmt *childStmt = *(memberExpr->child_begin());
            while (!isa<DeclRefExpr>(childStmt)) {
                if (childStmt->child_begin() == childStmt->child_end()) {
                    return {};
                }
                childStmt = *(childStmt->child_begin());
            }
            string name{};
            if (isa<DeclRefExpr>(childStmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt);
                name.append(refExpr->getNameInfo().getAsString());
                ids = make_pair(_getRefVarId(refExpr), memberExpr->getMemberDecl()->getID());
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
        DepnHelper(CustomCPG &customCPG, VarVec &writtenVarVec, unsigned nodeID)
                : _customCPG(customCPG), _writtenVarVec(writtenVarVec), _nodeID(nodeID),
                 _hashMap(customCPG.varMap) {}

        void buildDepn(const Stmt *stmt) {
            _buildDepn(stmt, true);
        }

        void depnOfDecl(const VarDecl *varDecl) {
            VarIdType id = varDecl->getID();
            _insertVarId(id, varDecl->getNameAsString());

            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
            }

            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
            _recordWrittenVar(id);
        }

        void updateNodeID(unsigned nodeID) {
            _nodeID = nodeID;
        }
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
