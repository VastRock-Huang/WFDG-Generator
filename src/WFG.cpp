//
// Created by Unravel on 2022/3/25.
//
#include "WFGGen/WFG.h"
#include "util.h"

namespace wfg {
    string WFGNode::toString(const WFGNode &node) {
        return "{id: " + to_string(node.id) + ", lineWeight: " + to_string(node.lineWeight) + ", nodeWeight: " +
               to_string(node.nodeWeight) + ", weight: " + to_string(node.weight) + ", markedLines: " +
               Util::setToString(node.markedLines, Util::numToString<unsigned>) + ", stmtVec: " +
               Util::vecToString(node.stmtVec, Util::numToString<unsigned>) + "}";
    }


    string WFG::toString() const {
        return "{funcName: " + _funcName + ", rootLine: " + to_string(_rootLine) + ", nodes: " +
               Util::mapToString(_nodes,Util::numToString<unsigned>, WFGNode::toString) + ", edges: "
               + Util::setToString(_edges, Util::numPairToString<unsigned,unsigned>) +"}";
    }
}