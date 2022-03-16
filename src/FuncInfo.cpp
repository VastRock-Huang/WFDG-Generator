//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"


namespace wfg {
    void FuncInfo::insertIdentifier(const string &id, unsigned lineNo) {
        auto it = _idMap.find(id);
        if(it != _idMap.end()) {
            it->second.push_back(lineNo);
        } else {
            _idMap.emplace(id, vector<unsigned>{lineNo});
        }
    }
}