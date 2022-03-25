//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CONFIGURATION_H
#define WFG_GENERATOR_CONFIGURATION_H

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>

using namespace std;

namespace wfg {
    class Configuration {
    private:
        static vector<string> _initKeywords() {
            return DEFAULT_KEYWORDS;
        }

        static vector<string> _initASTStmtKinds() {
            return DEFAULT_AST_STMT_KINDS;
        }

        unordered_map<string, unsigned> _initASTStmtKindMap() {
            unordered_map<string, unsigned> kindMap;
            int i = 0;
            for (auto &kind: ASTStmtKinds) {
                kindMap.emplace(kind, i++);
            }
            return kindMap;
        }

    public:
        static const vector<string> DEFAULT_KEYWORDS;

        static const vector<string> DEFAULT_AST_STMT_KINDS;

        const vector<string> keyWords;

        const bool hasDestFunc{false};

        const string destFunc{"rsa_int_export_to"};

        const bool hasSensitiveLine{false};

        const unsigned sensitiveLine{0};

        const vector<string> ASTStmtKinds;

        const unordered_map<string, unsigned> ASTStmtKindMap;

        const bool useWeight{true};

        const double weightPredRatio{0.85};

        const double weightSuccRatio{0.85};

        const unsigned graphPredDepth{5};

        const unsigned graphSuccDepth{5};

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        bool matchDestFunc(const string& funcName) const;

        void updateStmtVec(vector<unsigned> &stmtVec, const string &stmtName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
