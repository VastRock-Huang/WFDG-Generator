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
        const Configuration &_config;
        ASTContext &_context;
        SourceManager &_manager;
        unordered_set<string> &_varDeclSet;

        vector<pair<unsigned, unsigned>>
        _findSensitiveLines(const SourceRange &sourceRange, const pair<unsigned,unsigned>& lineRange) const;

        MiniCFG _buildMiniCFG(const FunctionDecl *funcDecl) const;

        void _travelCFGStmt(const Stmt *stmt, CFGNode &node) const;

        void _catchSpecialStmt(const Stmt *stmt, CFGNode &node) const;

        pair<unsigned, unsigned> _getLineRange(const SourceRange &sourceRange) const;

    public:
        FuncInfoGenConsumer(ASTContext &ctx, const Configuration &config, unordered_set<string> &varDeclSet) :
                _config(config), _context(ctx), _manager(ctx.getSourceManager()), _varDeclSet(varDeclSet) {}

        void HandleTranslationUnit(ASTContext &ctx) override {
            TraverseDecl(ctx.getTranslationUnitDecl());
        }

        bool VisitFunctionDecl(FunctionDecl *funcDecl);

        bool VisitVarDecl(VarDecl *varDecl) {
            if (_manager.getFileID(varDecl->getSourceRange().getBegin()) == _manager.getMainFileID()) {
                _varDeclSet.emplace(varDecl->getQualifiedNameAsString());
            }
            return true;
        }
    };

    class FuncInfoGenAction
            : public ASTFrontendAction {
    private:
        unordered_set<string> _varDeclSet{};

        unique_ptr <ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<FuncInfoGenConsumer>(compiler.getASTContext(),
                                                          GlobalInstance::getInstance().Config, _varDeclSet);
        }

        void ExecuteAction() override {
            ASTFrontendAction::ExecuteAction();
            _lexToken();
        }

        void _lexToken() const;
    };

}

#endif //WFG_GENERATOR_FUNCINFOGEN_H
