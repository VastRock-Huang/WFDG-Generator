//
// Created by Unravel on 2022/3/14.
//
#include "GlobalInstance.h"

namespace wfg {
    Configuration GlobalInstance::Config{};
    vector<FuncInfo> GlobalInstance::FuncInfoList{};
    unordered_set<string> GlobalInstance::VarDeclSet{};
}
