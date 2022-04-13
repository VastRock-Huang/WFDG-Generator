//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_WFG_H
#define WFG_GENERATOR_WFG_H

#include <string>
#include <set>
#include <map>
#include <vector>
#include <utility>

using namespace std;

namespace wfg {
    struct WFGNode {
        unsigned id{0};
        double lineWeight{0.};
        double nodeWeight{0.};
        double weight{0.};
        set<unsigned> markedLines{};
        vector<unsigned> stmtVec{};

        static string toString(const WFGNode &node);
    };

    class WFG {
    private:
        const string _funcName;
        const unsigned _rootLine;
        map<unsigned, WFGNode> _nodes{};
        set<pair<unsigned, unsigned>> _edges{};
        set<pair<unsigned, unsigned>> _depnEdges{};
    public:
        explicit WFG(string funcName, unsigned rootLine = 0) : _funcName(move(funcName)), _rootLine(rootLine) {}

        void setNodes(map<unsigned, WFGNode> &&nodes) {
            _nodes = nodes;
        }

        const map<unsigned, WFGNode> &getNodes() const {
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

#endif //WFG_GENERATOR_WFG_H
