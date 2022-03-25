//
// Created by Unravel on 2022/3/24.
//

#include "WFGGen/WFG.h"
#include "GlobalInstance.h"
#include <set>
#include <map>
#include <queue>
#include <cmath>
#include <iostream>

namespace wfg {
    void genLineWeight(const IdMapper &idMapper, unsigned rootLine, map<unsigned, double> &lineWeightMap) {
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
        const Configuration &Config = GlobalInstance::Config;
        while (!idQue.empty()) {
            // 每一层递减一次权重
            curPredWeight *= Config.weightPredRatio;
            curSuccWeight *= Config.weightSuccRatio;
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

    void
    getWFGNodes(const MiniCFG &miniCFG, const map<unsigned, double> &lineWeightMap, map<unsigned, WFGNode> &wfgNodes) {
        // 遍历所有结点
        int i = 0;
        for (const CFGNode &cfgNode: miniCFG.getNodes()) {
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

    vector<unsigned> findRootNodes(const map<unsigned, WFGNode> &wfgNodes, unsigned rootLine) {
        vector<unsigned> rootNodes{};
        for (auto &nodePair: wfgNodes) {
            if (nodePair.second.markedLines.count(rootLine)) {
                rootNodes.emplace_back(nodePair.first);
            }
        }
        return rootNodes;
    }

    void genNodeWeight(map<unsigned, WFGNode> &wfgNodes, const MiniCFG &miniCFG, const vector<unsigned> &rootNodes) {
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
            miniCFG.for_each_pred(rootNode, predExecution);
            miniCFG.for_each_succ(rootNode, succExecution);
        }

        const Configuration &Config = GlobalInstance::Config;
        for (unsigned depth = 0; depth < Config.graphPredDepth && !predNodeQue.empty(); ++depth) {
            curPredWeight *= Config.weightPredRatio;
            for (auto size = predNodeQue.size(); size > 0; --size) {
                unsigned node = predNodeQue.front();
                predNodeQue.pop();
                auto it = wfgNodes.find(node);
                if (it == wfgNodes.end()) {
                    continue;
                }
                it->second.nodeWeight = curPredWeight;
                miniCFG.for_each_pred(node, predExecution);
            }
        }

        for (unsigned depth = 0; depth < Config.graphSuccDepth && !succNodeQue.empty(); ++depth) {
            curPredWeight *= Config.weightSuccRatio;
            for (auto size = succNodeQue.size(); size > 0; --size) {
                unsigned node = succNodeQue.front();
                succNodeQue.pop();
                auto it = wfgNodes.find(node);
                if (it == wfgNodes.end()) {
                    continue;
                }
                it->second.nodeWeight = curSuccWeight;
                miniCFG.for_each_succ(node, succExecution);
            }
        }
    }

    void buildWFG(WFG &w, map<unsigned, WFGNode> &wfgNodes, const MiniCFG &miniCFG) {
        for (auto it = wfgNodes.begin(); it != wfgNodes.end();) {
            WFGNode &node = it->second;
            if (node.nodeWeight == 0.) {
                it = wfgNodes.erase(it);
            } else {
                node.weight = sqrt(node.lineWeight * node.nodeWeight);
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
            miniCFG.for_each_succ(nodePair.first, insertEdges);
        }
    }

    vector<WFG> genWFGs(const FuncInfo &funcInfo) {
        vector<WFG> wfgs{};
        for (const auto &linePair: funcInfo.getSensitiveLinePairs()) {
            unsigned rootLine = linePair.first;
            map<unsigned, double> lineWeightMap{};
            genLineWeight(funcInfo.getIdMapper(), rootLine, lineWeightMap);
            map<unsigned, WFGNode> wfgNodes{};
            getWFGNodes(funcInfo.getMiniCFG(), lineWeightMap, wfgNodes);
            genNodeWeight(wfgNodes, funcInfo.getMiniCFG(), findRootNodes(wfgNodes, rootLine));
            WFG w(funcInfo.getFuncName(), rootLine);
            buildWFG(w, wfgNodes, funcInfo.getMiniCFG());
            wfgs.push_back(move(w));
        }
        return wfgs;
    }

}
