//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_MINICFG_H
#define WFG_GENERATOR_MINICFG_H

#include "util.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <assert.h>

using namespace std;

namespace wfg {

    struct CFGNode {
        vector<pair<unsigned, unsigned>> lineRanges{};
        vector<unsigned> stmtVec;

        CFGNode() = default;

        CFGNode(vector<unsigned> &&vec) : stmtVec(vec) {}

        static string toString(const CFGNode &node) {
            return "{lineRanges: " + Util::vecToString(node.lineRanges, Util::numPairToString<unsigned, unsigned>) +
                   ", stmtVec: " + Util::vecToString(node.stmtVec, Util::numToString<unsigned>) + "}";
        }
    };

    class MiniCFG {
    private:
        std::string _funcName;

        unsigned _nodeCnt;
        vector<CFGNode> _nodes;

        int _succIdx{-1};
        unsigned _succCnt{0};   // succ边的总数
        vector<unsigned> _nodesSuccCnt;
        vector<unsigned> _nodesSuccVec{};

        int _predIdx{-1};
        unsigned _predCnt{0};
        vector<unsigned> _nodesPredCnt;
        vector<unsigned> _nodesPredVec{};

    public:
        MiniCFG(const string &funcName, unsigned nodeCnt, const unordered_map<string, unsigned> &ASTStmtKindMap)
                : _funcName(funcName), _nodeCnt(nodeCnt), _nodes(nodeCnt),
                  _nodesSuccCnt(nodeCnt + 1), _nodesPredCnt(nodeCnt + 1) {}

        void addSuccEdge(unsigned cur, unsigned succ);

        void finishSuccEdges() {
            while (_succIdx < _nodeCnt) {
                _nodesSuccCnt[++_succIdx] = _succCnt;
            }
        }

        void finishPredEdges() {
            while (_predIdx < _nodeCnt) {
                _nodesPredCnt[++_predIdx] = _predCnt;
            }
        }

        void addPredEdge(unsigned cur, unsigned pred);

        void setCFGNode(unsigned nodeID, const CFGNode &node) {
            _nodes[nodeID] = node;
        }

        std::string getFuncName() const {
            return _funcName;
        }

        unsigned getNodeCnt() const {
            return _nodeCnt;
        }

        unsigned getEntryNodeID() const {
            return _nodeCnt - 1;
        }

        unsigned getExitNodeID() const {
            return 0;
        }

        string toString() const {
            return "{funcName: " + _funcName + ", nodeCnt: " + to_string(_nodeCnt) +
                   ", nodes: " + Util::vecToString(_nodes, CFGNode::toString) +
                   ", succCnt: " + to_string(_succCnt) +
                   ", nodesSuccCnt: " + Util::vecToString(_nodesSuccCnt, Util::numToString<unsigned>) +
                   ", nodesSuccVec: " + Util::vecToString(_nodesSuccVec, Util::numToString<unsigned>) +
                   ", predCnt: " + to_string(_predCnt) + ", nodesPredCnt: " +
                   Util::vecToString(_nodesPredCnt, Util::numToString<unsigned>) + ", nodesPredVec: " +
                   Util::vecToString(_nodesPredVec, Util::numToString<unsigned>) +
                   +"}";
        }
    };

}

#endif //WFG_GENERATOR_MINICFG_H
