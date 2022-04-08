//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CUSTOMCPG_H
#define WFG_GENERATOR_CUSTOMCPG_H

#include "util.h"
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
        using VarIdType = int64_t;
        using VarIdPair = pair<VarIdType, VarIdType>;

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

    public:
        struct DepnMap {
            // 写变量idx-[(依赖的读变量id,读变量idx)...]
            unordered_map<unsigned, vector<pair<VarIdPair, unsigned>>> wVarMap{};
            // 读变量idx-[(依赖的写变量idx, 所在结点)...]
            unordered_map<unsigned, vector<pair<unsigned, unsigned>>> rVarMap{};

            static string varIdPairToString(const VarIdPair &idPair) {
                return Util::numPairToString(idPair);
            }

            static string
            idToString(const unordered_map<VarIdPair, string, pair_hash> &varMap, const VarIdPair &idPair) {
                return varMap.at(idPair);
            }

            static string wPairToString(const pair<VarIdPair, unsigned> &p) {
                return Util::pairToString(p, varIdPairToString, Util::numToString<unsigned>);
            }

            static string wVarVecToString(const vector<pair<VarIdPair, unsigned>> &wVarVec) {
                return Util::vecToString(wVarVec, wPairToString);
            }

            static string rVarVecToString(const vector<pair<unsigned, unsigned>> &rVarVec) {
                return Util::vecToString(rVarVec, Util::numPairToString<unsigned, unsigned>);
            }

            static string toString(const DepnMap &depnMap) {
                return "{wVarMap: " +
                       Util::hashmapToString(depnMap.wVarMap, Util::numToString<unsigned>, wVarVecToString)
                       + ", rVarMap: " +
                       Util::hashmapToString(depnMap.rVarMap, Util::numToString<unsigned>, rVarVecToString)
                       + "}";
            }
        };

        struct DepnMapper {
            // 写变量idx->[(依赖的读变量id,读变量idx)...]
            vector<vector<pair<VarIdPair, int>>> wVarVec{};
            // 读变量idx->[(依赖的写变量idx, 所在结点)...]
            vector<vector<pair<int, unsigned>>> rVarVec{};
            // 变量id->(写idx集合, 读idx集合)
            unordered_map<VarIdPair, pair<unordered_set<int>, unordered_set<int>>, pair_hash> predMap{};

            static string depnPairToString(const pair<unordered_set<int>, unordered_set<int>> &p) {
                auto lbd = [](const unordered_set<int> &s) -> string {
                    return Util::hashsetToString(s, Util::numToString<int>);
                };
                return Util::pairToString(p, lbd, lbd);
            }

            static string wPairToString(const pair<VarIdPair, int> &p) {
                return Util::pairToString(p, Util::numPairToString<VarIdType, VarIdType>, Util::numToString<int>);
            }

            string toString(const unordered_map<VarIdPair, string, pair_hash> &varMap) const {
                return "{wVarVec: " + Util::vecToString(
                        wVarVec, [](const auto &v) -> string {
                            return Util::vecToString(v, DepnMapper::wPairToString);
                        }) + ", rVarVec: " + Util::vecToString(
                        rVarVec, [](const auto &v) -> string {
                            return Util::vecToString(v, Util::numPairToString<int, unsigned>);
                        }) + ", predMap: " + Util::hashmapToString(
                        predMap, [&varMap](const auto &p) -> string {
                            return varMap.at(p);
                        }, DepnMapper::depnPairToString) + "}";
            }
        };

        DepnMapper depnPredMapper{};

        unordered_map<VarIdPair, string, pair_hash> varMap{};
        const unordered_map<string, unsigned> &ASTStmtKindMap;

        explicit CustomCPG(const unordered_map<string, unsigned> &ASTStmtKindMap) : ASTStmtKindMap(ASTStmtKindMap) {}

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
