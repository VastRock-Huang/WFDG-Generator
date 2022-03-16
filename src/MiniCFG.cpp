//
// Created by Unravel on 2022/3/14.
//

#include "MiniCFG.h"

namespace wfg {
    void MiniCFG::addSuccEdge(unsigned cur, unsigned succ) {
        assert((_succIdx == -1 || cur <= _succIdx) && cur < _nodeCnt);
        _succIdx =static_cast<int>(cur + 1);

        _nodesSuccCnt[cur + 1] = ++_succCnt;
        _nodesSuccVec.push_back(succ);
    }

    void MiniCFG::addPredEdge(unsigned cur, unsigned pred) {
        assert((_predIdx == -1 || cur <= _predIdx) && cur < _nodeCnt);
        _predIdx =static_cast<int>(cur + 1);

        _nodesPredCnt[cur + 1] = ++_predCnt;
        _nodesPredVec.push_back(pred);
    }

    void MiniCFG::addASTStmtKind(const string &stmtKind) {
        auto it = AST_STMT_KIND_MAP.find(stmtKind);
        if(it!=AST_STMT_KIND_MAP.end()){
            ++_ASTStmtVec[it->second];
        }
    }
}