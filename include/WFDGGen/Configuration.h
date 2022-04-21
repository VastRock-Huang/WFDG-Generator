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

namespace wfdg {
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
        string destFunc{};

        unsigned sensitiveLine{0};

        bool debug{false};

        static const vector<string> DEFAULT_KEYWORDS;

        static const vector<string> DEFAULT_AST_STMT_KINDS;

        vector<string> keyWords;

        vector<string> ASTStmtKinds;

        unordered_map<string, unsigned> ASTStmtKindMap;

        bool useWeight{true};

        double weightPredRatio{0.85};

        double weightSuccRatio{0.85};

        unsigned graphPredDepth{5};

        unsigned graphSuccDepth{5};

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        void init(string destFuncName, unsigned sensitiveLineNo, bool doDebug) {
            static bool initialized{false};
            if(initialized) {
                return;
            }
            initialized = true;
            // 只有给定函数名给定敏感行才有意义
            if(!destFunc.empty()) {
                sensitiveLine = sensitiveLineNo;
            }
            debug = doDebug;
            destFunc = move(destFuncName);
        }

        bool matchDestFunc(const string& funcName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
