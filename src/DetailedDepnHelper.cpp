//
// Created by Unravel on 2022/4/12.
//

#include "DetailedDepnHelper.h"

namespace wfdg {
    void DetailedDepnHelper::_traceReadVar(const AbstractDepnHelper::VarIdPair &ids, unsigned int lineNum)  {
        unordered_set<RefPair, util::pair_hash> refFrom{};
        vector<bool> visited(_nodeCnt - _nodeID, false);
        queue<unsigned> nodeQue{};
        nodeQue.push(_nodeID);
        while (!nodeQue.empty()) {
            unsigned curNode = nodeQue.front();
            nodeQue.pop();
            visited[curNode - _nodeID] = true;
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenVarInNode(curNode, ids)) != -1) {
//                _customCPG.addDataDepnEdge(predNode, _nodeID);
                refFrom.emplace(leftIdx, curNode);
                if (_debug)
                    llvm::outs() << "find " << _getVarNameByIds(ids) << " at " << curNode << '\n';
            } else {
                for (unsigned vecIdx = _customCPG.pred_begin(curNode);
                     vecIdx != _customCPG.pred_end(curNode); ++vecIdx) {
                    unsigned predNode = _customCPG.pred_at(vecIdx);
                    if (predNode <= curNode|| visited[predNode - _nodeID]) {
                        continue;
                    }
                    nodeQue.push(predNode);
                }
            }
        }

        int rightIdx = _depnMapper.pushRefInfo(ids, _nodeID, refFrom);
        _readVarMap.emplace(ids, rightIdx);
        int sensitiveIdx = _customCPG.inSensitiveLine(lineNum);
        if (sensitiveIdx != -1) {
            _depnMapper.pushSensitiveRVar(sensitiveIdx, ids, rightIdx, _nodeID);
        }
    }

    void DetailedDepnHelper::_traceReadStructVar(const AbstractDepnHelper::VarIdPair &memIds, const string &name,
                                                 unsigned int lineNum) {
        if (memIds.first == 0) {
            return;
        }
        _insertVarIds(memIds, name);
        VarIdPair varIds = make_pair(0, memIds.first);
        unordered_set<RefPair, util::pair_hash> refFrom{};
        vector<bool> visited(_nodeCnt - _nodeID, false);
        queue<unsigned> nodeQue{};
        nodeQue.push(_nodeID);
        while (!nodeQue.empty()) {
            unsigned curNode = nodeQue.front();
            nodeQue.pop();
            visited[curNode - _nodeID] = true;
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenStructInNode(curNode, varIds, memIds)) != -1) {
//                _customCPG.addDataDepnEdge(predNode, _nodeID);
                refFrom.emplace(leftIdx, curNode);
                if (_debug)
                    llvm::outs() << "find " << _getVarNameByIds(memIds) << " at " << curNode << '\n';
            } else {
                for (unsigned vecIdx = _customCPG.pred_begin(curNode);
                     vecIdx != _customCPG.pred_end(curNode); ++vecIdx) {
                    unsigned predNode = _customCPG.pred_at(vecIdx);
                    if (predNode <= curNode|| visited[predNode - _nodeID]) {
                        continue;
                    }
                    nodeQue.push(predNode);
                }
            }
        }

        int rightIdx = _depnMapper.pushRefInfo(memIds, _nodeID, refFrom);
        _readVarMap.emplace(memIds, rightIdx);
        int sensitiveIdx = _customCPG.inSensitiveLine(lineNum);
        if (sensitiveIdx != -1) {
            _depnMapper.pushSensitiveRVar(sensitiveIdx, memIds, rightIdx, _nodeID);
        }
    }
}