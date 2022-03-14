//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_WFGGEN_H
#define WFG_GENERATOR_WFGGEN_H

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

#include <vector>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

namespace wfg {

    class WFGGenConsumer
            : public ASTConsumer, public RecursiveASTVisitor<WFGGenConsumer> {
    private:
        ASTContext &_context;
        SourceManager &_manager;

        vector<pair<unsigned, unsigned>>
        findSensitiveLines(const SourceLocation &beginLoc, const SourceLocation &endLoc) const;

    public:
        WFGGenConsumer(ASTContext &ctx) : _context(ctx), _manager(ctx.getSourceManager()) {}

        void HandleTranslationUnit(ASTContext &ctx) override {
            TraverseDecl(ctx.getTranslationUnitDecl());
        }

        bool VisitFunctionDecl(FunctionDecl *funcDecl);
    };

    class WFGGenAction
            : public ASTFrontendAction {
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<WFGGenConsumer>(compiler.getASTContext());
        }
    };

}

#endif //WFG_GENERATOR_WFGGEN_H
