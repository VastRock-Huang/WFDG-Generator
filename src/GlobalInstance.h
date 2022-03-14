//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_GLOBALINSTANCE_H
#define WFG_GENERATOR_GLOBALINSTANCE_H

#include "FuncInfo.h"


namespace wfg {
    struct GlobalInstance {
        static vector<FuncInfo> funcInfoList;
    };
}

#endif //WFG_GENERATOR_GLOBALINSTANCE_H
