//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_WFDG_H
#define WFG_GENERATOR_WFDG_H

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
        };

    private:
        const string _funcName;
        const unsigned _rootLine;
        map<unsigned, WFDGNode> _nodes{};
        set<pair<unsigned, unsigned>> _edges{};
        set<pair<unsigned, unsigned>> _dataDepnEdges{};

    public:
        explicit WFDG(string funcName, unsigned rootLine = 0) : _funcName(move(funcName)), _rootLine(rootLine) {}

        void addNode(unsigned nodeID, WFDGNode &&node) {
            _nodes.emplace(nodeID, move(node));
        }

        const map<unsigned, WFDGNode> &getNodes() const {
            return _nodes;
        }

        void addEdge(unsigned dest, unsigned src) {
            _edges.emplace(dest, src);
        }

        void setDepnEdges(const set<pair<unsigned, unsigned>> &depnEdges) {
            _dataDepnEdges = depnEdges;
        }

        void addDepnEdge(pair<unsigned, unsigned> depnEdge) {
            _dataDepnEdges.emplace(move(depnEdge));
        }

        string toString() const;
    };
}

#endif //WFG_GENERATOR_WFDG_H
