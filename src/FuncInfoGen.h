//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_FUNCINFOGEN_H
#define WFG_GENERATOR_FUNCINFOGEN_H

#include "FuncInfo.h"
#include "CustomCPG.h"
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
#include <array>

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
        const SourceManager &_manager;
        vector<FuncInfo> &_funcInfoList;
        unordered_set<string> &_varDeclSet;

        pair<unsigned, unsigned> _getLineRange(const SourceLocation &beginLoc, const SourceLocation &endLoc) const;

        void _findSensitiveLines(const FunctionDecl *functionDecl, const pair<unsigned, unsigned> &lineRange,
                                 CustomCPG &customCPG) const;

        void _buildCustomCPG(const FunctionDecl *funcDecl, CustomCPG &customCPG) const;

        static void _buildDepnInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                    CustomCPG &customCPG);

        void _traverseCFGStmtToUpdateStmtVec(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const;

        void _catchSpecialStmt(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const;

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
