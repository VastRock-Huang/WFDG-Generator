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
        const vector<string> _initKeywords() {
            return DEFAULT_KEYWORDS;
        }

        const vector<string> _initASTStmtKinds() {
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

        const bool hasFuncPrefix{false};

        const string destFunc{};

        const vector<string> ASTStmtKinds;

        const unordered_map<string, unsigned> ASTStmtKindMap;

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        bool matchDestFunc(const string& funcName) const;

        void updateStmtVec(vector<unsigned> &stmtVec, const string &stmtName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
