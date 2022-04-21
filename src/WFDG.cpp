//
// Created by Unravel on 2022/3/25.
//
#include "WFDGGen/WFDG.h"
#include "CPGGen.h"
#include "WFDGGen.h"
#include "util.h"

namespace wfdg {
    string WFDG::WFDGNode::toString(const WFDGNode &node) {
        return "{"
               "id: " + to_string(node.id) +
               ", depnWeight: " + to_string(node.depnWeight) +
               ", nodeWeight: " + to_string(node.nodeWeight) +
               ", weight: " + to_string(node.weight) +
               ", stmtVec: " + util::vecToString(node.stmtVec) +
               "}\n";
    }


    string WFDG::toString() const {
        return "{funcName: " + _funcName + ", " +
               "rootLine: " + to_string(_rootLine) + ",\n" +
               "nodes:\n" + util::mapToString(_nodes, util::numToString<unsigned>,
                                              WFDGNode::toString) + ", " +
               "edges: " + util::setToString(_edges, util::numPairToString<unsigned, unsigned>) + ",\n" +
               "dataDepnEdges: " +
               util::setToString(_depnEdges, util::numPairToString<unsigned, unsigned>) +
               "}";
    }

    vector<WFDG> genWFDGs(const vector<string> &srcPathList, const Configuration &config, vector<string> compileArgs) {
        FixedCompilationDatabase database(".", move(compileArgs));
        ClangTool tool(database, move(srcPathList));
        // 取消错误输出
        class : public DiagnosticConsumer {
        public:
            virtual bool IncludeInDiagnosticCounts() const {
                return false;
            }
        } diagConsumer;
        tool.setDiagnosticConsumer(&diagConsumer);
        vector<CustomCPG> customCPGList{};
        tool.run(unique_ptr<CPGGenFactory>(new CPGGenFactory(config, customCPGList)).get());
        vector<WFDG> res{};
        vector<WFDG> wfdgs;
        for (auto &cpg: customCPGList) {
            WFDGGenerator wfgGenerator(config, cpg);
            wfgGenerator.genWFDGs(wfdgs);
        }
        return wfdgs;
    }
}