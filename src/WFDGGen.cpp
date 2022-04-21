//
// Created by Unravel on 2022/3/24.
//

#include "WFDGGen/WFDG.h"
#include "WFDGGen.h"
#include <set>
#include <queue>
#include <cmath>
#include <iostream>

namespace wfdg {
    void WFDGGenerator::_genContrDepnWeight(unordered_map<unsigned, double> &depnWeightMap) {
        const DepnMapper &depnMapper = _customCPG.getDepnMapper();
        double initWeight = 1.;
        auto isSensitiveVar = [this, &depnMapper](const VarIdPair &ids) -> bool {
            return any_of(depnMapper.getSensitiveRVars(_sensitiveIdx).begin(),
                          depnMapper.getSensitiveRVars(_sensitiveIdx).end(),
                          [&ids](const auto &p) { return p.first == ids; })
                   || any_of(depnMapper.getSensitiveWVars(_sensitiveIdx).begin(),
                             depnMapper.getSensitiveWVars(_sensitiveIdx).end(),
                             [&ids](const auto &p) { return p.first == ids; });
        };

        // 记录变量是否涉及过
        VarMap<double> varWeight{};
        double curWeight = initWeight;
        queue<int> idxQueue;
        // 将所有控制依赖的结点中的变量放入队列
        for (unsigned node: depnMapper.getSensitiveNodes(_sensitiveIdx)) {
//            cout << "curNode:" << node << '\n';
            unsigned contrNode = _customCPG.getContrNode(node);
            while (contrNode != 0) {
//                cout << "contrNode:" << contrNode << '\n';
                curWeight *= _config.weightPredRatio;
                const VarMap<int> *vars = depnMapper.getContrVars(contrNode);
                if (!vars) {
                    depnWeightMap.emplace(contrNode, 0.);
                    contrNode = _customCPG.getContrNode(contrNode);
                    continue;
                }
                double maxWeight = 0.;
                for (const auto &p: *vars) {
                    double weight;
                    // 这个变量已经涉及过则直接使用其权重
                    auto it = varWeight.find(p.first);
                    if (it != varWeight.end()) {
                        weight = it->second;
                    } else {
                        weight = isSensitiveVar(p.first) ? initWeight : curWeight;
                        varWeight.emplace(p.first, weight);
                    }
                    maxWeight = max(maxWeight, weight);
                    idxQueue.push(p.second);
//                    cout << "iniRPush:" << p.second << '\n';
                }
                depnWeightMap.emplace(contrNode, maxWeight);
                contrNode = _customCPG.getContrNode(contrNode);
            }
        }
        bool isRight = true;
        unordered_set<int> leftIdxSet{};
        while (!idxQueue.empty()) {
            if (isRight) {
                curWeight *= _config.weightPredRatio;
                for (unsigned size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: rpop: " << to_string(p) << '\n';
                    idxQueue.pop();
                    const RightData &rightData = depnMapper.getRightData(p);
                    for (const RefPair &refPair: rightData.refFrom) {
                        if (depnWeightMap.count(refPair.second) == 0) {
                            depnWeightMap.emplace(refPair.second, curWeight);
                        }
                        if (leftIdxSet.count(refPair.first) == 0) {
                            leftIdxSet.emplace(refPair.first);
//                            cout << "D: lpush: " << to_string(refPair.first) << '\n';
                            idxQueue.emplace(refPair.first);
                        }
                    }
                }
            } else {
                for (unsigned size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: lpop: " << to_string(p) << '\n';
                    idxQueue.pop();
                    const LeftData &leftData = depnMapper.getLeftData(p);
                    for (const AssignPair &assignPair: leftData.assignFrom) {
//                        cout << "D: rpush: " << to_string(assignPair.second) << '\n';
                        idxQueue.emplace(assignPair.second);
                    }
                }
            }
            isRight = !isRight;
        }
    }

    void WFDGGenerator::_genDataDepnWeight(unordered_map<unsigned, double> &depnWeightMap) {
        const DepnMapper &depnMapper = _customCPG.getDepnMapper();
        const unordered_set<unsigned> &sensitiveNodes = depnMapper.getSensitiveNodes(_sensitiveIdx);
        if (sensitiveNodes.empty()) {
            return;
        }

        queue<pair<int, unsigned>> idxQueue{};
        bool isRight = true;
        double initWeight = 1., curWeight = initWeight;

//        unsigned rootNode = *min_element(sensitiveNodes.begin(), sensitiveNodes.end());
        const vector<pair<VarIdPair, int>> &rVars = depnMapper.getSensitiveRVars(_sensitiveIdx);
        if (!rVars.empty()) {
            for (unsigned nodeID: depnMapper.getSensitiveNodes(_sensitiveIdx)) {
                depnWeightMap.emplace(nodeID, initWeight);
                for (const auto &p: rVars) {
                    idxQueue.emplace(p.second, nodeID);
                }
            }
        }
        while (!idxQueue.empty()) {
            if (isRight) {
                curWeight *= _config.weightPredRatio;
                for (unsigned size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: rpop: " << util::numPairToString(p) << '\n';
                    idxQueue.pop();
                    const RightData &rightData = depnMapper.getRightData(p.first);
                    for (const RefPair &refPair: rightData.refFrom) {
                        auto it = depnWeightMap.find(refPair.second);
                        if (it == depnWeightMap.end() || curWeight > it->second) {
                            depnWeightMap[refPair.second] = curWeight;
                        }
                        if (refPair.second > p.second) {
                            _customCPG.addDataDepnEdge(refPair.second, p.second);
                        }
//                        cout << "D: lpush: " << DepnMapper::refPairToString(refPair) <<'\n';
                        idxQueue.emplace(refPair);
                    }
                }
            } else {
                for (unsigned size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: lpop: " << util::numPairToString(p) << '\n';
                    idxQueue.pop();
                    const LeftData &leftData = depnMapper.getLeftData(p.first);
                    for (const AssignPair &assignPair: leftData.assignFrom) {
//                        cout << "D: rpush: " << util::numPairToString(make_pair(assignPair.second, p.second)) <<'\n';
                        idxQueue.emplace(assignPair.second, p.second);
                    }
                }
            }
            isRight = !isRight;
        }

        isRight = false;
        curWeight = initWeight;
        const vector<pair<VarIdPair, int>> &wVars = depnMapper.getSensitiveWVars(_sensitiveIdx);
        if (!wVars.empty()) {
            for (unsigned nodeID: depnMapper.getSensitiveNodes(_sensitiveIdx)) {
                depnWeightMap.emplace(nodeID, initWeight);
                for (const auto &p: wVars) {
                    idxQueue.emplace(p.second, nodeID);
                }
            }
        }

        while (!idxQueue.empty()) {
            if (isRight) {
                for (auto size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: r2pop: " << util::numPairToString(p) << '\n';
                    idxQueue.pop();
                    const RightData &rightData = depnMapper.getRightData(p.first);
//                    cout << "D: l2push: " << util::numPairToString(make_pair(rightData.assignTo.second, p.second)) <<'\n';
                    if (DepnMapper::isVar(rightData.assignTo.first)) {
                        idxQueue.emplace(rightData.assignTo.second, p.second);
                    }
                }
            } else {
                curWeight *= _config.weightSuccRatio;
                for (auto size = idxQueue.size(); size != 0; --size) {
                    const auto p = idxQueue.front();
//                    cout << "D: l2pop: " << util::numPairToString(p) << '\n';
                    idxQueue.pop();
                    const LeftData &leftData = depnMapper.getLeftData(p.first);
                    for (const RefPair &refPair: leftData.refTo) {
                        auto it = depnWeightMap.find(refPair.second);
                        if (it == depnWeightMap.end() || curWeight > it->second) {
                            depnWeightMap[refPair.second] = curWeight;
                        }
                        if (refPair.second < p.second) {
                            _customCPG.addDataDepnEdge(p.second, refPair.second);
                        }
//                        cout << "D: r2push: " << DepnMapper::refPairToString(refPair) <<'\n';
                        idxQueue.emplace(refPair);
                    }
                }
            }
            isRight = !isRight;
        }
    }

    void
    WFDGGenerator::_genNodeWeight(const unordered_map<unsigned, double> &depnWeightMap,
                                  unordered_map<unsigned, double> &nodeWeightMap) const {
        if (depnWeightMap.empty()) {
            return;
        }

        queue<unsigned> predNodeQue{};
        queue<unsigned> succNodeQue{};
        unordered_set<unsigned> nodeSet{};
        double initWeight = 1., curPredWeight = initWeight, curSuccWeight = initWeight;

        auto predExecution = [&depnWeightMap, &predNodeQue](unsigned predNode,
                                                            unsigned curNode) -> void {
            if (depnWeightMap.count(predNode) == 0 || predNode < curNode) {
                return;
            }
//            cout << "cur:" << curNode << ", push:" << predNode << '\n';
            predNodeQue.emplace(predNode);
        };

        auto succExecution = [&depnWeightMap, &nodeSet, &succNodeQue](unsigned succNode, unsigned curNode) -> void {
            if (depnWeightMap.count(succNode) == 0 || succNode > curNode || nodeSet.count(succNode) != 0) {
                return;
            }
            nodeSet.emplace(succNode);
            succNodeQue.emplace(succNode);
        };

        auto addPredDepn = [this, &predNodeQue, &depnWeightMap](unsigned predSize, unsigned nodeId) -> void {
            if (predSize != predNodeQue.size()) {
                return;
            }
            unsigned contrNode = _customCPG.getContrNode(nodeId);
            while (contrNode != 0 && depnWeightMap.count(contrNode) == 0) {
                contrNode = _customCPG.getContrNode(contrNode);
            }
            if (contrNode != 0) {
//                cout << "excur:" << nodeId << ", push:" << contrNode << '\n';
                predNodeQue.emplace(contrNode);
            }
        };

        for (unsigned rootNode: _customCPG.getDepnMapper().getSensitiveNodes(_sensitiveIdx)) {
            nodeSet.emplace(rootNode);
            nodeWeightMap[rootNode] = initWeight;
            unsigned preSize = predNodeQue.size();
            _customCPG.for_each_pred(rootNode, predExecution);
            // 没有结点被添加即其所有前驱结点都不依赖,则添加其依赖结点
            addPredDepn(preSize, rootNode);
            _customCPG.for_each_succ(rootNode, succExecution);
        }

        for (unsigned depth = 0; depth < _config.graphPredDepth && !predNodeQue.empty(); ++depth) {
            curPredWeight *= _config.weightPredRatio;
            for (auto size = predNodeQue.size(); size > 0; --size) {
                unsigned nodeID = predNodeQue.front();
                predNodeQue.pop();
                if (nodeSet.count(nodeID) != 0) {
                    continue;
                }
                nodeSet.emplace(nodeID);
                nodeWeightMap.emplace(nodeID, curPredWeight);
                unsigned predSize = predNodeQue.size();
                _customCPG.for_each_pred(nodeID, predExecution);
                addPredDepn(predSize, nodeID);
            }
        }

        for (unsigned depth = 0; depth < _config.graphSuccDepth && !succNodeQue.empty(); ++depth) {
            curSuccWeight *= _config.weightSuccRatio;
            for (auto size = succNodeQue.size(); size > 0; --size) {
                unsigned nodeID = succNodeQue.front();
                succNodeQue.pop();
                nodeWeightMap.emplace(nodeID, curSuccWeight);
                _customCPG.for_each_succ(nodeID, succExecution);
            }
        }
    }

    WFDG WFDGGenerator::_buildWFDG(unsigned int rootLine, const unordered_map<unsigned, double> &depnWeightMap,
                                   const unordered_map<unsigned, double> &nodeWeightMap) const {
        WFDG w(_customCPG.getFuncName(), rootLine);
        for (auto p: nodeWeightMap) {
            WFDGNode node{};
            node.id = p.first;
            node.stmtVec = _customCPG.getStmtVec(node.id);
//            cout << "node:" << node.id << '\n';
            node.depnWeight = depnWeightMap.at(node.id);
//            cout << "||\n";
            node.nodeWeight = p.second;
            node.weight = _config.useWeight ? sqrt(node.depnWeight * node.nodeWeight) : 0.;
            w.addNode(p.first, move(node));
        }

        auto insertEdges = [&w,&nodeWeightMap](unsigned succNode, unsigned curNode) -> void {
            if (nodeWeightMap.count(succNode) != 0) {
                w.addEdge(succNode, curNode);
            }
        };
        for (auto &p: w.getNodes()) {
            _customCPG.for_each_succ(p.first, insertEdges);
        }
        for(const pair<unsigned, unsigned> &edge: _customCPG.getDataDepnEdges()) {
            if(nodeWeightMap.count(edge.first) != 0 && nodeWeightMap.count(edge.second) != 0) {
                w.addDepnEdge(edge);
            }
        }
        return w;
    }

    void WFDGGenerator::genWFDGs(vector<WFDG> &wfdgs) {
        if (_customCPG.getSensitiveLineMap().empty()) {
            _genWFDGWithoutSensitiveLine(wfdgs);
            return;
        }
        const map<unsigned, int> &sensitiveLineMap = _customCPG.getSensitiveLineMap();
        for (const auto &p: sensitiveLineMap) {
//            cout << "sensitiveLine:" << p.first << '\n';
//            cout << "sensitiveNode:" << util::hashsetToString(_customCPG.getDepnMapper().getSensitiveNodes(p.second))
//                 << '\n';
            _setSensitiveIdx(p.second);

            unordered_map<unsigned, double> depnWeightMap{};
            _genContrDepnWeight(depnWeightMap);
            _genDataDepnWeight(depnWeightMap);
//            cout << util::hashmapToString(depnWeightMap, util::numToString<unsigned>, util::numToString<double>)
//                 << '\n';
            if (depnWeightMap.empty()) {
                continue;
            }
            unordered_map<unsigned, double> nodeWeightMap{};
            _genNodeWeight(depnWeightMap, nodeWeightMap);
//            cout << util::hashmapToString(nodeWeightMap, util::numToString<unsigned>, util::numToString<double>)
//                 << '\n';
            wfdgs.emplace_back(_buildWFDG(p.first, depnWeightMap, nodeWeightMap));
        }
    }

    void WFDGGenerator::_genWFDGWithoutSensitiveLine(vector<WFDG> &wfdgs) {
        WFDG w(_customCPG.getFuncName(), 0U);
        for (unsigned i = 0; i < _customCPG.getNodeCnt(); ++i) {
            WFDGNode node{};
            node.id = i;
            node.stmtVec = _customCPG.getStmtVec(i);
            node.weight = _config.useWeight ? 1 : 0;
            w.addNode(i, move(node));
        }
        auto insertEdges = [&w](unsigned succNode, unsigned curNode) -> void {
            w.addEdge(succNode, curNode);
        };
        for (auto &nodePair: w.getNodes()) {
            _customCPG.for_each_succ(nodePair.first, insertEdges);
        }
        w.setDepnEdges(_customCPG.getDataDepnEdges());
        wfdgs.push_back(move(w));
    }


}
