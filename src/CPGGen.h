//
// Created by Unravel on 2022/3/14.
//

#ifndef WFG_GENERATOR_CPGGEN_H
#define WFG_GENERATOR_CPGGEN_H

#include "CustomCPG.h"
#include "WFDGGen/Configuration.h"
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

namespace wfdg {

    class CPGGenConsumer
            : public ASTConsumer, public RecursiveASTVisitor<CPGGenConsumer> {
    private:
        const Configuration &_config;
        ASTContext &_context;
        const SourceManager &_manager;
        vector<CustomCPG> &_customCPGList;

        pair<unsigned, unsigned> _getLineRange(const SourceLocation &beginLoc, const SourceLocation &endLoc) const;

        map<unsigned, int> _findSensitiveLines(const FunctionDecl *functionDecl,
                                               const pair<unsigned, unsigned> &lineRange) const;

        void _buildCustomCPG(const FunctionDecl *funcDecl, CustomCPG &customCPG) const;

        void _buildDepnInCPG(const FunctionDecl *funcDecl, const unique_ptr <CFG> &wholeCFG,
                             CustomCPG &customCPG) const;

        void _traverseCFGStmtToUpdateStmtVec(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const;

        void _catchSpecialStmt(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const;

    public:
        CPGGenConsumer(ASTContext &ctx, const Configuration &config, vector<CustomCPG> &customCPGList) :
                _config(config), _context(ctx), _manager(ctx.getSourceManager()), _customCPGList(customCPGList) {}

        void HandleTranslationUnit(ASTContext &ctx) override {
            TraverseDecl(ctx.getTranslationUnitDecl());
        }

        bool VisitFunctionDecl(FunctionDecl *funcDecl);
    };

    class CPGGenAction
            : public ASTFrontendAction {
    private:
        const Configuration &_config;
        vector <CustomCPG> &_customCPGList;

        unique_ptr <ASTConsumer> CreateASTConsumer(CompilerInstance &compiler, StringRef file) override {
            return llvm::make_unique<CPGGenConsumer>(compiler.getASTContext(), _config, _customCPGList);
        }

    public:
        CPGGenAction(const Configuration &config, vector <CustomCPG> &customCPGList)
                : _config(config), _customCPGList(customCPGList) {}
    };


    class CPGGenFactory
            : public FrontendActionFactory {
    private:
        const Configuration &_config;
        vector <CustomCPG> &_customCPGList;
    public:
        CPGGenFactory(const Configuration &config, vector <CustomCPG> &customCPGList)
                : _config(config), _customCPGList(customCPGList) {}

        FrontendAction *create() override {
            return new CPGGenAction(_config, _customCPGList);
        }
    };

}

#endif //WFG_GENERATOR_CPGGEN_H
