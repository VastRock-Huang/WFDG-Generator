//
// Created by Unravel on 2022/4/4.
//

#ifndef WFG_GENERATOR_DEPNHELPER_H
#define WFG_GENERATOR_DEPNHELPER_H

#include "CustomCPG.h"
#include <clang/AST/AST.h>
#include <unordered_map>

using namespace clang;

namespace wfg {
    class DepnHelper {
    private:
        using VarIdType = const string *;
        using VarVec = vector<unordered_set<VarIdType>>;

        CustomCPG &_customCPG;
        VarVec &_writtenVarVec;
        unsigned _nodeID;

        bool _noneWrittenVarInNode(unsigned nodeID, VarIdType writtenVar) const {
            return _writtenVarVec.at(nodeID).count(writtenVar) == 0;
        }

        bool _noneWrittenVarInCurNode(VarIdType writtenVar) const {
            return _writtenVarVec.at(_nodeID).count(writtenVar) == 0;
        }

        void _traceWrittenVar(unsigned searchNode, VarIdType writtenVar, VarIdType structVar = nullptr);

        void _insertWrittenVar(VarIdType writtenVar) {
            _writtenVarVec.at(_nodeID).insert(writtenVar);
        }

        void _depnOfDeclRefExpr(const Stmt *stmt);

        void _depnOfIncDecOp(const UnaryOperator *op);

        void _depnOfWrittenVar(const Stmt* writtenExpr);

        static bool hasVarId(VarIdType varId) {
            return varId != nullptr;
        }

        static string _getStructWholeName(const MemberExpr *memberExpr);

        static string _getStructVar(const string &structWholeName) {
            return structWholeName.substr(0, structWholeName.find_first_of(".-"));
        }

    public:
        DepnHelper(CustomCPG &customCPG, VarVec &writtenVarVec, unsigned nodeID)
                : _customCPG(customCPG), _writtenVarVec(writtenVarVec), _nodeID(nodeID) {}

        void buildDepn(const Stmt *stmt);
    };
}

#endif //WFG_GENERATOR_DEPNHELPER_H
