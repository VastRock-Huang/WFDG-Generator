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

namespace wfdg {
    class CustomCPG {
    public:
        struct CPGNode {
            // 升序且合并后的区间
            vector<pair<unsigned, unsigned>> lineRanges{};  // FIXME: need to remove
            vector<unsigned> stmtVec;

            explicit CPGNode(unsigned size) : stmtVec(size) {}

            void mergeLineRanges() {
                util::mergeLineRanges(lineRanges);
            }

            static string toString(const CPGNode &node) {
                return "{lineRanges: " + util::vecToString(node.lineRanges, util::numPairToString<unsigned, unsigned>) +
                       ", stmtVec: " + util::vecToString(node.stmtVec, util::numToString<unsigned>) + "}";
            }
        };

    private:
        unsigned _nodeCnt{0};
        vector<CPGNode> _nodes{};
        vector<bool> _isloop{};
        vector<bool> _hasCondition{};

        unsigned _succIdx{0};
        unsigned _succCnt{0};   // succ边的总数
        vector<unsigned> _nodesSuccCnt{};
        vector<unsigned> _nodesSuccVec{};

        unsigned _predIdx{0};
        unsigned _predCnt{0};
        vector<unsigned> _nodesPredCnt{};
        vector<unsigned> _nodesPredVec{};

        map<unsigned, int> _sensitiveLines;
        vector<unsigned> _contrDepn{};
        set<pair<unsigned, unsigned>> _dataDepnEdges{};
        DepnMapper _depnMapper;

    public:
        const unordered_map<string, unsigned> &ASTStmtKindMap;

        CustomCPG(const unordered_map<string, unsigned> &ASTStmtKindMap,
                  map<unsigned, int> &&sensitiveLines)
                : _sensitiveLines(sensitiveLines), _depnMapper(_sensitiveLines.size()),
                  ASTStmtKindMap(ASTStmtKindMap) {}

        DepnMapper &getDepnMapper() {
            return _depnMapper;
        }

        void initNodeCnt(unsigned nodeCnt) {
            _nodeCnt = nodeCnt;
            _nodes.assign(nodeCnt, CPGNode(ASTStmtKindMap.size()));
            _nodesSuccCnt.assign(nodeCnt + 1, 0);
            _nodesPredCnt.assign(nodeCnt + 1, 0);
            _isloop.assign(nodeCnt, false);
            _hasCondition.assign(nodeCnt, false);
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
            _dataDepnEdges.emplace(cur, pred);
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

        unsigned pred_size(unsigned nodeId) const {
            return pred_end(nodeId) - pred_begin(nodeId);
        }

        unsigned pred_front(unsigned nodeId) const {
            return pred_at(pred_begin(nodeId));
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

        unsigned succ_back(unsigned nodeID) const {
            return succ_at(succ_end(nodeID) - 1);
        }

        unsigned succ_size(unsigned nodeId) const {
            return succ_end(nodeId) - succ_begin(nodeId);
        }

        void for_each_succ(unsigned curNode, const function<void(unsigned, unsigned)> &execution) const {
            for (unsigned vecIdx = succ_begin(curNode); vecIdx != succ_end(curNode); ++vecIdx) {
                unsigned succNode = succ_at(vecIdx);
                execution(succNode, curNode);
            }
        }

        const map<unsigned, int> &getSensitiveLineMap() const {
            return _sensitiveLines;
        }

        int inSensitiveLine(unsigned lineNum) const {
            auto it = _sensitiveLines.find(lineNum);
            return it == _sensitiveLines.end() ? -1 : it->second;
        }

        set<pair<unsigned, unsigned>> &getDataDepnEdges() {
            return _dataDepnEdges;
        }

        const set<pair<unsigned, unsigned>> &getDataDepnEdges() const {
            return _dataDepnEdges;
        }

        const DepnMapper &getDepnMapper() const {
            return _depnMapper;
        }

        void setIsLoop(unsigned nodeId) {
            _isloop.at(nodeId) = true;
        }

        bool isLoop(unsigned nodeId) const {
            return _isloop.at(nodeId);
        }

        void setHasCondition(unsigned nodeId) {
            _hasCondition.at(nodeId) = true;
        }

        bool hasCondition(unsigned nodeId) const {
            return _hasCondition.at(nodeId);
        }

        void setContrDepn(vector<unsigned> contrDepn) {
            _contrDepn = move(contrDepn);
        }

        unsigned getContrNode(unsigned nodeId) const {
            return _contrDepn.at(nodeId);
        }

        string toString() const {
            return "{nodeCnt: " + to_string(_nodeCnt) +
                   ", nodes: " + util::vecToString(_nodes, CPGNode::toString) +
                   ", succCnt: " + to_string(_succCnt) +
                   ", nodesSuccCnt: " + util::vecToString(_nodesSuccCnt) +
                   ", nodesSuccVec: " + util::vecToString(_nodesSuccVec) +
                   ", predCnt: " + to_string(_predCnt) +
                   ", nodesPredCnt: " + util::vecToString(_nodesPredCnt) +
                   ", nodesPredVec: " + util::vecToString(_nodesPredVec) +
                   ", sensitiveLines:" +
                   util::mapToString(_sensitiveLines, util::numToString<unsigned>, util::numToString<int>) +
                   ", contrDepnEdges: " + util::vecToString(_contrDepn) +
                   (_sensitiveLines.empty() ?
                    ", dataDepnEdges: " +
                    util::setToString(_dataDepnEdges, util::numPairToString<unsigned, unsigned>)
                                            : ", _depnMapper: " + _depnMapper.toString()) +
                   "}";
        }
    };

}

#endif //WFG_GENERATOR_CUSTOMCPG_H
