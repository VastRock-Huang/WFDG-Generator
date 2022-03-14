//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFO_H
#define WFG_GENERATOR_FUNCINFO_H

#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace wfg {
    class FuncInfo {
    private:
        string _funcName;
        unsigned _startLine;
        unsigned _endLine;
        vector<pair<unsigned, unsigned>> _sensitiveLines{};

    public:
        FuncInfo(string funcName, unsigned start, unsigned end)
                : _funcName(funcName), _startLine(start), _endLine(end) {}

        string toString() const {
            string str = "{funcName: " + _funcName + ", startLine: " + to_string(_startLine) + ", endLine: "
                         + to_string(_endLine) + ", sensitiveLines: [";
            int sz = _sensitiveLines.size();
            for (int i = 0; i < sz; ++i) {
                str += "(" + to_string(_sensitiveLines[i].first) + ", " + to_string(_sensitiveLines[i].second) + ")";
                if (i != sz - 1) {
                    str += ", ";
                }
            }
            str += "]}";
            return str;
        }

        void setSensitiveLines(vector<pair<unsigned, unsigned>> &&sensitiveLines) {
            _sensitiveLines = sensitiveLines;
        }
    };
}


#endif //WFG_GENERATOR_FUNCINFO_H
