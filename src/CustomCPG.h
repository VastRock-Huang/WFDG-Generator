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
    //! 定制CPG
    class CustomCPG {
    private:
        const string _funcName;     //!< 函数名

        unsigned _nodeCnt{0};   //!< 结点数
        vector<bool> _isloop{}; //!< 记录每个结点是否有循环语句的vector
        vector<bool> _hasCondition{};   //!< 记录每个结点是否有条件语句的vector

        unsigned _succIdx{0};   //!< 出度边索引
        unsigned _succCnt{0};   //!< 出度边的总数
        vector<unsigned> _nodesSuccCnt{};   //!< CSR存图的结点出度边数目
        vector<unsigned> _nodesSuccVec{};   //!< CSR存图的结点出度边向量

        unsigned _predIdx{0};   //!< 入度边索引
        unsigned _predCnt{0};   //!< 入度边的总数
        vector<unsigned> _nodesPredCnt{};   //!< CSR存图的结点入度边数目
        vector<unsigned> _nodesPredVec{};   //!< CSR存图的结点入度边向量

        vector<vector<unsigned>> _stmtVec{};    //!< 每个结点的AST语法向量

        map<unsigned, int> _sensitiveLines;     //!< 敏感行及序号
        vector<unsigned> _contrDepn{};      //!< 每个结点的控制依赖结点
        set<pair<unsigned, unsigned>> _dataDepnEdges{};     //!< 数据依赖边的集合
        DepnMapper _depnMapper;     //!< 依赖关系映射器

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

        //! 初始化图结点数目
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

        //! 设置结点有循环语句
        void setIsLoop(unsigned nodeId) {
            _isloop.at(nodeId) = true;
        }


        bool isLoop(unsigned nodeId) const {
            return _isloop.at(nodeId);
        }

        //! 设置结点有条件语句
        void setHasCondition(unsigned nodeId) {
            _hasCondition.at(nodeId) = true;
        }

        bool hasCondition(unsigned nodeId) const {
            return _hasCondition.at(nodeId);
        }

        void addSuccEdge(unsigned cur, unsigned succ);

        //! 完成出度边的添加
        void finishSuccEdges() {
            // 填充CSR格式中为设定的结点数
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

        //! 完成入度边的添加
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

        //! 更新结点的语法特征向量
        //! \param[in] nodeID 结点ID
        //! \param[in] stmtName 语句类名
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

        //! 获取敏感行映射
        const map<unsigned, int> &getSensitiveLineMap() const {
            return _sensitiveLines;
        }

        //! 设置结点的控制依赖vector
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
