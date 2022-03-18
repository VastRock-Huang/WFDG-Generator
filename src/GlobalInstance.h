//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_GLOBALINSTANCE_H
#define WFG_GENERATOR_GLOBALINSTANCE_H

#include "Configuration.h"
#include "FuncInfo.h"
#include <unordered_set>


namespace wfg {
    struct GlobalInstance {
        static Configuration Config;
        static vector<FuncInfo> FuncInfoList;
        static unordered_set<string> VarDeclSet;
    };
}

#endif //WFG_GENERATOR_GLOBALINSTANCE_H
