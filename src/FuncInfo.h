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

        pair<unsigned, unsigned> _lineRange;

        vector<pair<unsigned, unsigned>> _sensitiveLines{};

        MiniCFG _miniCFG;

        // 行号向量是默认去重升序的
        unordered_map<string, vector<unsigned>> _idMap{};

    public:
        FuncInfo(string funcName, unsigned start, unsigned end, MiniCFG &&miniCFG)
                : _funcName(funcName), _lineRange(make_pair(start, end)), _miniCFG(miniCFG) {}

        string toString() const {
            return "{funcName: " + _funcName + ", lineRange: " + Util::numPairToString(_lineRange)
                   + ", sensitiveLines:" + Util::numPairVecToString(_sensitiveLines)
                   + ", miniCFG: " + _miniCFG.toString() + ", idMap: " + Util::str_NumVecMapToString(_idMap) + "}";
        }

        void setSensitiveLines(vector<pair<unsigned, unsigned>> &&sensitiveLines) {
            _sensitiveLines = sensitiveLines;
        }

        void insertIdentifier(string &&id, unsigned lineNo);

        pair<unsigned, unsigned> getLineRange() const {
            return _lineRange;
        }

    };
}


#endif //WFG_GENERATOR_FUNCINFO_H
