//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFO_H
#define WFG_GENERATOR_FUNCINFO_H

#include "MiniCFG.h"
#include "util.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

using namespace std;

namespace wfg {
    class FuncInfo {
    private:
        string _funcName;

        unsigned _startLine;

        unsigned _endLine;

        vector<pair<unsigned, unsigned>> _sensitiveLines{};

        MiniCFG _miniCFG;

        unordered_map<string, vector<unsigned>> _idMap{};

    public:
        FuncInfo(string funcName, unsigned start, unsigned end, MiniCFG &&miniCFG)
                : _funcName(funcName), _startLine(start), _endLine(end), _miniCFG(miniCFG) {}

        string toString() const {
            return "{funcName: " + _funcName + ", startLine: " + to_string(_startLine) + ", endLine: "
                   + to_string(_endLine) + ", sensitiveLines:" + Util::NumPairVecToString(_sensitiveLines)
                   + ", miniCFG: " + _miniCFG.toString() +"}";
        }

        void setSensitiveLines(vector<pair<unsigned, unsigned>> &&sensitiveLines) {
            _sensitiveLines = sensitiveLines;
        }

        void insertIdentifier(const string& id, unsigned lineNo);
    };
}


#endif //WFG_GENERATOR_FUNCINFO_H
