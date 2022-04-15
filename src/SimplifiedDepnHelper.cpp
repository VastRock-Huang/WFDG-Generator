//
// Created by Unravel on 2022/4/12.
//

#include "SimplifiedDepnHelper.h"

namespace wfdg {
    void SimplifiedDepnHelper::_traceReadVar(unsigned searchNode, const VarIdPair &ids) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (predNode <= searchNode) {
                continue;
            }
            if (_noneWrittenVarInNode(predNode, ids)) {
                _traceReadVar(predNode, ids);
            } else {
                _customCPG.addDepnEdge(predNode, _nodeID);
                if (_debug)
                    llvm::outs() << "find " << DepnMapper::varIdPairToString(ids) << " at " << predNode << '\n';
            }
        }
    }

    void SimplifiedDepnHelper::_traceReadStructVar(unsigned searchNode, const VarIdPair &varIds,
                                                   const VarIdPair &memIds) {
        for (unsigned vecIdx = _customCPG.pred_begin(searchNode);
             vecIdx != _customCPG.pred_end(searchNode); ++vecIdx) {
            unsigned predNode = _customCPG.pred_at(vecIdx);
            if (_noneWrittenStructInNode(predNode, varIds, memIds)) {
                _traceReadStructVar(predNode, varIds, memIds);
            } else {
                _customCPG.addDepnEdge(predNode, _nodeID);
                if (_debug)
                    llvm::outs() << "find " << DepnMapper::varIdPairToString(memIds) << " at " << predNode << '\n';
            }
        }
    }
}