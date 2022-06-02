//
// Created by Unravel on 2022/3/24.
//

#ifndef WFG_GENERATOR_WFDGGEN_H
#define WFG_GENERATOR_WFDGGEN_H

#include "WFDGGen/WFDG.h"
#include "DepnMapper.h"
#include "CustomCPG.h"
#include "WFDGGen/Configuration.h"

using namespace std;

namespace wfdg {
    //! WFDG生成器
    class WFDGGenerator {
    private:
        using WFDGNode = WFDG::WFDGNode;
        using VarIdPair = DepnMapper::VarIdPair;
        using LeftData = DepnMapper::LeftData;
        using RightData = DepnMapper::RightData;
        using RefPair = DepnMapper::RefPair;
        using AssignPair = DepnMapper::AssignPair;
        template<typename T>
        using VarMap = DepnMapper::VarMap<T>;

        const Configuration &_config;   //!< 配置
        CustomCPG &_customCPG;  //!< 定制CPG

        int _sensitiveIdx{};    //!< 敏感行的序号

        //! 设置敏感行的序号
        void _setSensitiveIdx(int idx) {
            _sensitiveIdx = idx;
        }

        void _genContrDepnWeight(unordered_map<unsigned, double> &depnWeightMap) const;

        void _genDataDepnWeight(unordered_map<unsigned, double> &depnWeightMap);

        void _genNodeWeight(const unordered_map<unsigned, double> &depnWeightMap,
                            unordered_map<unsigned, double> &nodeWeightMap) const;

        WFDG _buildWFDG(unsigned rootLine, const unordered_map<unsigned, double> &depnWeightMap,
                        const unordered_map<unsigned, double> &nodeWeightMap) const;

        WFDG _genWFDGWithoutSensitiveLine() const;

    public:
        WFDGGenerator(const Configuration &config, CustomCPG &customCPG)
                : _config(config), _customCPG(customCPG) {}

        void genWFDGs(vector<WFDG> &wfdgs);
    };
}

#endif //WFG_GENERATOR_WFDGGEN_H
