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

namespace wfdg {

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
        static string varMapToString(const VarMap<T> &vMap,
                                     const function<string(decltype(vMap.begin()->second))> &vFunc
                                     = util::numToString<T>) {
            return util::hashmapToString(vMap, util::numPairToString<VarIdType, VarIdType>, vFunc);
        }

        struct LeftData {
            vector<AssignPair> assignFrom;  //!< 所依赖的赋值变量(变量id,变量rightIdx)
            vector<RefPair> refTo{};  //!< 值被引用的位置(变量rightIdx,所在结点id)

            explicit LeftData(vector<AssignPair> &&from) : assignFrom(from) {}

            static string toString(const LeftData &leftData) {
                return "{assignFrom: " +
                       util::vecToString(leftData.assignFrom, DepnMapper::assignPairToString) +
                       ", refTo: " +
                       util::vecToString(leftData.refTo, DepnMapper::refPairToString) + "}";
            }
        };

        struct RightData {
            vector<RefPair> refFrom;    //!< 所引用的值的位置(变量leftIdx,所在结点id)
            AssignPair assignTo{};    //!< 赋值的变量(变量id, 变量leftIdx)

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

        vector<unordered_set<unsigned>> _sensitiveNodes;
        vector<vector<pair<VarIdPair, int>>> _sensitiveWVars;
        vector<vector<pair<VarIdPair, int>>> _sensitiveRVars;

        unordered_map<unsigned, VarMap<int>> _contrVarMap{};

        vector<LeftData> _leftVec{};
        vector<RightData> _rightVec{};

        VarMap<unordered_set<int>> _leftMap{};
        VarMap<unordered_set<int>> _rightMap{};

    public:
        explicit DepnMapper(unsigned sensitiveCnt) :
                _sensitiveNodes(sensitiveCnt), _sensitiveWVars(sensitiveCnt), _sensitiveRVars(sensitiveCnt) {}

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

        int pushAssignInfo(const VarIdPair &ids, const VarMap<int> &assignFrom) {
            int leftIdx = static_cast<int>(_leftVec.size());
            _leftMap[ids].emplace(leftIdx);
            vector<AssignPair> vec{};
            for (const auto &p: assignFrom) {
                _rightVec.at(p.second).assignTo = make_pair(ids, leftIdx);
                vec.emplace_back(p);
            }
//            cout << util::vecToString(vec, assignPairToString) <<'\n';
            _leftVec.emplace_back(move(vec));
            return leftIdx;
        }

        int
        pushRefInfo(const VarIdPair &ids, unsigned curNode, const unordered_set<RefPair, util::pair_hash> &refFrom) {
            int rightIdx = static_cast<int>(_rightVec.size());
            _rightMap[ids].emplace(rightIdx);
            vector<RefPair> vec{};
            for (const RefPair &p: refFrom) {
                _leftVec.at(p.first).refTo.emplace_back(rightIdx, curNode);
                vec.emplace_back(p);
            }
//            cout << util::vecToString(vec, refPairToString) <<'\n';
            _rightVec.emplace_back(move(vec));
            return rightIdx;
        }

        string getVarNameByIds(const VarIdPair &ids) const {
            return _varMap.at(ids);
        }

        void pushSensitiveWVar(int sensitiveIdx, const VarIdPair &ids, int leftIdx, unsigned nodeID) {
            _sensitiveNodes.at(sensitiveIdx).emplace(nodeID);
            _sensitiveWVars.at(sensitiveIdx).emplace_back(ids, leftIdx);
        }

        void pushSensitiveRVar(int sensitiveIdx, const VarIdPair &ids, int rightIdx, unsigned nodeID) {
            _sensitiveNodes.at(sensitiveIdx).emplace(nodeID);
            _sensitiveRVars.at(sensitiveIdx).emplace_back(ids, rightIdx);
        }

        const vector<pair<VarIdPair, int>> &getSensitiveRVars(int sensitiveIdx) const {
            return _sensitiveRVars.at(sensitiveIdx);
        }

        const vector<pair<VarIdPair, int>> &getSensitiveWVars(int sensitiveIdx) const {
            return _sensitiveWVars.at(sensitiveIdx);
        }

        const unordered_set<unsigned> &getSensitiveNodes(int sensitiveIdx) const {
            return _sensitiveNodes.at(sensitiveIdx);
        }

        int getVarLastRightIdx(const VarIdPair& ids) const {
            int resIdx = -1;
            for(int rightIdx: _rightMap.at(ids)) {
                resIdx = max(rightIdx, resIdx);
            }
            return resIdx;
        }

        const LeftData &getLeftData(int leftIdx) const {
            return _leftVec.at(leftIdx);
        }

        const RightData &getRightData(int rightIdx) const {
            return _rightVec.at(rightIdx);
        }

        void pushContrVar(unsigned nodeId, VarIdPair ids, int rightIdx) {
            auto it = _contrVarMap.find(nodeId);
            if (it == _contrVarMap.end()) {
                std::tie(it, std::ignore) = _contrVarMap.emplace(nodeId, VarMap<int>());
            }
            it->second.emplace(move(ids), rightIdx);
        }

        const VarMap<int> *getContrVars(unsigned nodeId) const {
            auto it = _contrVarMap.find(nodeId);
            if (it != _contrVarMap.end()) {
                return &(it->second);
            }
            return nullptr;
        }

        string toString() const;
    };
}

#endif //WFG_GENERATOR_DEPNMAPPER_H
