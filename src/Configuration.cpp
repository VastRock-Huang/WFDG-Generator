//
// Created by Unravel on 2022/3/14.
//

#include "Configuration.h"

namespace wfg {
    const vector<string> Configuration::DEFAULT_KEYWORDS{"memcpy", "strcpy", "read", "free", "buf"};

    const vector<string> Configuration::DEFAULT_AST_STMT_KINDS{
            "VarDecl", "FunctionDecl", "FieldDecl", "DeclRefExpr",
                  "IntegerLiteral", "TypeDefDecl", "EnumConstantDecl", "BinaryOperator",
                  "ParenExpr", "CStyleCastExpr", "CallExpr", "UnaryOperator",
                  "CompoundStmt", "IfStmt", "StringLiteral", "ArraySubscriptExpr",
                  "PureAttr", "ConstAttr", "DeclStmt", "ReturnStmt",
                  "InitListExpr", "GotoStmt", "AsmLabelAttr", "ConditionalOperator",
                  "EnumDecl", "CaseStmt", "AsmStmt", "NullStmt",
                  "StmtExpr", "ForStmt", "BreakStmt", "DoStmt",
                  "WhileStmt", "SwitchStmt", "ContinueStmt", "ImplicitCastExpr"
    };

    bool Configuration::matchDestFunc(const string &funcName) const {
        return !hasFuncPrefix || funcName == destFunc;
    }

    void Configuration::updateStmtVec(vector<unsigned> &stmtVec, const string &stmtName) const {
        auto it = ASTStmtKindMap.find(stmtName);
        if (it != ASTStmtKindMap.end()) {
            ++stmtVec[it->second];
        }
    }

    void Configuration::mergeLineRanges(vector<pair<unsigned,unsigned>> &ranges) {
        vector<pair<unsigned,unsigned>> result;
        sort(ranges.begin(), ranges.end());
        size_t i = 0, sz = ranges.size();
        while (i < sz) {
            unsigned minx = ranges[i].first, maxx = ranges[i].second;
            ++i;
            while (i < sz) {
                if(ranges[i].first <= maxx + 1) {
                    maxx = max(maxx, ranges[i].second);
                    ++i;
                } else {
                    break;
                }
            }
            result.emplace_back(minx, maxx);
        }
        ranges.assign(result.begin(), result.end());
    }
}