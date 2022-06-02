//
// Created by Unravel on 2022/3/14.
//


#include "ContrDepn.h"
#include "CPGGen.h"
#include "SimplifiedDepnHelper.h"
#include "DetailedDepnHelper.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfdg {

    map<unsigned, int> CPGGenConsumer::_findSensitiveLines(const FunctionDecl *functionDecl,
                                                           const pair<unsigned, unsigned> &lineRange) const {
        if(_config.noSensitive) {
            return {};
        }
        unsigned sensitiveLine = _config.sensitiveLine;
        if (sensitiveLine != 0 && util::numInRange(sensitiveLine, lineRange) == 0) {
            return {{sensitiveLine, 0}};
        }
        const SourceLocation &beginLoc = functionDecl->getLocation();
        const SourceLocation &endLoc = functionDecl->getEndLoc();

        FileID fileId = _manager.getMainFileID();
        StringRef funcContent{_manager.getCharacterData(beginLoc),
                              _manager.getCharacterData(endLoc) - _manager.getCharacterData(beginLoc) + 1UL};
        unsigned fileOffset = _manager.getFileOffset(beginLoc);
        map<unsigned, int> res{};
        int idx = 0;
        for (const string& key: _config.keyWords) {
            StringRef keyword{key};
            size_t pos = 0;
            pos = funcContent.find(keyword, pos);
            while (pos != StringRef::npos) {
                unsigned line = _manager.getSpellingLineNumber(_manager.getComposedLoc(fileId, fileOffset + pos));
                bool insert{};
//                llvm::outs() << keyword << ": " << line << '\n';
                tie(std::ignore, insert) = res.emplace(line, idx);
                if (insert) {
                    ++idx;
                }
                pos += keyword.size();
                pos = funcContent.find(keyword, pos);
            }
        }
        return res;
    }

    //! 深度遍历当前语句并更新结点的AST语法特征向量
    void CPGGenConsumer::_traverseCFGStmtToUpdateStmtVec(const Stmt *stmt, CustomCPG &customCPG,
                                                         unsigned nodeID) const {
        customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
            _traverseCFGStmtToUpdateStmtVec(*it, customCPG, nodeID);
        }
    }

    //! 更新结点特殊(Terminator)语句的AST语法特征向量
    void CPGGenConsumer::_catchSpecialStmt(const Stmt *stmt, CustomCPG &customCPG, unsigned nodeID) const {
        if (stmt) {
            customCPG.updateNodeStmtVec(nodeID, stmt->getStmtClassName());
        }
    }

    //! 获取给定源代码起止位置对应的起止行号
    pair<unsigned, unsigned>
    CPGGenConsumer::_getLineRange(const SourceLocation &beginLoc, const SourceLocation &endLoc) const {
        unsigned startLine = _context.getFullLoc(beginLoc).getSpellingLineNumber();
        unsigned endLine = _context.getFullLoc(endLoc).getSpellingLineNumber();
        if (startLine <= endLine) {
            return make_pair(startLine, endLine);
        }
        return make_pair(endLine, startLine);
    }

    //! 构建定制CPG的控制流图部分
    void CPGGenConsumer::_buildContrFlowInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                              CustomCPG &customCPG) const {
        customCPG.initNodeCnt(wholeCFG->size());

        for (auto &block: *wholeCFG) {
            unsigned cur = block->getBlockID();     // 获取结点ID

            // 添加出度边
            for (auto it = block->succ_begin(); it != block->succ_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                customCPG.addSuccEdge(cur, b->getBlockID());
            }
            customCPG.finishSuccEdges();

            // 添加入度边
            for (auto it = block->pred_begin(); it != block->pred_end(); ++it) {
                CFGBlock *b = *it;
                if (!b && !(b = it->getPossiblyUnreachableBlock())) {
                    continue;
                }
                customCPG.addPredEdge(cur, b->getBlockID());
            }
            customCPG.finishPredEdges();

            // 遍历结点中的语句更新AST语法特征向量
            for (const CFGElement &element: *block) {
                if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                    const Stmt *stmt = cfgStmt->getStmt();
                    _traverseCFGStmtToUpdateStmtVec(stmt, customCPG, cur);
                }
            }
            // 对Terminator等特殊语句单独处理
            const Stmt *terStmt = block->getTerminatorStmt();
            _catchSpecialStmt(terStmt, customCPG, cur);
            if (terStmt) {
                customCPG.setHasCondition(cur);
                if (isa<ForStmt>(terStmt) || isa<DoStmt>(terStmt) || isa<WhileStmt>(terStmt)) {
                    customCPG.setIsLoop(cur);
                }
            }
            _catchSpecialStmt(block->getLoopTarget(), customCPG, cur);
            _catchSpecialStmt(block->getLabel(), customCPG, cur);
        }
    }


    //! 遍历函数声明的回调函数
    bool CPGGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        // 有函数体且为本cpp文件
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getLocation()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if (_config.matchDestFunc(funcName)) {
                if (_config.debug)
                    llvm::outs() << "\nFUNC: " << funcName << '\n';
                // 获取函数起止行号
                pair<unsigned, unsigned> lineRange = _getLineRange(funcDecl->getLocation(), funcDecl->getEndLoc());
                // 若配置了优化选项且函数小于10行则跳过
                if(_config.useOptimization && lineRange.second - lineRange.first <= 10) {
                    return true;
                }
                // clang生成CFG图
                unique_ptr<CFG> wholeCFG = CFG::buildCFG(funcDecl, funcDecl->getBody(), &_context, CFG::BuildOptions());
                // 若配置了优化选项且CFG结点数小于5则跳过
                if(_config.useOptimization && wholeCFG->size() <= 5) {
                    return true;
                }

                CustomCPG customCPG(funcName, _config.ASTStmtKindMap, move(_findSensitiveLines(funcDecl, lineRange)));
                _buildContrFlowInCPG(funcDecl, wholeCFG, customCPG);
                _buildDepnInCPG(funcDecl, wholeCFG, customCPG);
                _customCPGList.push_back(customCPG);
            }
        }
        return true;
    }

    //! 构建定制CPG的依赖关系部分
    void CPGGenConsumer::_buildDepnInCPG(const FunctionDecl *funcDecl, const unique_ptr<CFG> &wholeCFG,
                                         CustomCPG &customCPG) const {
        unique_ptr<AbstractDepnHelper> depnHelper{};
        // 根据有无敏感行构建不同的依赖关系构建器
        if (customCPG.getSensitiveLineMap().empty()) {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new SimplifiedDepnHelper(wholeCFG, customCPG, customCPG.getDataDepnEdges(), _config.debug));
        } else {
            depnHelper = unique_ptr<AbstractDepnHelper>(
                    new DetailedDepnHelper(wholeCFG, _context, customCPG, customCPG.getDepnMapper(), _config.debug));
        }

        depnHelper->depnOfParamDecl(funcDecl->parameters());
        depnHelper->buildDepnInCPG();

        ContrDepn contrDepn(customCPG, wholeCFG->size());
        auto res = contrDepn.gen();
        customCPG.setContrDepn(res);
//        llvm::outs() << "dom:" << util::vecToString(res) << '\n';
    }

}