//
// Created by Unravel on 2022/3/24.
//

#ifndef WFG_GENERATOR_WFGGEN_H
#define WFG_GENERATOR_WFGGEN_H

#include "WFDGGen/WFDG.h"
#include "Configuration.h"
#include "FuncInfo.h"

using namespace std;

namespace wfdg {
    class WFDGGenerator {
    private:
        using WFDGNode = WFDG::WFDGNode;
        using VarIdPair = DepnMapper::VarIdPair;
        using LeftData = DepnMapper::LeftData;
        using RightData = DepnMapper::RightData;
        using RefPair = DepnMapper::RefPair;
        using AssignPair = DepnMapper::AssignPair;

        const Configuration &_config;
        FuncInfo &_funcInfo;
        CustomCPG &_customCPG;

        void _genLineWeight(unsigned sensitiveIdx, unordered_map<unsigned, double> &lineWeightMap);

        void _genNodeWeight(unsigned sensitiveIdx, const unordered_map<unsigned, double> &lineWeightMap,
                            unordered_map<unsigned, double> &nodeWeightMap) const;

        WFDG _buildWFDG(unsigned rootLine, const unordered_map<unsigned, double> & lineWeightMap,
                        const unordered_map<unsigned, double>& nodeWeightMap) const;

        void _genWFDGWithoutSensitiveLine(vector<WFDG> &wfdgs);

    public:
        WFDGGenerator(const Configuration &config, FuncInfo &funcInfo) : _config(config), _funcInfo(funcInfo),
                                                                               _customCPG(funcInfo.getCustomCPG()) {}

        vector<WFDG> genWFDGs();
    };
}

#endif //WFG_GENERATOR_WFGGEN_H
