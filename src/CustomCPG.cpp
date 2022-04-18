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

    string CustomCPG::toString() const {
        return "{funcName: " + _funcName + ",\n" +
               "nodeCnt: " + to_string(_nodeCnt) + ",\n" +
               "isLoop: " + util::vecToString(_isloop) + ",\n" +
               "hasCondition: " + util::vecToString(_hasCondition) + ",\n" +
               "succCnt: " + to_string(_succCnt) + ",\n" +
               "nodesSuccCnt: " + util::vecToString(_nodesSuccCnt) + ",\n" +
               "nodesSuccVec: " + util::vecToString(_nodesSuccVec) + ",\n" +
               "predCnt: " + to_string(_predCnt) + ",\n" +
               "nodesPredCnt: " + util::vecToString(_nodesPredCnt) + ",\n" +
               "nodesPredVec: " + util::vecToString(_nodesPredVec) + ",\n" +
               "stmtVec: ...,\n" +
               "sensitiveLines:" +
               util::mapToString(_sensitiveLines, util::numToString<unsigned>,
                                 util::numToString<int>) + ",\n" +
               "contrDepnEdges: " + util::vecToString(_contrDepn) + ",\n" +
               (_sensitiveLines.empty() ?
                "dataDepnEdges: " +
                util::setToString(_dataDepnEdges, util::numPairToString<unsigned, unsigned>)
                                        : "depnMapper:\n" + _depnMapper.toString()) +
               "}";
    }

}