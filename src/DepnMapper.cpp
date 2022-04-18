//
// Created by Unravel on 2022/4/9.
//

#include "DepnMapper.h"

namespace wfdg {
    string DepnMapper::toString() const {
        auto hashsetToStr = [](const unordered_set<int> &s) -> string {
            return util::hashsetToString(s, util::numToString < int > );
        };
        auto vecToStr = [](const vector<AssignPair> &vec) -> string {
            return util::vecToString(vec, assignPairToString);
        };

        return "{varMap: " +
               varMapToString(_varMap, [](const string &s) -> string {
                   return s;
               }) + ",\n" +
               "sensitiveNodes: " +
               util::vecToString(_sensitiveNodes, [](const auto &hashset) -> string {
                   return util::hashsetToString(hashset);
               }) + ",\n" +
               "sensitiveWVars: " + util::vecToString(_sensitiveWVars, vecToStr) + ",\n" +
               "sensitiveRVars: " + util::vecToString(_sensitiveRVars, vecToStr) + ",\n" +
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