//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFOGEN_H
#define WFG_GENERATOR_FUNCINFOGEN_H

#include "MiniCFG.h"
#include <clang/Analysis/CFG.h>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <vector>
#include <memory>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

namespace wfg {

    class FuncInfoGenConsumer
            : public ASTConsumer, public RecursiveASTVisitor<FuncInfoGenConsumer> {
    private:
        ASTContext &_context;
        SourceManager &_manager;

        vector<pair<unsigned, unsigned>>
        findSensitiveLines(const SourceLocation &beginLoc, const SourceLocation &endLoc) const;

        MiniCFG buildMiniCFG(FunctionDecl *funcDecl) const;

        void travelCFGStmt(const Stmt* stmt, MiniCFG& miniCFG) const;

        void catchSpecialStmt(const Stmt* stmt, MiniCFG& miniCfg) const;

    public:
        FuncInfoGenConsumer(ASTContext &ctx) : _context(ctx), _manager(ctx.getSourceManager()) {}

        void HandleTranslationUnit(ASTContext &ctx) override {
            TraverseDecl(ctx.getTranslationUnitDecl());
        }

        bool VisitFunctionDecl(FunctionDecl *funcDecl);
    };

    class FuncInfoGenAction
            : public ASTFrontendAction {
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<FuncInfoGenConsumer>(compiler.getASTContext());
        }
    };

}

#endif //WFG_GENERATOR_FUNCINFOGEN_H
