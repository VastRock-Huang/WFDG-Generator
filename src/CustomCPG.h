//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CUSTOMCPG_H
#define WFG_GENERATOR_CUSTOMCPG_H

#include "util.h"
#include "DepnMapper.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <map>
#include <cassert>

using namespace std;

namespace wfg {
    class CustomCPG {
    public:
        struct CPGNode {
            // 升序且合并后的区间
            vector<pair<unsigned, unsigned>> lineRanges{};  // FIXME: need to remove
            vector<unsigned> stmtVec;

            explicit CPGNode(unsigned size) : stmtVec(size) {}

            void mergeLineRanges() {
                Util::mergeLineRanges(lineRanges);
            }

            static string toString(const CPGNode &node) {
                return "{lineRanges: " + Util::vecToString(node.lineRanges, Util::numPairToString<unsigned, unsigned>) +
                       ", stmtVec: " + Util::vecToString(node.stmtVec, Util::numToString<unsigned>) + "}";
            }
        };

    private:
        unsigned _nodeCnt{0};
        vector<CPGNode> _nodes{};

        unsigned _succIdx{0};
        unsigned _succCnt{0};   // succ边的总数
        vector<unsigned> _nodesSuccCnt{};
        vector<unsigned> _nodesSuccVec{};

        unsigned _predIdx{0};
        unsigned _predCnt{0};
        vector<unsigned> _nodesPredCnt{};
        vector<unsigned> _nodesPredVec{};

        set<pair<unsigned, unsigned>> _depnEdges{};
        DepnMapper _depnPredMapper{};

    public:

        const unordered_map<string, unsigned> &ASTStmtKindMap;

        explicit CustomCPG(const unordered_map<string, unsigned> &ASTStmtKindMap) : ASTStmtKindMap(ASTStmtKindMap) {}

        DepnMapper& getDepnMapper() {
            return _depnPredMapper;
        }

        void initNodeCnt(unsigned nodeCnt) {
            _nodeCnt = nodeCnt;
            _nodes.assign(nodeCnt, CPGNode(ASTStmtKindMap.size()));
            _nodesSuccCnt.assign(nodeCnt + 1, 0);
            _nodesPredCnt.assign(nodeCnt + 1, 0);
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

        CPGNode &getNode(unsigned nodeID) {
            return _nodes.at(nodeID);
        }

        const vector<CPGNode> &getNodes() const {
            return _nodes;
        }

        void updateNodeStmtVec(unsigned nodeID, const string &stmtName) {
            auto it = ASTStmtKindMap.find(stmtName);
            if (it != ASTStmtKindMap.end()) {
                ++(_nodes.at(nodeID).stmtVec.at(it->second));
            }
        }

        void addDepnEdge(unsigned pred, unsigned cur) {
            _depnEdges.emplace(pred, cur);
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
                unsigned predNode = pred_at(vecIdx);
                execution(predNode, curNode);
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
                unsigned succNode = succ_at(vecIdx);
                execution(succNode, curNode);
            }
        }

        const set<pair<unsigned, unsigned>> &getDepnEdges() const {
            return _depnEdges;
        }

        string toString() const {
            return "{nodeCnt: " + to_string(_nodeCnt) +
                   ", nodes: " + Util::vecToString(_nodes, CPGNode::toString) +
                   ", succCnt: " + to_string(_succCnt) +
                   ", nodesSuccCnt: " + Util::vecToString(_nodesSuccCnt, Util::numToString<unsigned>) +
                   ", nodesSuccVec: " + Util::vecToString(_nodesSuccVec, Util::numToString<unsigned>) +
                   ", predCnt: " + to_string(_predCnt) + ", nodesPredCnt: " +
                   Util::vecToString(_nodesPredCnt, Util::numToString<unsigned>) + ", nodesPredVec: " +
                   Util::vecToString(_nodesPredVec, Util::numToString<unsigned>) + ", depnEdges: " +
                   Util::setToString(_depnEdges, Util::numPairToString<unsigned, unsigned>) +
                   +"}";
        }
    };

}

#endif //WFG_GENERATOR_CUSTOMCPG_H
