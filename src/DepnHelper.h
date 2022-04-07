//
// Created by Unravel on 2022/4/4.
//

#ifndef WFG_GENERATOR_DEPNHELPER_H
#define WFG_GENERATOR_DEPNHELPER_H

#include "CustomCPG.h"
#include <clang/AST/AST.h>
#include <unordered_map>
#include <utility>

using namespace clang;

namespace wfg {
    class DepnHelper {
    private:
        using VarIdType = int64_t;
        using VarIdPair = pair<VarIdType, VarIdType>;
        using VarVec = vector<unordered_set<VarIdType>>;
        using StructVec = vector<set<VarIdPair>>;

        CustomCPG &_customCPG;
        VarVec &_writtenVarVec;
        StructVec &_writtenStructVec;
        unsigned _nodeID;
        unordered_map<VarIdType, string> &_varMap;
        unordered_map<VarIdType, map<VarIdType, string>> &_structMap;


        bool _noneWrittenVarInNode(unsigned nodeID, VarIdType writtenVar) const {
            return _writtenVarVec.at(nodeID).count(writtenVar) == 0;
        }

        bool _noneWrittenStructInNode(unsigned nodeID, const VarIdPair &memIds) const {
            return _writtenStructVec.at(nodeID).count(memIds) == 0;
        }

        bool _noneWrittenVarInCurNode(VarIdType varId) const {
            return _writtenVarVec.at(_nodeID).count(varId) == 0;
        }

        void _traceReadVar(unsigned searchNode, VarIdType varId);

        void _traceReadStructVar(unsigned searchNode, const VarIdPair &ids);

        void _traceReadVar(VarIdType varId) {
            if (_noneWrittenVarInCurNode(varId)) {
                _traceReadVar(_nodeID, varId);
            }
        }

        void _traceReadStructVar(const VarIdPair &ids, string name) {
            _structMap.at(ids.first).emplace(ids.second, move(name));
            if (_noneWrittenVarInNode(_nodeID, ids.first)
                && _noneWrittenStructInNode(_nodeID, ids)) {
                _traceReadStructVar(_nodeID, ids);
            }
        }

        void _recordWrittenVar(VarIdType varId) {
            _writtenVarVec.at(_nodeID).insert(varId);
        }

        void _recordWrittenStruct(const VarIdPair &ids, string name) {
            if (ids.first != 0) {
                _structMap.at(ids.first).emplace(ids.second, move(name));
                _writtenStructVec.at(_nodeID).insert(ids);
            }
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfMemberExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt *writtenExpr);

        void _insertVarId(VarIdType varId, string varName) {
            _varMap.emplace(varId, move(varName));
        }

        void _insertStructId(VarIdType structId, string structName) {
            llvm::outs() << structName << ":" << structName << '\n';
            _varMap.emplace(structId, move(structName));
            _structMap.emplace(structId, map<VarIdType, string>());
        }

        string _getVarNameById(VarIdType varId) const {
            return _varMap.at(varId);
        }

        string _getStructNameByIds(const VarIdPair &ids) const {
            return _structMap.at(ids.first).at(ids.second);
        }

        static bool _hasVarId(VarIdType varId) {
            return varId != 0;
        }

        static string _getStructVar(const string &structWholeName) {
            return structWholeName.substr(0, structWholeName.find_first_of(".-"));
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
                ids = {_getRefVarId(refExpr), memberExpr->getMemberDecl()->getID()};
            } else if (isa<MemberExpr>(childStmt)) {
                const MemberExpr *childMem = cast<MemberExpr>(childStmt);
                name.append(_getStructIdsAndName(childMem, ids));
            }
            name.append(memberExpr->isArrow() ? "->" : ".");
            name.append(memberExpr->getMemberDecl()->getNameAsString());
            return name;
        }

        static bool _isStructDecl(const VarDecl *varDecl) {
            const Type *type = varDecl->getType().getTypePtr();
            while (type->isPointerType()) {
                type = type->getPointeeOrArrayElementType();
            }
            return type->isStructureType();
        }


    public:
        DepnHelper(CustomCPG &customCPG, VarVec &writtenVarVec, StructVec &writtenStructVec, unsigned nodeID)
                : _customCPG(customCPG), _writtenVarVec(writtenVarVec), _writtenStructVec(writtenStructVec),
                  _nodeID(nodeID), _varMap(customCPG.getVarMap()),
                  _structMap(customCPG.getStructMap()) {}

        void buildDepn(const Stmt *stmt);

        void depnOfDecl(const VarDecl *varDecl) {
            VarIdType id = varDecl->getID();
            if (_isStructDecl(varDecl)) {
                _insertStructId(id, varDecl->getNameAsString());
            } else {
                _insertVarId(id, varDecl->getNameAsString());
            }
            if (const Expr *initExpr = varDecl->getInit()) {
                buildDepn(initExpr);
            }

            llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
            _recordWrittenVar(id);
        }
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
