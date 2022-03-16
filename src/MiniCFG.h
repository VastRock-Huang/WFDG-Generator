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

    class MiniCFG {
    private:
        const unordered_map<string, unsigned> &AST_STMT_KIND_MAP;

        std::string _funcName;
        unsigned _nodeCnt;

        int _succIdx{-1};
        unsigned _succCnt{0};   // succ边的总数
        vector<unsigned> _nodesSuccCnt;
        vector<unsigned> _nodesSuccVec{};

        int _predIdx{-1};
        unsigned _predCnt{0};
        vector<unsigned> _nodesPredCnt;
        vector<unsigned> _nodesPredVec{};

        vector<unsigned> _ASTStmtVec;

        string ASTStmtVecToString() const {
            string str = "[";
            size_t i = 0, sz = AST_STMT_KIND_MAP.size();
            for(auto& item: AST_STMT_KIND_MAP) {
                str += item.first + ":"  + to_string(_ASTStmtVec[item.second]);
                if(++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }

    public:
        MiniCFG(const string &funcName, unsigned nodeCnt, const unordered_map<string, unsigned> &ASTStmtKindMap)
                : _funcName(funcName), _nodeCnt(nodeCnt), _nodesSuccCnt(nodeCnt + 1),
                  _nodesPredCnt(nodeCnt + 1), AST_STMT_KIND_MAP(ASTStmtKindMap), _ASTStmtVec(ASTStmtKindMap.size()) {}

        void addSuccEdge(unsigned cur, unsigned succ);

        void finishSuccEdges() {
            while (_succIdx<_nodeCnt) {
                _nodesSuccCnt[++_succIdx] = _succCnt;
            }
        }

        void finishPredEdges() {
            while (_predIdx < _nodeCnt) {
                _nodesPredCnt[++_predIdx] = _predCnt;
            }
        }

        void addPredEdge(unsigned cur, unsigned pred);

        void addASTStmtKind(const string &stmtKind);

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
            return "{funcName: " + _funcName + ", nodeCnt: " + to_string(_nodeCnt)
                         +", succCnt: " + to_string(_succCnt) + ", nodesSuccCnt: " + Util::numVecToString(_nodesSuccCnt)
                         +", nodesSuccVec: " + Util::numVecToString(_nodesSuccVec)
                         +", predCnt: " + to_string(_predCnt) + ", nodesPredCnt: " + Util::numVecToString(_nodesPredCnt)
                         +", nodesPredVec: " + Util::numVecToString(_nodesPredVec)
                         + ", ASTStmtVec: " + ASTStmtVecToString() + "}";
        }
    };

}

#endif //WFG_GENERATOR_MINICFG_H
