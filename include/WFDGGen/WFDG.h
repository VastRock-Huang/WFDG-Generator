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
    class WFDG {
    public:
        struct WFDGNode {
            unsigned id{0};
            double depnWeight{0.};
            double nodeWeight{0.};
            double weight{0.};
            vector<unsigned> stmtVec{};

            static string toString(const WFDGNode &node);
            static string toJson(const WFDGNode &node);
        };

    private:
        const string _funcName;
        const unsigned _rootLine;
        map<unsigned, WFDGNode> _nodes{};
        set<pair<unsigned, unsigned>> _edges{};
        set<pair<unsigned, unsigned>> _depnEdges{};
        set<pair<unsigned, unsigned>> _allEdges{};

    public:
        WFDG(string funcName, unsigned rootLine) : _funcName(move(funcName)), _rootLine(rootLine) {}

        string getFuncName() const {
            return _funcName;
        }

        unsigned getRootLine() const {
            return _rootLine;
        }

        void addNode(unsigned nodeID, WFDGNode &&node) {
            _nodes.emplace(nodeID, move(node));
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
