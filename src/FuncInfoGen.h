//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFOGEN_H
#define WFG_GENERATOR_FUNCINFOGEN_H

#include "MiniCFG.h"
#include "FuncInfo.h"
#include "Configuration.h"
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
#include <memory>

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
        vector<FuncInfo> &_funcInfoList;
        unordered_set<string> &_varDeclSet;


        vector<pair<unsigned, unsigned>>
        _findSensitiveLines(const SourceRange &sourceRange, const pair<unsigned, unsigned> &lineRange) const;

        MiniCFG _buildMiniCFG(const FunctionDecl *funcDecl) const;

        void _travelCFGStmt(const Stmt *stmt, CFGNode &node) const;

        void _catchSpecialStmt(const Stmt *stmt, CFGNode &node) const;

        pair<unsigned, unsigned> _getLineRange(const SourceRange &sourceRange) const;

    public:
        FuncInfoGenConsumer(ASTContext &ctx, const Configuration &config, vector<FuncInfo> &funcInfoList,
                            unordered_set<string> &varDeclSet) :
                _config(config), _context(ctx), _manager(ctx.getSourceManager()), _funcInfoList(funcInfoList),
                _varDeclSet(varDeclSet) {}

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
        const Configuration &_config;
        vector<FuncInfo> &_funcInfoList;
        unordered_set<string> _varDeclSet{};

        unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<FuncInfoGenConsumer>(compiler.getASTContext(),
                                                          _config, _funcInfoList, _varDeclSet);
        }

        void ExecuteAction() override {
            ASTFrontendAction::ExecuteAction();
            _lexToken();
        }

        void _lexToken() const;

    public:
        FuncInfoGenAction(const Configuration &config, vector<FuncInfo> &funcInfoList) : _config(config),
                                                                                         _funcInfoList(funcInfoList) {}
    };


    class FuncInfoGenFactory
            : public FrontendActionFactory {
    private:
        const Configuration &_config;
        vector<FuncInfo> &_funcInfoList;
    public:
        FuncInfoGenFactory(const Configuration &config, vector<FuncInfo> &funcInfoList) : _config(config),
                                                                                          _funcInfoList(funcInfoList) {}

        FrontendAction *create() override {
            return new FuncInfoGenAction(_config, _funcInfoList);
        }
    };

}

#endif //WFG_GENERATOR_FUNCINFOGEN_H
