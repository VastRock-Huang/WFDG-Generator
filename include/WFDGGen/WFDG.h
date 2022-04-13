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

namespace wfg {
    class WFDG {
    public:
        struct WFDGNode {
            unsigned id{0};
            double lineWeight{0.};
            double nodeWeight{0.};
            double weight{0.};
            set<unsigned> markedLines{};
            vector<unsigned> stmtVec{};

            static string toString(const WFDGNode &node);
        };

    private:
        const string _funcName;
        const unsigned _rootLine;
        map<unsigned, WFDGNode> _nodes{};
        set<pair<unsigned, unsigned>> _edges{};
        set<pair<unsigned, unsigned>> _depnEdges{};
    public:
        explicit WFDG(string funcName, unsigned rootLine = 0) : _funcName(move(funcName)), _rootLine(rootLine) {}

        void setNodes(map<unsigned, WFDGNode> &&nodes) {
            _nodes = nodes;
        }

        const map<unsigned, WFDGNode> &getNodes() const {
            return _nodes;
        }

        void addEdge(unsigned src, unsigned dest) {
            _edges.emplace(src, dest);
        }

        void setDepnEdges(const set<pair<unsigned,unsigned>>& depnEdges) {
            _depnEdges = depnEdges;
        }

        string getFuncName() const {
            return _funcName;
        }

        string toString() const;
    };
}

#endif //WFG_GENERATOR_WFDG_H
