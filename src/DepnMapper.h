//
// Created by Unravel on 2022/4/9.
//

#ifndef WFG_GENERATOR_DEPNMAPPER_H
#define WFG_GENERATOR_DEPNMAPPER_H

#include "util.h"
#include <utility>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace wfg {

    class DepnMapper {
    public:
        using VarIdType = int64_t;
        using VarIdPair = pair<VarIdType, VarIdType>;
        // (变量Idx,变量所在结点id)
        using RefPair = pair<int, unsigned>;
        // (变量id,变量Idx)
        using AssignPair = pair<VarIdPair, int>;
        template<typename T>
        using VarMap = unordered_map<VarIdPair, T, util::pair_hash>;

        static bool isVar(const VarIdPair &idPair) {
            return idPair.second != 0;
        }

        static string varIdPairToString(const VarIdPair &idPair) {
            return util::numPairToString(idPair);
        }

        static string refPairToString(const RefPair &refPair) {
            return util::numPairToString(refPair);
        }

        static string assignPairToString(const AssignPair &assignPair) {
            return util::pairToString(assignPair, varIdPairToString, util::numToString<int>);
        }

        template<typename T>
        static string
        varMapToString(const VarMap<T> &vMap, const function<string(decltype(vMap.begin()->second))> &vFunc) {
            return util::hashmapToString(vMap, util::numPairToString<VarIdType, VarIdType>, vFunc);
        }

        struct LeftData {
            vector<AssignPair> assignFrom;  //! 所依赖的赋值变量(变量id,变量rightIdx)
            vector<RefPair> refTo{};  //! 值被引用的位置(变量rightIdx,所在结点id)

            explicit LeftData(vector<AssignPair> &&from) : assignFrom(from) {}

            static string toString(const LeftData &leftData) {
                return "{assignFrom: " +
                       util::vecToString(leftData.assignFrom, DepnMapper::assignPairToString) +
                       ", refTo: " +
                       util::vecToString(leftData.refTo, DepnMapper::refPairToString) + "}";
            }
        };

        struct RightData {
            vector<RefPair> refFrom;    //! 所引用的值的位置(变量leftIdx,所在结点id)
            AssignPair assignTo{};    //! 赋值的变量(变量id, 变量leftIdx)

            explicit RightData(vector<RefPair> &&from) : refFrom(from) {}

            static string toString(const RightData &rightData) {
                return "{refFrom: " +
                       util::vecToString(rightData.refFrom, DepnMapper::refPairToString) +
                       ", assignTo: " +
                       (isVar(rightData.assignTo.first) ?
                        DepnMapper::assignPairToString(rightData.assignTo) : "()")
                       + "}";
            }
        };

    private:
        VarMap<string> _varMap{};

        VarMap<unordered_set<int>> _leftMap{};
        vector<LeftData> _leftVec{};

        VarMap<unordered_set<int>> _rightMap{};
        vector<RightData> _rightVec{};

    public:
        unsigned rightVecSize() const {
            return _rightVec.size();
        }

        void pushVar(VarIdPair ids, string name) {
            auto res = _varMap.emplace(move(ids), move(name));
            if (res.second) {
                _rightMap.emplace(ids, unordered_set<int>());
                _leftMap.emplace(ids, unordered_set<int>());
            }
        }

        int pushAssignInfo(const VarIdPair &ids, vector<AssignPair> &&assignFrom) {
            int leftIdx = static_cast<int>(_leftVec.size());
            _leftMap.at(ids).emplace(leftIdx);
            for (const AssignPair &p: assignFrom) {
                _rightVec.at(p.second).assignTo = make_pair(ids, leftIdx);
            }
            _leftVec.emplace_back(move(assignFrom));
            return leftIdx;
        }

        int pushRefInfo(const VarIdPair &ids, unsigned curNode, vector<RefPair> &&refFrom) {
            int rightIdx = static_cast<int>(_rightVec.size());
            _rightMap.at(ids).emplace(rightIdx);
            for (const RefPair &p: refFrom) {
                _leftVec.at(p.first).refTo.emplace_back(rightIdx, curNode);
            }
            _rightVec.emplace_back(move(refFrom));
            return rightIdx;
        }

        string getVarNameByIds(const VarIdPair &ids) const {
            return _varMap.at(ids);
        }

        string toString() const {
            auto hashsetToStr = [](const unordered_set<int> &s) -> string {
                return util::hashsetToString(s, util::numToString < int > );
            };
            return "{varMap: " +
                   varMapToString(_varMap, [](const string &s) -> string {
                       return s;
                   }) + ",\n" +
                   "leftVec: " + util::vecToString(_leftVec, LeftData::toString) + ",\n" +
                   "rightVec: " + util::vecToString(_rightVec, RightData::toString) + ",\n" +
                   "leftMap: " + varMapToString(_leftMap, hashsetToStr) + ",\n" +
                   "rightMap: " + varMapToString(_rightMap, hashsetToStr) + "\n" +
                   "}";
        }
    };
}

#endif //WFG_GENERATOR_DEPNMAPPER_H
