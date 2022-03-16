//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_WFG_H
#define WFG_GENERATOR_WFG_H

#include <string>
#include <set>
#include <vector>
#include <utility>

using namespace std;

class WFG {
private:
    struct Node {
        unsigned _id;
        vector<unsigned> _markedLines;
        unsigned _lineWeight;
        unsigned _nodeWeight;
        unsigned _weight;
        vector<unsigned> _ASTStmtVec;
    };

    string _funcName;
    set<Node> _nodes;
    vector<pair<unsigned,unsigned>> _edges;
};

#endif //WFG_GENERATOR_WFG_H
