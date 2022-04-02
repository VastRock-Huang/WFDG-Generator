//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CUSTOMCFG_H
#define WFG_GENERATOR_CUSTOMCFG_H

#include "util.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cassert>

using namespace std;

namespace wfg {
    class CustomCFG {
    public:
        struct CFGNode {
            // 升序且合并后的区间
            vector<pair<unsigned, unsigned>> lineRanges{};
            vector<unsigned> stmtVec{};

            CFGNode() = default;

            explicit CFGNode(vector<unsigned> &&vec) : stmtVec(vec) {}

            void mergeLineRanges() {
                Util::mergeLineRanges(lineRanges);
            }

            static string toString(const CFGNode &node) {
                return "{lineRanges: " + Util::vecToString(node.lineRanges, Util::numPairToString<unsigned, unsigned>) +
                       ", stmtVec: " + Util::vecToString(node.stmtVec, Util::numToString<unsigned>) + "}";
            }
        };

    private:
        unsigned _nodeCnt{0};
        vector<CFGNode> _nodes{};

        unsigned _succIdx{0};
        unsigned _succCnt{0};   // succ边的总数
        vector<unsigned> _nodesSuccCnt{};
        vector<unsigned> _nodesSuccVec{};

        unsigned _predIdx{0};
        unsigned _predCnt{0};
        vector<unsigned> _nodesPredCnt{};
        vector<unsigned> _nodesPredVec{};

    public:
        void extendNodeCnt(unsigned nodeCnt) {
            if (nodeCnt <= _nodeCnt) {
                return;
            }
            _nodeCnt = nodeCnt;
            _nodes.resize(nodeCnt);
            _nodesSuccCnt.resize(nodeCnt + 1);
            _nodesPredCnt.resize(nodeCnt + 1);
        }

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
            _nodes.at(nodeID) = node;
        }

        const vector<CFGNode> &getNodes() const {
            return _nodes;
        }

        unsigned pred_begin(unsigned nodeId) const {
            return _nodesPredCnt.at(nodeId);
        }

        unsigned pred_end(unsigned nodeId) const {
            return _nodesPredCnt.at(nodeId + 1);
        }

        unsigned pred_at(unsigned preVecIdx) const {
            return _nodesPredVec.at(preVecIdx);
        }

        void for_each_pred(unsigned curNode, const function<void(unsigned, unsigned)> &execution) const {
            for (unsigned vecIdx = pred_begin(curNode); vecIdx != pred_end(curNode); ++vecIdx) {
                unsigned succNode = pred_at(vecIdx);
                execution(succNode, curNode);
            }
        }

        unsigned succ_begin(unsigned nodeId) const {
            return _nodesSuccCnt.at(nodeId);
        }

        unsigned succ_end(unsigned nodeId) const {
            return _nodesSuccCnt.at(nodeId + 1);
        }

        unsigned succ_at(unsigned succVecIdx) const {
            return _nodesSuccVec.at(succVecIdx);
        }

        void for_each_succ(unsigned curNode, const function<void(unsigned, unsigned)> &execution) const {
            for (unsigned vecIdx = succ_begin(curNode); vecIdx != succ_end(curNode); ++vecIdx) {
                unsigned predNode = succ_at(vecIdx);
                execution(predNode, curNode);
            }
        }

        string toString() const {
            return "{nodeCnt: " + to_string(_nodeCnt) +
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

#endif //WFG_GENERATOR_CUSTOMCFG_H
