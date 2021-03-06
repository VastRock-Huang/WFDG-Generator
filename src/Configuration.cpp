//
// Created by Unravel on 2022/3/14.
//

#include "WFDGGen/Configuration.h"

namespace wfdg {
    const unordered_set<string> Configuration::DEFAULT_KEYWORDS{"USER_SET","memcpy", "strcpy", "read", "free", "buf"};

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

    //! 判断是否满足配置中给定的目标函数
    bool Configuration::matchDestFunc(const string &funcName) const {
        return destFunc.empty() || funcName == destFunc;
    }

}