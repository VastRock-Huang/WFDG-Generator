//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CONFIGURATION_H
#define WFG_GENERATOR_CONFIGURATION_H

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace wfdg {
    class Configuration {
    private:
        static unordered_set<string> _initKeywords() {
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
        string destFunc{};

        // 为true时则直接生成整个函数的WFDG图
        bool noSensitive{false};

        unsigned sensitiveLine{0};

        bool debug{false};

        static const unordered_set<string> DEFAULT_KEYWORDS;

        static const vector<string> DEFAULT_AST_STMT_KINDS;

        unordered_set<string> keyWords;

        vector<string> ASTStmtKinds;

        unordered_map<string, unsigned> ASTStmtKindMap;

        bool useWeight{true};

        bool useOptimization{true};

        double weightPredRatio{0.85};

        double weightSuccRatio{0.85};

        unsigned graphPredDepth{5};

        unsigned graphSuccDepth{5};

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        void specifyFunc(string destFuncName, unsigned sensitiveLineNo, bool notUseSensitive) {
            // 只有给定函数名给定敏感行才有意义
            if(!destFuncName.empty() && !notUseSensitive) {
                sensitiveLine = sensitiveLineNo;
            }
            destFunc = move(destFuncName);
            noSensitive = notUseSensitive;
        }

        void addKeyWords(const unordered_set<string>& keys) {
            for(string key: keys) {
                keyWords.emplace(move(key));
            }
        }

        bool matchDestFunc(const string& funcName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
