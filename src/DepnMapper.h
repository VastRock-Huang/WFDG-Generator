//
// Created by Unravel on 2022/4/9.
//

#ifndef WFG_GENERATOR_DEPNMAPPER_H
#define WFG_GENERATOR_DEPNMAPPER_H

#include "util.h"
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace wfg {

    class DepnMapper {
    public:
        using VarIdType = int64_t;
        using VarIdPair = pair<VarIdType, VarIdType>;
        using WVarVec = vector<pair<VarIdPair, int>>;
        using RVarVec = vector<pair<int, unsigned>>;
        template<typename T>
        using VarMap = unordered_map<VarIdPair, T, pair_hash>;

    private:
        VarMap<string> _varMap{};
        // 写变量idx->[(依赖的读变量id,读变量idx)...]
        vector<WVarVec> _wVarVec{};
        // 读变量idx->[(依赖的写变量idx, 所在结点)...]
        vector<RVarVec> _rVarVec{};
        // 变量id->(写idx集合, 读idx集合)
        VarMap<pair<unordered_set<int>, unordered_set<int>>> _predMap{};

    public:
        unsigned rVarVecSize() const {
            return _rVarVec.size();
        }

        void pushVar(VarIdPair ids, string name) {
            auto res = _varMap.emplace(move(ids), move(name));
            if (res.second) {
                _predMap.emplace(ids, make_pair(unordered_set<int>(), unordered_set<int>()));
            }
        }

        unsigned pushWVarDetails(const VarIdPair &ids, WVarVec &&vec) {
            unsigned size = _wVarVec.size();
            _predMap.at(ids).first.emplace(size);
            _wVarVec.emplace_back(vec);
            return size;
        }

        unsigned pushRVarDetails(const VarIdPair &ids, RVarVec &&vec) {
            unsigned size = _rVarVec.size();
            _predMap.at(ids).second.emplace(size);
            _rVarVec.emplace_back(vec);
            return size;
        }

        string getVarNameByIds(const VarIdPair &ids) const {
            return _varMap.at(ids);
        }

        template<typename T>
        static string
        varMapToString(const VarMap<T> &vMap, const function<string(decltype(vMap.begin()->second))> &vFunc) {
            return Util::hashmapToString(vMap, Util::numPairToString<VarIdType, VarIdType>, vFunc);
        }

        static string depnPairToString(const pair<unordered_set<int>, unordered_set<int>> &p) {
            auto lbd = [](const unordered_set<int> &s) -> string {
                return Util::hashsetToString(s, Util::numToString<int>);
            };
            return Util::pairToString(p, lbd, lbd);
        }

        static string wPairToString(const pair<VarIdPair, int> &p) {
            return Util::pairToString(p, Util::numPairToString<VarIdType, VarIdType>, Util::numToString<int>);
        }

        string toString() const {
            return "{varMap: " + DepnMapper::varMapToString(_varMap,
                                                            [](const string &s) {
                                                                return s;
                                                            }) +
                   ", wVarVec: " + Util::vecToString(
                    _wVarVec, [](const auto &v) -> string {
                        return Util::vecToString(v, DepnMapper::wPairToString);
                    }) + ", rVarVec: " + Util::vecToString(
                    _rVarVec, [](const auto &v) -> string {
                        return Util::vecToString(v, Util::numPairToString<int, unsigned>);
                    }) + ", predMap: " + DepnMapper::varMapToString(
                    _predMap, DepnMapper::depnPairToString) + "}";
        }
    };
}

#endif //WFG_GENERATOR_DEPNMAPPER_H
