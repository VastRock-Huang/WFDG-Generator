//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"


namespace wfg {
    void FuncInfo::insertIdentifier(const string &id, unsigned lineNo) {
        auto res = _idMapper.idSet.emplace(id);
        const string* idAddr = &(*res.first);

        auto it = _idMapper.idMap.find(idAddr);
        if(it == _idMapper.idMap.end()) {
            _idMapper.idMap.emplace(idAddr, vector<unsigned>{lineNo});
        } else if(lineNo > it->second.back()){
            it->second.push_back(lineNo);
        } else {
            return;
        }

        auto it2 = _idMapper.lineMap.find(lineNo);
        if(it2 != _idMapper.lineMap.end()) {
            it2->second.push_back(idAddr);
        } else {
            _idMapper.lineMap.emplace(lineNo,vector<const string *>{idAddr});
        }
    }
}