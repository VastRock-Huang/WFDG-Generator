//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_GLOBALINSTANCE_H
#define WFG_GENERATOR_GLOBALINSTANCE_H

#include "Configuration.h"
#include "FuncInfo.h"
#include <unordered_set>


namespace wfg {
    class GlobalInstance {
    private:
        GlobalInstance() = default;

    public:
        const Configuration Config{};
        vector<FuncInfo> FuncInfoList{};

        static GlobalInstance& getInstance() {
            static GlobalInstance instance;
            return instance;
        }

        GlobalInstance(const GlobalInstance&) = delete;
        GlobalInstance& operator=(const GlobalInstance&) = delete;
    };
}

#endif //WFG_GENERATOR_GLOBALINSTANCE_H
