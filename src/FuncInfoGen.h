//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFOGEN_H
#define WFG_GENERATOR_FUNCINFOGEN_H

#include "MiniCFG.h"
#include "GlobalInstance.h"
#include <clang/Analysis/CFG.h>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <utility>
#include <vector>

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
        Preprocessor &_preprocessor;
        const LangOptions &_langOpts;

        vector<pair<unsigned, unsigned>>
        _findSensitiveLines(const SourceLocation &beginLoc, const SourceLocation &endLoc) const;

        MiniCFG _buildMiniCFG(const FunctionDecl *funcDecl) const;

        void _travelCFGStmt(const Stmt *stmt, CFGNode &node) const;

        void _catchSpecialStmt(const Stmt *stmt, CFGNode &node) const;

        pair<unsigned, unsigned> _getStmtLineRange(const SourceRange &sourceRange) const;

    public:
        FuncInfoGenConsumer(ASTContext &ctx, Preprocessor& processor) : _context(ctx), _manager(ctx.getSourceManager()),
                                                                       _preprocessor(processor),
                                                                       _langOpts(processor.getLangOpts()) {}

        void HandleTranslationUnit(ASTContext &ctx) override {
            TraverseDecl(ctx.getTranslationUnitDecl());
        }

        bool VisitFunctionDecl(FunctionDecl *funcDecl);

        bool VisitVarDecl(VarDecl * varDecl){
            if(_manager.getFileID(varDecl->getSourceRange().getBegin()) == _manager.getMainFileID()){
                GlobalInstance::VarDeclSet.emplace(varDecl->getQualifiedNameAsString());
            }
            return true;
        }
    };

    class FuncInfoGenAction
            : public ASTFrontendAction {
    private:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<FuncInfoGenConsumer>(compiler.getASTContext(),
                                                          compiler.getPreprocessor());
        }

        void ExecuteAction() override {
            ASTFrontendAction::ExecuteAction();
            _lexToken();
        }

        void _lexToken() const;
    };

}

#endif //WFG_GENERATOR_FUNCINFOGEN_H
