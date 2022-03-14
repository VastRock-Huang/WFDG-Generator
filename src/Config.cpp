//
// Created by Unravel on 2022/3/14.
//

#include "Config.h"

namespace wfg {
    const vector<string> Config::KEYWORDS{"memcpy", "strcpy", "read", "free", "buf"};

    bool Config::hasFuncPrefix{false};

    string Config::FuncPrefix{};

    bool Config::matchFuncPrefix(StringRef funcName) {
        return !hasFuncPrefix || funcName.startswith(FuncPrefix);
    }
}