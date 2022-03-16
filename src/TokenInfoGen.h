//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_TOKENINFOGEN_H
#define WFG_GENERATOR_TOKENINFOGEN_H


#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

namespace wfg{
    class TokenInfoGenAction
            : public FrontendAction {
    private:
        unsigned checkAndGetTokenInFunc(const Token& token);
    public:
        void ExecuteAction() override {
            Preprocessor &preprocessor = getCompilerInstance().getPreprocessor();
            Token token;
            preprocessor.EnterMainSourceFile();
            do {
                preprocessor.Lex(token);
                unsigned lineNo = checkAndGetTokenInFunc(token);
                if(lineNo != 0) {

                }
            } while (token.isNot(tok::eof));
        }
    };
}

#endif //WFG_GENERATOR_TOKENINFOGEN_H
