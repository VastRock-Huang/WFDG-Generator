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

    string WFDG::WFDGNode::toJson(const WFDGNode &node) {
        return "{"
               "\"id\": " + util::numToString(node.id) +
               ", \"depnWeight\": " + util::numToString(node.depnWeight) +
               ", \"nodeWeight\": " + util::numToString(node.nodeWeight) +
               ", \"weight\": " + util::numToString(node.weight) +
               ", \"stmtVec\": " + util::vecToString(node.stmtVec) +
               "}";
    }

    string WFDG::toString() const {
        return "{funcName: " + _funcName + ", " +
               "rootLine: " + to_string(_rootLine) + ",\n" +
               "nodes:\n" + util::mapToString(_nodes, util::numToString<unsigned>,
                                              WFDGNode::toString) + ", " +
               "edges: " + util::setToString(_edges, util::numPairToString<unsigned, unsigned>) + ",\n" +
               "dataDepnEdges: " +
               util::setToString(_depnEdges, util::numPairToString<unsigned, unsigned>) +
               "allEdges: " +
               util::setToString(_allEdges, util::numPairToString<unsigned,unsigned>) +
               "}";
    }

    string WFDG::toJson() const {
        return "{\"funcName\": " + util::strToJson(_funcName) + ", " +
               "\"rootLine\": " + util::numToString(_rootLine) + ", " +
               "\"nodes\": " + util::mapToJson(_nodes, util::numToJson<unsigned>,
                                              WFDGNode::toJson) + ", " +
               "\"allEdges\": " +
               util::setToJson(_allEdges, util::numPairToJson<unsigned,unsigned>) +
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

        vector<WFDG> wfdgs;
        for (auto &cpg: customCPGList) {
            WFDGGenerator wfgGenerator(config, cpg);
            wfgGenerator.genWFDGs(wfdgs);
        }
        return wfdgs;
    }
}