//
// Created by Unravel on 2022/4/12.
//

#include "DetailedDepnHelper.h"

namespace wfdg {
    void DetailedDepnHelper::_traceReadVar(unsigned searchNode, const VarIdPair &ids,
                                           unordered_set<RefPair, util::pair_hash> &refFrom) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (predNode <= searchNode) {
                continue;
            }
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenVarInNode(predNode, ids)) != -1) {
//                _customCPG.addDepnEdge(predNode, _nodeID);
                refFrom.emplace(leftIdx, predNode);
                llvm::outs() << "find " << _getVarNameByIds(ids) << " at " << predNode << '\n';
            } else {
                _traceReadVar(predNode, ids, refFrom);
            }
        }
    }

    void DetailedDepnHelper::_traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                                 const VarIdPair &memIds, unordered_set<RefPair, util::pair_hash> &refFrom) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            int leftIdx = -1;
            if ((leftIdx = _hasWrittenStructInNode(predNode, varIds, memIds)) != -1) {
//                _customCPG.addDepnEdge(predNode, _nodeID);
                refFrom.emplace(leftIdx, predNode);
                llvm::outs() << "find " << _getVarNameByIds(memIds) << " at " << predNode << '\n';
            } else {
                _traceReadStructVar(predNode, varIds, memIds, refFrom);
            }
        }
    }
}