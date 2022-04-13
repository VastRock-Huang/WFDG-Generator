//
// Created by Unravel on 2022/3/24.
//

#ifndef WFG_GENERATOR_WFGGEN_H
#define WFG_GENERATOR_WFGGEN_H

#include "WFDGGen/WFDG.h"
#include "Configuration.h"
#include "FuncInfo.h"

using namespace std;

namespace wfg {
    class WFDGGenerator {
    private:
        using WFDGNode = WFDG::WFDGNode;

        const Configuration &_config;
        const FuncInfo &_funcInfo;
        const CustomCPG &_customCPG;

        void _genLineWeight(unsigned rootLine, map<unsigned, double> &lineWeightMap);

        void _getWFGNodes(const map<unsigned, double> &lineWeightMap, map<unsigned, WFDGNode> &wfdgNodes);

        void _genNodeWeight(map<unsigned, WFDGNode> &wfdgNodes, const vector<unsigned> &rootNodes);

        WFDG _buildWFG(map<unsigned, WFDGNode> &wfgNodes, unsigned rootLine);

        void _genWFDGWithoutSensitiveLine(vector<WFDG>& wfgs);

        static vector<unsigned> findRootNodes(const map<unsigned, WFDGNode> &wfgNodes, unsigned rootLine);

    public:
        WFDGGenerator(const Configuration &config, const FuncInfo &funcInfo) : _config(config), _funcInfo(funcInfo),
                                                                               _customCPG(_funcInfo.getCustomCPG()) {}

        vector<WFDG> genWFDGs();
    };
}

#endif //WFG_GENERATOR_WFGGEN_H
