//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFO_H
#define WFG_GENERATOR_FUNCINFO_H

#include "CustomCPG.h"
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
            return util::vecToString(vec, getIdName);
        }

        static string numVecToString(const vector<unsigned> &vec) {
            return util::vecToString(vec, util::numToString<unsigned>);
        }

        string toString() const {
            return "{idMap: " + util::hashmapToString(idMap, getIdName, numVecToString) + ", lineMap: "
                   + util::hashmapToString(lineMap, util::numToString<unsigned>, vecToString) + "}";
        }
    };


    class FuncInfo {
    private:
        string _funcName;

        pair<unsigned, unsigned> _lineRange;

        vector<pair<unsigned, unsigned>> _sensitiveLines{};

        CustomCPG _customCPG;

        IdMapper _idMapper{};

    public:
        FuncInfo(string funcName, pair<unsigned,unsigned> lineRange, const unordered_map<string, unsigned>& ASTStmtKindMap)
                : _funcName(std::move(funcName)), _lineRange(move(lineRange)), _customCPG(ASTStmtKindMap) {}

        string toString() const {
            return "{funcName: " + _funcName + ", lineRange: " + util::numPairToString(_lineRange)
                   + ", customCPG: " + _customCPG.toString() + ", idMapper:" + _idMapper.toString() + "}";
        }

        void insertIdentifier(const string &id, unsigned lineNo);

        pair<unsigned, unsigned> getLineRange() const {
            return _lineRange;
        }

        const IdMapper& getIdMapper() const {
            return _idMapper;
        }

        CustomCPG& getCustomCPG() {
            return _customCPG;
        }

        const CustomCPG& getCustomCPG() const {
            return _customCPG;
        }

        string getFuncName() const {
            return _funcName;
        }
    };
}


#endif //WFG_GENERATOR_FUNCINFO_H
