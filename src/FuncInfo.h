//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFO_H
#define WFG_GENERATOR_FUNCINFO_H

#include "CustomCFG.h"
#include "util.h"
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace wfg {
    struct IdMapper {
        unordered_set<string> idSet{};
        // 行号向量是默认去重升序的
        unordered_map<const string *, vector<unsigned>> idMap{};
        unordered_map<unsigned, vector<const string *>> lineMap{};

        static string getIdName(const string *idAddr) {
            return *idAddr;
        }

        static string vecToString(const vector<const string *> &vec) {
            return Util::vecToString(vec, getIdName);
        }

        static string numVecToString(const vector<unsigned> &vec) {
            return Util::vecToString(vec, Util::numToString<unsigned>);
        }

        string toString() const {
            return "{idMap: " + Util::hashmapToString(idMap, getIdName, numVecToString) + ", lineMap: "
                   + Util::hashmapToString(lineMap, Util::numToString<unsigned>, vecToString) + "}";
        }
    };


    class FuncInfo {
    private:
        string _funcName;

        pair<unsigned, unsigned> _lineRange;

        vector<pair<unsigned, unsigned>> _sensitiveLines{};

        CustomCFG _customCFG{};

        IdMapper _idMapper{};

    public:
        FuncInfo(string funcName, pair<unsigned,unsigned> lineRange)
                : _funcName(std::move(funcName)), _lineRange(move(lineRange)) {}

        string toString() const {
            return "{funcName: " + _funcName + ", lineRange: " + Util::numPairToString(_lineRange)
                   + ", sensitiveLines:" + Util::vecToString(_sensitiveLines, Util::numPairToString<unsigned, unsigned>)
                   + ", customCFG: " + _customCFG.toString() + ", idMapper:" + _idMapper.toString() + "}";
        }

        void setSensitiveLines(vector<pair<unsigned, unsigned>> &&sensitiveLines) {
            _sensitiveLines = sensitiveLines;
        }

        void insertIdentifier(const string &id, unsigned lineNo);

        pair<unsigned, unsigned> getLineRange() const {
            return _lineRange;
        }

        const vector<pair<unsigned ,unsigned >>& getSensitiveLinePairs() const {
            return _sensitiveLines;
        }

        const IdMapper& getIdMapper() const {
            return _idMapper;
        }

        CustomCFG& getCFG() {
            return _customCFG;
        }

        const CustomCFG& getCFG() const {
            return _customCFG;
        }

        string getFuncName() const {
            return _funcName;
        }
    };
}


#endif //WFG_GENERATOR_FUNCINFO_H
