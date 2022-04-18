//
// Created by Unravel on 2022/3/25.
//
#include "WFDGGen/WFDG.h"
#include "util.h"

namespace wfdg {
    string WFDG::WFDGNode::toString(const WFDGNode &node) {
        return "{"
               "id: " + to_string(node.id) +
               ", depnWeight: " + to_string(node.depnWeight) +
               ", nodeWeight: " + to_string(node.nodeWeight) +
               ", weight: " + to_string(node.weight) +
               ", stmtVec: " + util::vecToString(node.stmtVec) +
               "}\n";
    }


    string WFDG::toString() const {
        return "{funcName: " + _funcName + ", " +
               "rootLine: " + to_string(_rootLine) + ",\n" +
               "nodes:\n" + util::mapToString(_nodes, util::numToString<unsigned>,
                                              WFDGNode::toString) + ", " +
               "edges: " + util::setToString(_edges, util::numPairToString<unsigned, unsigned>) + ",\n" +
               "dataDepnEdges: " +
               util::setToString(_dataDepnEdges, util::numPairToString<unsigned, unsigned>) +
               "}";
    }
}