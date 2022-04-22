//
// Created by Unravel on 2022/4/9.
//

#include "DepnMapper.h"

namespace wfdg {
    string DepnMapper::toString() const {
        auto hashsetToStr = [](const unordered_set<int> &s) -> string {
            return util::hashsetToString(s, util::numToString < int > );
        };
        auto varMapToStr = [](const VarMap<pair<int, unsigned>> &vMap) -> string {
            return DepnMapper::varMapToString(vMap, util::numPairToString<int, unsigned>);
        };

        return "{varMap: " +
               varMapToString(_varMap, [](const string &s) -> string {
                   return s;
               }) + ",\n" +
               "sensitiveWVars: " + util::vecToString(_sensitiveWVars, varMapToStr) + ",\n" +
               "sensitiveRVars: " + util::vecToString(_sensitiveRVars, varMapToStr) + ",\n" +
               "contrVarMap: " +
               util::hashmapToString(_contrVarMap, util::numToString<unsigned>,
                                     [](const auto &hashset) -> string {
                                         return DepnMapper::varMapToString(hashset);
                                     }) + ",\n" +
               "leftVec: " + util::vecToString(_leftVec, LeftData::toString) + ",\n" +
               "rightVec: " + util::vecToString(_rightVec, RightData::toString) + ",\n" +
               "leftMap: " + varMapToString(_leftMap, hashsetToStr) + ",\n" +
               "rightMap: " + varMapToString(_rightMap, hashsetToStr) +
               "}";
    }
}