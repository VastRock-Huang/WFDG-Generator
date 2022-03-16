//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CONFIGURATION_H
#define WFG_GENERATOR_CONFIGURATION_H

#include <llvm/ADT/StringRef.h>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <algorithm>


using namespace std;
using llvm::StringRef;

namespace wfg {
    class Configuration {
    private:
        const vector<string> initKeywords() {
            return DEFAULT_KEYWORDS;
        }

        const vector<string> initASTStmtKinds() {
            return DEFAULT_AST_STMT_KINDS;
        }

        unordered_map<string, unsigned> initASTStmtKindMap() {
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

        const string funcPrefix{};

        const vector<string> ASTStmtKinds;

        const unordered_map<string, unsigned> ASTStmtKindMap;

        Configuration() : keyWords(initKeywords()), ASTStmtKinds(initASTStmtKinds()),
                          ASTStmtKindMap(initASTStmtKindMap()) {}

        bool matchFuncPrefix(StringRef funcName) const;

        void updateStmtVec(vector<unsigned> &stmtVec, const string &stmtName) const;

        static void mergeLineRanges(vector<pair<unsigned, unsigned>> &ranges);
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
