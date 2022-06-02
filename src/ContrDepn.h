//
// Created by Unravel on 2022/4/17.
//

#ifndef WFG_GENERATOR_CONTRDEPN_H
#define WFG_GENERATOR_CONTRDEPN_H

#include "CustomCPG.h"
#include <vector>
#include <unordered_map>
#include <iostream>

using namespace std;

namespace wfdg {
    //! 控制依赖生成器
    class ContrDepn {
    private:
        const CustomCPG &_customCPG;
        unsigned _size;
        vector<unsigned> _dfn;  //!< dfs序,非0
        vector<unsigned> _dad;  //!< 结点在搜索树中的父结点,1起始
        vector<unsigned> _semi; //!< dfn最小的半支配点,1起始
        vector<unsigned> _parent;   //!< 并查集祖先结点,1起始,非0
        vector<unsigned> _best; //!< 祖先链中dfn值最小的semi,1起始
        vector<unsigned> _id;   //!< 时间戳对应的结点ID,1起始
        vector<unsigned> _idom;     //!< 结点的控制依赖结点
        unsigned _idx{};    //!< 时间戳

    private:
        void _dfs(unsigned curNode) {
            _dfn[curNode] = ++_idx;
            _id[_idx] = curNode;
            for (unsigned vecIdx = _customCPG.succ_begin(curNode);
                 vecIdx != _customCPG.succ_end(curNode); ++vecIdx) {
                unsigned succNode = _customCPG.succ_at(vecIdx);
                if (_dfn[succNode] == 0) {
                    _dfs(succNode);
                    _dad[_dfn[succNode]] = _dfn[curNode];
                }
            }
        }

        unsigned _find(unsigned x) {
            if (x == _parent[x]) {
                return x;
            }
            unsigned y = _find(_parent[x]);
            if (_semi[_best[x]] > _semi[_best[_parent[x]]]) {
                _best[x] = _best[_parent[x]];
            }
            return _parent[x] = y;
        }

        void _LengauerTarjan() {
            vector <vector<unsigned>> dom(_size + 1);
            vector<unsigned> idom(_size + 1); //!< dfn最大支配点,1起始
            for (unsigned y = _idx; y > 1; --y) {
                unsigned x = _dad[y];
                unsigned nodeID = _id[y];
                // 求出半支配点
                for (unsigned vecIdx = _customCPG.pred_begin(nodeID);
                     vecIdx != _customCPG.pred_end(nodeID); ++vecIdx) {
                    unsigned predNode = _customCPG.pred_at(vecIdx);
                    unsigned dfn = _dfn[predNode];
                    if (dfn != 0) {
                        _find(dfn);
                        _semi[y] = min(_semi[y], _semi[_best[dfn]]);//
                    }
                    dom[_semi[y]].emplace_back(y);
                    _parent[y] = x;

                    for (unsigned z: dom[x]) {
                        _find(z);
                        idom[z] = _semi[_best[z]] < x ? _best[z] : x;
                    }
                    dom[x].clear();
                }
                for (unsigned i = 2; i <= _idx; ++i) {
                    if (idom[i] != _semi[i]) {
                        idom[i] = idom[idom[i]];
                    }
                    dom[idom[i]].emplace_back(i);
                }
                idom[1] = 0;
            }
            for (unsigned i = 1; i <= _idx; ++i) {
                _idom[_id[i]] = _id[idom[i]];
            }
        }

        unsigned genDepn(unsigned nodeID, vector<bool> &visited) {
            if (visited[nodeID]) {
                return _idom[nodeID];
            }
            visited[nodeID] = true;
            if (!_customCPG.hasCondition(_idom[nodeID])) {
                _idom[nodeID] = genDepn(_idom[nodeID], visited);
            }
            unsigned vecIdx = _customCPG.pred_begin(nodeID);
            if(vecIdx < _customCPG.pred_size()) {
                unsigned predNode = _customCPG.pred_at(vecIdx);
                if (_customCPG.isLoop(predNode) && _customCPG.succ_back(predNode) == nodeID) {
                    _idom[nodeID] = genDepn(predNode, visited);
                }
            }
            return _idom[nodeID];
        }


    public:
        ContrDepn(const CustomCPG &customCPG, unsigned size)
                : _customCPG(customCPG),
                  _size(size), _dfn(size),
                  _dad(size + 1), _semi(size + 1),
                  _parent(size + 1),
                  _best(size + 1),
                  _id(size + 1),
                  _idom(size) {
            for (unsigned i = 1; i <= _size; ++i) {
                _parent[i] = _semi[i] = _best[i] = i;
            }
        }

        //! 生成结点的控制依赖向量
        //! \return 控制依赖向量idom idom[i]为第i号结点依赖的结点,没有则为0
        vector<unsigned> gen() {
            _dfs(_size - 1);
            _LengauerTarjan();
            unordered_map<unsigned, unsigned> res{};
            vector<bool> visited(_size);
            visited.front() = visited.back() = true;
            _idom.front() = 0;
            for (unsigned i = 1; i < _size - 1; ++i) {
                genDepn(i, visited);
            }
            return _idom;
        }
    };
}


#endif //WFG_GENERATOR_CONTRDEPN_H
