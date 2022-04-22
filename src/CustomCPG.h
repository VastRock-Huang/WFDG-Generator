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
    private:
        const string _funcName;

        unsigned _nodeCnt{0};
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

        vector<vector<unsigned>> _stmtVec{};

        map<unsigned, int> _sensitiveLines;
        vector<unsigned> _contrDepn{};
        set<pair<unsigned, unsigned>> _dataDepnEdges{};
        DepnMapper _depnMapper;

    public:
        const unordered_map<string, unsigned> &ASTStmtKindMap;

        CustomCPG(string funcName, const unordered_map<string, unsigned> &ASTStmtKindMap,
                  map<unsigned, int> &&sensitiveLines)
                : _funcName(move(funcName)), _sensitiveLines(sensitiveLines),
                  _depnMapper(_sensitiveLines.size()),
                  ASTStmtKindMap(ASTStmtKindMap) {}

        const string &getFuncName() const {
            return _funcName;
        }

        void initNodeCnt(unsigned nodeCnt) {
            _nodeCnt = nodeCnt;
            _stmtVec.assign(nodeCnt, vector<unsigned>(ASTStmtKindMap.size()));
            _nodesSuccCnt.assign(nodeCnt + 1, 0);
            _nodesPredCnt.assign(nodeCnt + 1, 0);
            _isloop.assign(nodeCnt, false);
            _hasCondition.assign(nodeCnt, false);
        }

        unsigned getNodeCnt() const {
            return _nodeCnt;
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

        void addSuccEdge(unsigned cur, unsigned succ);

        void finishSuccEdges() {
            while (_succIdx < _nodeCnt) {
                _nodesSuccCnt[++_succIdx] = _succCnt;
            }
        }

        unsigned succ_size() const {
            return _succCnt;
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

        void for_each_succ(unsigned curNode, const function<void(unsigned, unsigned)> &execution) const {
            for (unsigned vecIdx = succ_begin(curNode); vecIdx != succ_end(curNode); ++vecIdx) {
                unsigned succNode = succ_at(vecIdx);
                execution(succNode, curNode);
            }
        }

        void addPredEdge(unsigned cur, unsigned pred);

        void finishPredEdges() {
            while (_predIdx < _nodeCnt) {
                _nodesPredCnt[++_predIdx] = _predCnt;
            }
        }

        unsigned pred_size() const {
            return _predCnt;
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

        void updateNodeStmtVec(unsigned nodeID, const string &stmtName) {
            auto it = ASTStmtKindMap.find(stmtName);
            if (it != ASTStmtKindMap.end()) {
                ++_stmtVec.at(nodeID).at(it->second);
            }
        }

        vector<unsigned> getStmtVec(unsigned nodeId) const {
            return _stmtVec.at(nodeId);
        }

        int inSensitiveLine(unsigned lineNum) const {
            auto it = _sensitiveLines.find(lineNum);
            return it == _sensitiveLines.end() ? -1 : it->second;
        }

        const map<unsigned, int> &getSensitiveLineMap() const {
            return _sensitiveLines;
        }

        void setContrDepn(vector<unsigned> contrDepn) {
            _contrDepn = move(contrDepn);
        }

        unsigned getContrNode(unsigned nodeId) const {
            return _contrDepn.at(nodeId);
        }

        void addDataDepnEdge(unsigned pred, unsigned cur) {
            _dataDepnEdges.emplace(cur, pred);
        }

        set<pair<unsigned, unsigned>> &getDataDepnEdges() {
            return _dataDepnEdges;
        }

        const set<pair<unsigned, unsigned>> &getDataDepnEdges() const {
            return _dataDepnEdges;
        }

        DepnMapper &getDepnMapper() {
            return _depnMapper;
        }

        const DepnMapper &getDepnMapper() const {
            return _depnMapper;
        }

        string toString() const;
    };

}

#endif //WFG_GENERATOR_CUSTOMCPG_H
