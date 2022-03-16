//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_MINICFG_H
#define WFG_GENERATOR_MINICFG_H

#include "util.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>

using namespace std;

namespace wfg {


    struct CFGNode {
        vector<pair<unsigned, unsigned>> lineRanges{};
        vector<unsigned> stmtVec;

        CFGNode() = default;
        CFGNode(vector<unsigned>&& vec) :stmtVec(vec) {}

        string toString() const {
            return "{lineRange: " + Util::numPairVecToString(lineRanges) + ", stmtVec: "
                   + Util::numVecToString(stmtVec) + "}";
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

//        string ASTStmtVecToString() const {
//            string str = "[";
//            size_t i = 0, sz = AST_STMT_KIND_MAP.size();
//            for(auto& item: AST_STMT_KIND_MAP) {
//                str += item.first + ":"  + to_string(_ASTStmtVec[item.second]);
//                if(++i != sz) {
//                    str += ", ";
//                }
//            }
//            return str += "]";
//        }

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

        void setCFGNode(unsigned nodeID,const CFGNode &node){
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
            return "{funcName: " + _funcName + ", nodeCnt: " + to_string(_nodeCnt) + ", nodes: " +
                   Util::objVecToString(_nodes) + ", succCnt: " + to_string(_succCnt) + ", nodesSuccCnt: " +
                   Util::numVecToString(_nodesSuccCnt) + ", nodesSuccVec: " + Util::numVecToString(_nodesSuccVec)
                   + ", predCnt: " + to_string(_predCnt) + ", nodesPredCnt: " + Util::numVecToString(_nodesPredCnt)
                   + ", nodesPredVec: " + Util::numVecToString(_nodesPredVec) +
                   +"}";
        }
    };

}

#endif //WFG_GENERATOR_MINICFG_H
