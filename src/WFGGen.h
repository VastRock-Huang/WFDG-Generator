//
// Created by Unravel on 2022/3/24.
//

#ifndef WFG_GENERATOR_WFGGEN_H
#define WFG_GENERATOR_WFGGEN_H

#include "WFGGen/WFG.h"
#include "Configuration.h"
#include "FuncInfo.h"

using namespace std;

namespace wfg {
    class WFGGenerator {
    private:
        const Configuration &_config;
        const FuncInfo &_funcInfo;
        const CustomCFG &_customCFG;

        void _genLineWeight(unsigned rootLine, map<unsigned, double> &lineWeightMap);

        void _getWFGNodes(const map<unsigned, double> &lineWeightMap, map<unsigned, WFGNode> &wfgNodes);

        void _genNodeWeight(map<unsigned, WFGNode> &wfgNodes, const vector<unsigned> &rootNodes);

        WFG _buildWFG(map<unsigned, WFGNode> &wfgNodes, unsigned rootLine);

        WFG _genWFGWithoutSensitiveLine();

        static vector<unsigned> findRootNodes(const map<unsigned, WFGNode> &wfgNodes, unsigned rootLine);

    public:
        WFGGenerator(const Configuration &config, const FuncInfo &funcInfo) : _config(config), _funcInfo(funcInfo),
                                                                              _customCFG(_funcInfo.getCFG()) {}

        vector<WFG> genWFGs();
    };
}

#endif //WFG_GENERATOR_WFGGEN_H
