//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_WFDG_H
#define WFG_GENERATOR_WFDG_H

#include "Configuration.h"
#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>

using namespace std;

namespace wfdg {
    //! 加权特征依赖图
    class WFDG {
    public:
        //! 加权特征依赖图结点
        struct WFDGNode {
            unsigned id{0};     //!< 结点ID
            double depnWeight{0.};  //!< 依赖权重
            double nodeWeight{0.};  //!< 结点权重
            double weight{0.};      //!< 结点的总权重
            vector<unsigned> stmtVec{}; //!< 语法特征向量

            static string toString(const WFDGNode &node);
            static string toJson(const WFDGNode &node);
        };

    private:
        const string _funcName; //!< 函数名
        const unsigned _rootLine;   //!< 敏感行号
        map<unsigned, WFDGNode> _nodes{};   //!< 结点映射表(结点ID->结点对象)
        set<pair<unsigned, unsigned>> _edges{};     //!< 边集合
        set<pair<unsigned, unsigned>> _depnEdges{}; //!< 数据依赖边集合
        set<pair<unsigned, unsigned>> _allEdges{};  //!< 所有边的总集合

    public:
        WFDG(string funcName, unsigned rootLine) : _funcName(move(funcName)), _rootLine(rootLine) {}

        string getFuncName() const {
            return _funcName;
        }

        unsigned getRootLine() const {
            return _rootLine;
        }

        void addNode(unsigned nodeID, const WFDGNode& node) {
            _nodes.emplace(nodeID, node);
        }

        const WFDGNode& getNode(unsigned nodeId) const {
            return _nodes.at(nodeId);
        }

        const map<unsigned, WFDGNode> &getNodes() const {
            return _nodes;
        }

        unsigned getNodeCnt() const {
            return _nodes.size();
        }

        void addEdge(unsigned dest, unsigned src) {
            _edges.emplace(dest, src);
            _allEdges.emplace(dest, src);
        }

        const set<pair<unsigned, unsigned>>& getEdges() const {
            return _edges;
        }

        void setDepnEdges(const set<pair<unsigned, unsigned>> &depnEdges) {
            _depnEdges = depnEdges;
            _allEdges.insert(depnEdges.cbegin(), depnEdges.cend());
        }

        const set<pair<unsigned, unsigned>>& getDepnEdges() const {
            return _depnEdges;
        }

        void addDepnEdge(pair<unsigned, unsigned> depnEdge) {
            _depnEdges.emplace(depnEdge);
            _allEdges.emplace(move(depnEdge));
        }

        void setAllEdges(const vector<vector<unsigned>>& edges) {
            for(const vector<unsigned>& edge: edges) {
                _allEdges.emplace(make_pair(edge[0], edge[1]));
            }
        }

        const set<pair<unsigned,unsigned>>& getAllEdges() const {
            return _allEdges;
        }

        unsigned getAllEdgeCnt() const {
            return _allEdges.size();
        }

        string toString() const;

        string toJson() const;
    };

    vector<WFDG>
    genWFDGs(const vector<string> &srcPathList, const Configuration &config, vector<string> compileArgs);
}

#endif //WFG_GENERATOR_WFDG_H
