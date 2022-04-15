//
// Created by Unravel on 2022/3/24.
//

#include "WFDGGen/WFDG.h"
#include "WFGGen.h"
#include <set>
#include <queue>
#include <cmath>
#include <iostream>

namespace wfg {
    void WFDGGenerator::_genLineWeight(unsigned sensitiveIdx, unordered_map<unsigned, double> &lineWeightMap) {
        const DepnMapper &depnMapper = _customCPG.getDepnMapper();
        queue<pair<int, unsigned>> idxQueue{};
        bool isRight = true;
        double initWeight = 1., curWeight = initWeight;

        const vector<pair<VarIdPair, int>> &rVars = depnMapper.getSensitiveRVars(sensitiveIdx);
        if (!rVars.empty()) {
            for (unsigned nodeID: depnMapper.getSensitiveNodes(sensitiveIdx)) {
                lineWeightMap.emplace(nodeID, initWeight);
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
                        if (lineWeightMap.count(refPair.second) == 0) {
                            lineWeightMap.emplace(refPair.second, curWeight);
                        }
                        if (refPair.second > p.second) {
                            _customCPG.addDepnEdge(refPair.second, p.second);
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
//                        cout << "D: lpush: " << util::numPairToString(make_pair(assignPair.second, p.second)) <<'\n';
                        idxQueue.emplace(assignPair.second, p.second);
                    }
                }
            }
            isRight = !isRight;
        }

        isRight = false;
        curWeight = initWeight;
        const vector<pair<VarIdPair, int>> &wVars = depnMapper.getSensitiveWVars(sensitiveIdx);
        if (!wVars.empty()) {
            for (unsigned nodeID: depnMapper.getSensitiveNodes(sensitiveIdx)) {
                lineWeightMap.emplace(nodeID, initWeight);
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
//                    cout << "D: r2push: " << util::numPairToString(make_pair(rightData.assignTo.second, p.second)) <<'\n';
                    if(DepnMapper::isVar(rightData.assignTo.first)) {
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
                        if (lineWeightMap.count(refPair.second) == 0
                            || curWeight > lineWeightMap.at(refPair.second)) {
                            lineWeightMap[refPair.second] = curWeight;
                        }
                        if(refPair.second < p.second) {
                            _customCPG.addDepnEdge(p.second, refPair.second);
                        }
//                        cout << "D: l2push: " << DepnMapper::refPairToString(refPair) <<'\n';
                        idxQueue.emplace(refPair);
                    }
                }
            }
            isRight = !isRight;
        }
    }

    void
    WFDGGenerator::_genNodeWeight(unsigned int sensitiveIdx, const unordered_map<unsigned, double> &lineWeightMap,
                                  unordered_map<unsigned, double> &nodeWeightMap) const {
        queue<unsigned> predNodeQue{};
        queue<unsigned> succNodeQue{};
        unordered_set<unsigned> nodeSet{};
        double initWeight = 1., curPredWeight = initWeight, curSuccWeight = initWeight;

        auto predExecution = [&lineWeightMap, &nodeSet, &predNodeQue](unsigned predNode,
                                                                      unsigned curNode) -> void {
            if (lineWeightMap.count(predNode) == 0 || predNode < curNode || nodeSet.count(predNode) != 0) {
                return;
            }
            nodeSet.emplace(predNode);
            predNodeQue.emplace(predNode);
        };

        auto succExecution = [&lineWeightMap, &nodeSet, &succNodeQue](unsigned succNode, unsigned curNode) -> void {
            if (lineWeightMap.count(succNode) == 0 || succNode > curNode || nodeSet.count(succNode) != 0) {
                return;
            }
            nodeSet.emplace(succNode);
            succNodeQue.emplace(succNode);
        };

        for (unsigned rootNode: _customCPG.getDepnMapper().getSensitiveNodes(sensitiveIdx)) {
            nodeWeightMap[rootNode] = initWeight;
            nodeSet.emplace(rootNode);
            _customCPG.for_each_pred(rootNode, predExecution);
            _customCPG.for_each_succ(rootNode, succExecution);
        }

        for (unsigned depth = 0; depth < _config.graphPredDepth && !predNodeQue.empty(); ++depth) {
            curPredWeight *= _config.weightPredRatio;
            for (auto size = predNodeQue.size(); size > 0; --size) {
                unsigned nodeID = predNodeQue.front();
                predNodeQue.pop();
                nodeWeightMap.emplace(nodeID, curPredWeight);
                _customCPG.for_each_pred(nodeID, predExecution);
            }
        }

        for (unsigned depth = 0; depth < _config.graphSuccDepth && !succNodeQue.empty(); ++depth) {
            curPredWeight *= _config.weightSuccRatio;
            for (auto size = succNodeQue.size(); size > 0; --size) {
                unsigned nodeID = succNodeQue.front();
                succNodeQue.pop();
                nodeWeightMap.emplace(nodeID, curSuccWeight);
                _customCPG.for_each_succ(nodeID, succExecution);
            }
        }
    }

    WFDG WFDGGenerator::_buildWFDG(unsigned int rootLine, const unordered_map<unsigned, double> &lineWeightMap,
                                   const unordered_map<unsigned, double> &nodeWeightMap) const {
        WFDG w(_funcInfo.getFuncName(), rootLine);
        for (auto p: nodeWeightMap) {
            WFDGNode node{};
            node.id = p.first;
            node.stmtVec = _customCPG.getNodes().at(node.id).stmtVec;
            node.lineWeight = lineWeightMap.at(node.id);
            node.nodeWeight = p.second;
            node.weight = _config.useWeight ? sqrt(node.lineWeight * node.nodeWeight) : 0.;
            w.addNode(p.first, move(node));
        }

        auto insertEdges = [&w](unsigned succNode, unsigned curNode) -> void {
            if (w.getNodes().count(succNode) != 0) {
                w.addEdge(curNode, succNode);
            }
        };
        for (auto &p: w.getNodes()) {
            _customCPG.for_each_succ(p.first, insertEdges);
        }
        w.setDepnEdges(_customCPG.getDepnEdges());
        cout << w.toString() << '\n';
        return w;
    }

    vector<WFDG> WFDGGenerator::genWFDGs() {
        vector<WFDG> wfdgs{};
        if (_customCPG.getSensitiveLinePairs().empty()) {
            _genWFDGWithoutSensitiveLine(wfdgs);
            return wfdgs;
        }
        const vector<pair<unsigned, unsigned>> &sensitiveLinePairs = _customCPG.getSensitiveLinePairs();
        for (unsigned i = 0; i < sensitiveLinePairs.size(); ++i) {
            unordered_map<unsigned, double> lineWeightMap{};
            _genLineWeight(i, lineWeightMap);
            unordered_map<unsigned, double> nodeWeightMap{};
            _genNodeWeight(i, lineWeightMap, nodeWeightMap);
            WFDG w = _buildWFDG(sensitiveLinePairs.at(i).first, lineWeightMap, nodeWeightMap);
            cout << w.toString() <<'\n';
            wfdgs.emplace_back(move(w));
        }
        return wfdgs;
    }

    void WFDGGenerator::_genWFDGWithoutSensitiveLine(vector<WFDG> &wfdgs) {
        int i = 0;
        WFDG w(_funcInfo.getFuncName());
        for (const CustomCPG::CPGNode &cfgNode: _customCPG.getNodes()) {
            WFDGNode node{};
            node.id = i;
            node.stmtVec = cfgNode.stmtVec;
            node.weight = _config.useWeight ? 1 : 0;
            w.addNode(i++, move(node));
        }
        auto insertEdges = [&w](unsigned succNode, unsigned curNode) -> void {
            w.addEdge(curNode, succNode);
        };
        for (auto &nodePair: w.getNodes()) {
            _customCPG.for_each_succ(nodePair.first, insertEdges);
        }
        w.setDepnEdges(_customCPG.getDepnEdges());
        cout << w.toString() <<'\n';
        wfdgs.push_back(move(w));
    }


}
