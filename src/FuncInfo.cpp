//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"


namespace wfg {
    void FuncInfo::insertIdentifier(string &&id, unsigned lineNo) {
        auto it = _idMap.find(id);
        if(it == _idMap.end()) {
            _idMap.emplace(id, vector<unsigned>{lineNo});
        } else if(lineNo > it->second.back()){
            it->second.push_back(lineNo);
        }
    }
}