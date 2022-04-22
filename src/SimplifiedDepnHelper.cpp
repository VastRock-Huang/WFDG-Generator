//
// Created by Unravel on 2022/4/12.
//

#include "SimplifiedDepnHelper.h"

namespace wfdg {

    void SimplifiedDepnHelper::_traceReadVar(const AbstractDepnHelper::VarIdPair &ids) {
        vector<bool> visited(_nodeCnt - _nodeID, false);
        queue<unsigned> nodeQue{};
        nodeQue.push(_nodeID);
        visited[_nodeID - _nodeID] = true;
        while (!nodeQue.empty()) {
            unsigned curNode = nodeQue.front();
            nodeQue.pop();
            visited[curNode - _nodeID] = true;
            if (_noneWrittenVarInNode(curNode, ids)) {
                for (unsigned vecIdx = _customCPG.pred_begin(curNode);
                     vecIdx != _customCPG.pred_end(curNode); ++vecIdx) {
                    unsigned predNode = _customCPG.pred_at(vecIdx);
                    if (predNode <= curNode || visited[predNode - _nodeID]) {
                        continue;
                    }
                    nodeQue.push(predNode);
                    visited[predNode - _nodeID] = true;
                }
            } else if (curNode != _nodeID) {
                _addDepnEdge(_nodeID, curNode);
                if (_debug)
                    llvm::outs() << "find " << DepnMapper::varIdPairToString(ids) << " at " << curNode << '\n';
            }
        }
    }

    void SimplifiedDepnHelper::_traceReadStructVar(const AbstractDepnHelper::VarIdPair &memIds)  {
        if (memIds.first == 0) {
            return;
        }
        VarIdPair varIds = make_pair(0, memIds.first);

        vector<bool> visited(_nodeCnt - _nodeID, false);
        queue<unsigned > nodeQue{};
        nodeQue.push(_nodeID);
        visited[_nodeID - _nodeID] = true;
        while (!nodeQue.empty()) {
            unsigned curNode = nodeQue.front();
            nodeQue.pop();
            visited[curNode - _nodeID] = true;
            if (_noneWrittenStructInNode(curNode, varIds, memIds)) {
                for (unsigned vecIdx = _customCPG.pred_begin(curNode);
                     vecIdx != _customCPG.pred_end(curNode); ++vecIdx) {
                    unsigned predNode = _customCPG.pred_at(vecIdx);
                    if (predNode <= curNode || visited[predNode - _nodeID]) {
                        continue;
                    }
                    nodeQue.push(predNode);
                    visited[predNode - _nodeID] = true;
                }
            } else if (curNode != _nodeID) {
                _addDepnEdge(_nodeID, curNode);
                if (_debug)
                    llvm::outs() << "find " << DepnMapper::varIdPairToString(memIds) << " at " << curNode << '\n';
            }
        }
    }
}