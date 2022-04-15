//
// Created by Unravel on 2022/3/14.
//

#include "CustomCPG.h"

namespace wfdg {
    void CustomCPG::addSuccEdge(unsigned cur, unsigned succ) {
        assert((_succIdx == 0U || cur <= _succIdx) && cur < _nodeCnt);
        _succIdx = cur + 1U;

        _nodesSuccCnt[cur + 1U] = ++_succCnt;
        _nodesSuccVec.push_back(succ);
    }

    void CustomCPG::addPredEdge(unsigned cur, unsigned pred) {
        assert((_predIdx == 0U || cur <= _predIdx) && cur < _nodeCnt);
        _predIdx = cur + 1U;

        _nodesPredCnt[cur + 1U] = ++_predCnt;
        _nodesPredVec.push_back(pred);
    }

}