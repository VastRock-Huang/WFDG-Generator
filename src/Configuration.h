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
        string _destFunc{};

        unsigned _sensitiveLine{0};

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

        const vector<string> ASTStmtKinds;

        const unordered_map<string, unsigned> ASTStmtKindMap;

        const bool useWeight{true};

        const double weightPredRatio{0.85};

        const double weightSuccRatio{0.85};

        const unsigned graphPredDepth{5};

        const unsigned graphSuccDepth{5};

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        void init(string destFunc, unsigned sensitiveLine) {
            static bool initialized{false};
            if(initialized) {
                return;
            }
            initialized = true;
            // 只有给定函数名给定敏感行才有意义
            if(!destFunc.empty()) {
                _sensitiveLine = sensitiveLine;
            }
            _destFunc = move(destFunc);
        }

        bool matchDestFunc(const string& funcName) const;

        unsigned getSensitiveLine() const {
            return _sensitiveLine;
        }

        void updateStmtVec(vector<unsigned> &stmtVec, const string &stmtName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
