//
// Created by Unravel on 2022/3/24.
//

#include "WFGGen/WFG.h"
#include "WFGGen.h"
#include <set>
#include <map>
#include <queue>
#include <cmath>

namespace wfg {
    void WFGGenerator::_genLineWeight(unsigned rootLine, map<unsigned, double> &lineWeightMap) {
        const IdMapper &idMapper = _funcInfo.getIdMapper();
        queue<const string *> idQue{};
        set<const string *> idSet{};
        double initWeight = 1., curPredWeight = initWeight, curSuccWeight = initWeight;
        // 设置敏感行权重
        lineWeightMap.emplace(rootLine, initWeight);
        // 遍历敏感行中的标识符并添加到队列
        auto it = idMapper.lineMap.find(rootLine);
        // 当clang解析有错误时(比如提供的头文件不全时)记录的变量可能有误,会导致在此返回
        if (it == idMapper.lineMap.end()) {
            return;
        }
        for (auto &id: it->second) {
            idQue.emplace(id);
            idSet.emplace(id);
        }
        // BFS 设置行权重
        while (!idQue.empty()) {
            // 每一层递减一次权重
            curPredWeight *= _config.weightPredRatio;
            curSuccWeight *= _config.weightSuccRatio;
            for (auto size = idQue.size(); size != 0; --size) {
                const string *id = idQue.front();
                idQue.pop();
                // 当前标识符所在行
                const vector<unsigned> &lineVec = idMapper.idMap.at(id);
                // 遍历当前标识符所在的每一行
                for (unsigned line: lineVec) {
                    // 当前行已经设置权重则跳过
                    if (lineWeightMap.count(line) != 0) {
                        continue;
                    }
                    // 设置当前行权重
                    lineWeightMap[line] = line < rootLine ? curPredWeight : curSuccWeight;
                    // 将当前中未加到
                    for (const string *newId: idMapper.lineMap.at(line)) {
                        if (idSet.count(newId) == 0) {
                            idQue.emplace(newId);
                            idSet.emplace(newId);
                        }
                    }
                }
            }
        }
    }

    void WFGGenerator::_getWFGNodes(const map<unsigned, double> &lineWeightMap, map<unsigned, WFGNode> &wfgNodes) {
        // 遍历所有结点
        int i = 0;
        for (const CustomCFG::CFGNode &cfgNode: _customCFG.getNodes()) {
            set<unsigned> markedLines{};
            double lineWeight = 0.;
            // 遍历所有标记行
            for (auto &lineWeightPair: lineWeightMap) {
                // 遍历结点的所有区间
                for (auto &lineRange: cfgNode.lineRanges) {
                    int pos = Util::numInRange(lineWeightPair.first, lineRange);
                    if (pos == 0) {
                        markedLines.emplace(lineWeightPair.first);
                        lineWeight = max(lineWeight, lineWeightPair.second);
                        break;
                    } else if (pos < 0) {
                        // 标记行在当前区间的左侧,则其不可能在后续的范围区间
                        break;
                    }
                }
            }
            if (markedLines.empty()) {
                continue;
            }
            WFGNode wfgNode{};
            wfgNode.id = i;
            wfgNode.stmtVec = cfgNode.stmtVec;
            wfgNode.lineWeight = lineWeight;
            wfgNode.markedLines = move(markedLines);
            wfgNodes.emplace(i, move(wfgNode));
            ++i;
        }
    }

    vector<unsigned> WFGGenerator::findRootNodes(const map<unsigned, WFGNode> &wfgNodes, unsigned rootLine) {
        vector<unsigned> rootNodes{};
        for (auto &nodePair: wfgNodes) {
            if (nodePair.second.markedLines.count(rootLine)) {
                rootNodes.emplace_back(nodePair.first);
            }
        }
        return rootNodes;
    }

    void WFGGenerator::_genNodeWeight(map<unsigned, WFGNode> &wfgNodes, const vector<unsigned> &rootNodes) {
//        unsigned rootNode = rootNodes[0];
        queue<unsigned> predNodeQue{};
        queue<unsigned> succNodeQue{};
        set<unsigned> nodeSet{};
        double initWeight = 1., curPredWeight = initWeight, curSuccWeight = initWeight;

        auto predExecution = [&nodeSet, &predNodeQue](unsigned predNode, unsigned curNode) -> void {
            if (nodeSet.count(predNode) != 0 || predNode < curNode) {
                return;
            }
            nodeSet.emplace(predNode);
            predNodeQue.emplace(predNode);
        };

        auto succExecution = [&nodeSet, &succNodeQue](unsigned succNode, unsigned curNode) -> void {
            if (nodeSet.count(succNode) != 0 || succNode > curNode) {
                return;
            }
            nodeSet.emplace(succNode);
            succNodeQue.emplace(succNode);
        };

        for (unsigned rootNode: rootNodes) {
            wfgNodes[rootNode].nodeWeight = 1;
            _customCFG.for_each_pred(rootNode, predExecution);
            _customCFG.for_each_succ(rootNode, succExecution);
        }

        for (unsigned depth = 0; depth < _config.graphPredDepth && !predNodeQue.empty(); ++depth) {
            curPredWeight *= _config.weightPredRatio;
            for (auto size = predNodeQue.size(); size > 0; --size) {
                unsigned node = predNodeQue.front();
                predNodeQue.pop();
                auto it = wfgNodes.find(node);
                if (it == wfgNodes.end()) {
                    continue;
                }
                it->second.nodeWeight = curPredWeight;
                _customCFG.for_each_pred(node, predExecution);
            }
        }

        for (unsigned depth = 0; depth < _config.graphSuccDepth && !succNodeQue.empty(); ++depth) {
            curPredWeight *= _config.weightSuccRatio;
            for (auto size = succNodeQue.size(); size > 0; --size) {
                unsigned node = succNodeQue.front();
                succNodeQue.pop();
                auto it = wfgNodes.find(node);
                if (it == wfgNodes.end()) {
                    continue;
                }
                it->second.nodeWeight = curSuccWeight;
                _customCFG.for_each_succ(node, succExecution);
            }
        }
    }

    WFG WFGGenerator::_buildWFG(map<unsigned, WFGNode> &wfgNodes, unsigned rootLine) {
        WFG w(_funcInfo.getFuncName(), rootLine);
        for (auto it = wfgNodes.begin(); it != wfgNodes.end();) {
            WFGNode &node = it->second;
            if (node.nodeWeight == 0.) {
                it = wfgNodes.erase(it);
            } else {
                node.weight = _config.useWeight ? sqrt(node.lineWeight * node.nodeWeight) : 0.;
                ++it;
            }
        }
        w.setNodes(move(wfgNodes));
        auto insertEdges = [&w](unsigned succNode, unsigned curNode) {
            if (w.getNodes().count(succNode) != 0) {
                w.addEdge(curNode, succNode);
            }
        };
        for (auto &nodePair: w.getNodes()) {
            _customCFG.for_each_succ(nodePair.first, insertEdges);
        }
        return w;
    }

    vector<WFG> WFGGenerator::genWFGs() {
        if (_funcInfo.getSensitiveLinePairs().empty()) {
            return {_genWFGWithoutSensitiveLine()};
        }

        vector<WFG> wfgs{};
        for (const auto &linePair: _funcInfo.getSensitiveLinePairs()) {
            unsigned rootLine = linePair.first;
            map<unsigned, double> lineWeightMap{};
            _genLineWeight(rootLine, lineWeightMap);
            map<unsigned, WFGNode> wfgNodes{};
            _getWFGNodes(lineWeightMap, wfgNodes);
            _genNodeWeight(wfgNodes, findRootNodes(wfgNodes, rootLine));
            WFG w = _buildWFG(wfgNodes, rootLine);
            wfgs.push_back(move(w));
        }
        return wfgs;
    }

    WFG WFGGenerator::_genWFGWithoutSensitiveLine() {
        map<unsigned, WFGNode> wfgNodes{};
        int i = 0;
        for (const CustomCFG::CFGNode &cfgNode: _customCFG.getNodes()) {
            WFGNode wfgNode{};
            wfgNode.id = i;
            wfgNode.stmtVec = cfgNode.stmtVec;
            wfgNode.weight = _config.useWeight ? 1 : 0;
            wfgNodes.emplace(i, move(wfgNode));
            ++i;
        }
        WFG w(_funcInfo.getFuncName());
        w.setNodes(move(wfgNodes));
        auto insertEdges = [&w](unsigned succNode, unsigned curNode) {
            w.addEdge(curNode, succNode);
        };
        for (auto &nodePair: w.getNodes()) {
            _customCFG.for_each_succ(nodePair.first, insertEdges);
        }
        return w;
    }

}
