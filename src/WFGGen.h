//
// Created by Unravel on 2022/3/24.
//

#ifndef WFG_GENERATOR_WFGGEN_H
#define WFG_GENERATOR_WFGGEN_H

#include "WFGGen/WFG.h"
#include "FuncInfo.h"

using namespace std;

namespace wfg {
    vector<WFG> genWFGs(const FuncInfo& funcInfo);
}

#endif //WFG_GENERATOR_WFGGEN_H
