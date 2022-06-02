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
    //! 模块配置
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
        //! 生成WFDG的目标函数
        string destFunc{};

        //! 是否有敏感行,true时则直接生成无敏感行的WFDG
        bool noSensitive{false};

        //! 敏感行号
        unsigned sensitiveLine{0};

        //! 是否启用Debug
        bool debug{false};

        static const unordered_set<string> DEFAULT_KEYWORDS;

        static const vector<string> DEFAULT_AST_STMT_KINDS;

        //! 漏洞关键词集合
        unordered_set<string> keyWords;

        //! AST语法特征的名称vector
        vector<string> ASTStmtKinds;

        //! AST语法特征的名称映射(AST语法特征名->序号)
        unordered_map<string, unsigned> ASTStmtKindMap;

        //! 是否使用权重
        bool useWeight{true};

        //! 是否启用优化(移除小函数和小图)
        bool useOptimization{true};

        //! 权重前驱衰减率
        double weightPredRatio{0.85};

        //! 权重后继衰减率
        double weightSuccRatio{0.85};

        //! 图前驱跟踪深度
        unsigned graphPredDepth{5};

        //! 图后继跟踪深度
        unsigned graphSuccDepth{5};

        Configuration() : keyWords(_initKeywords()), ASTStmtKinds(_initASTStmtKinds()),
                          ASTStmtKindMap(_initASTStmtKindMap()) {}

        //! 指定函数和敏感行
        void specifyFunc(string destFuncName, unsigned sensitiveLineNo, bool notUseSensitive) {
            // 只有给定函数名给定敏感行才有意义
            if(!destFuncName.empty() && !notUseSensitive) {
                sensitiveLine = sensitiveLineNo;
            }
            destFunc = move(destFuncName);
            noSensitive = notUseSensitive;
        }

        //! 添加漏洞关键词
        void addKeyWords(const unordered_set<string>& keys) {
            for(string key: keys) {
                keyWords.emplace(move(key));
            }
        }

        //! 是否匹配目标函数名
        bool matchDestFunc(const string& funcName) const;
    };
}

#endif //WFG_GENERATOR_CONFIGURATION_H
