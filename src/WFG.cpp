//
// Created by Unravel on 2022/3/25.
//
#include "WFGGen/WFG.h"
#include "util.h"

namespace wfg {
    string WFGNode::toString(const WFGNode &node) {
        return "{id: " + to_string(node.id) + ", lineWeight: " + to_string(node.lineWeight) + ", nodeWeight: " +
               to_string(node.nodeWeight) + ", weight: " + to_string(node.weight) + ", markedLines: " +
               util::setToString(node.markedLines, util::numToString<unsigned>) + ", stmtVec: " +
               util::vecToString(node.stmtVec, util::numToString<unsigned>) + "}";
    }


    string WFG::toString() const {
        return "{funcName: " + _funcName + ", rootLine: " + to_string(_rootLine) + ", nodes: " +
               util::mapToString(_nodes, util::numToString<unsigned>, WFGNode::toString) + ", edges: "
               + util::setToString(_edges, util::numPairToString<unsigned,unsigned>) + "}";
    }
}