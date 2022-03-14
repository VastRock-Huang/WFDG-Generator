//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CONFIG_H
#define WFG_GENERATOR_CONFIG_H

#include <llvm/ADT/StringRef.h>
#include <vector>
#include <string>


using namespace std;
using llvm::StringRef;

namespace wfg {
    struct Config {
        static const vector<string> KEYWORDS;

        static bool hasFuncPrefix;

        static string FuncPrefix;

        static bool matchFuncPrefix(StringRef funcName);
    };
}

#endif //WFG_GENERATOR_CONFIG_H
