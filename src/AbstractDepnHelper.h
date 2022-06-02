//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_ABSTRACTDEPNHELPER_H
#define WFG_GENERATOR_ABSTRACTDEPNHELPER_H

#include "DepnMapper.h"
#include "util.h"
#include <clang/AST/AST.h>
#include <clang/Analysis/CFG.h>
#include <unordered_map>
#include <utility>

using namespace clang;

namespace wfdg {
    class AbstractDepnHelper {
    public:
        //! 变量ID对
        using VarIdPair = DepnMapper::VarIdPair;

    protected:
        //! 语句映射表,用于跳过已经遍历过的语句
        class StmtHelper {
        private:
            using StmtMapTy = llvm::DenseMap<const Stmt *, pair<unsigned, unsigned>>;

            StmtMapTy _StmtMap;
            unsigned _currentBlock = 0;
            unsigned _currStmt = 0;

        public:
            StmtHelper(const CFG *cfg);

            //! 设置结点ID
            void setBlockID(unsigned i) { _currentBlock = i; }

            //! 设置语法序号
            void setStmtID(unsigned i) { _currStmt = i; }

            //! 判断当前语句是否跳过
            bool skipStmt(const Stmt *S) {
                StmtMapTy::iterator I = _StmtMap.find(S);
                if (I == _StmtMap.end()) {
                    return false;
                }
                if (_currentBlock >= 0 && I->second.first == _currentBlock
                    && I->second.second == _currStmt) {
                    return false;
                }
                return true;
            }
        };

        const unique_ptr <CFG> &_cfg;
        const unsigned _nodeCnt;
        StmtHelper _stmtHelper;
        unsigned _nodeID{};
        bool _debug{false};

        virtual void _doDepnOfReadRef(const DeclRefExpr *refExpr) = 0;

        //! 处理成员表达式
        virtual void _doDepnOfReadMember(const MemberExpr *memberExpr) = 0;

        //! 处理写入变量
        virtual void _doDepnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) = 0;

        //! 处理读写变量
        virtual void _doDepnOfRWVar(const Stmt *stmt) = 0;

        //! 处理变量声明
        virtual void _depnOfDecl(const VarDecl *varDecl) = 0;

        //! 处理Terminator条件语句
        virtual void _doTerminatorCondition(const Stmt *stmt) {}

        //! 在处理完结点依赖关系后调用
        virtual void _doAtNodeEnding() {};

        //! 更新结点ID
        virtual void _doNodeIdUpdate(unsigned nodeID) {
            _nodeID = nodeID;
        }

        void _depnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr);

        void _buildDepn(const Stmt *stmt);

        static VarIdPair _getRefVarIds(const DeclRefExpr *refExpr) {
            return make_pair(0, refExpr->getDecl()->getID());
        }

        //! 获取成员表达式的变量ID对
        static VarIdPair _getStructIds(const MemberExpr *memberExpr) {
            const Stmt *stmt = memberExpr;
            while (stmt->child_begin() != stmt->child_end()) {
                stmt = *(stmt->child_begin());
                if (const DeclRefExpr *refExpr = dyn_cast<DeclRefExpr>(stmt)) {
                    return make_pair(refExpr->getDecl()->getID(), memberExpr->getMemberDecl()->getID());
                }
            }
            return {};
        }

    public:
        explicit AbstractDepnHelper(const unique_ptr <CFG> &cfg)
                : _cfg(cfg), _nodeCnt(cfg->size()), _stmtHelper(cfg.get()) {}

        AbstractDepnHelper(const unique_ptr <CFG> &cfg, bool debug)
                : _cfg(cfg), _nodeCnt(cfg->size()), _stmtHelper(cfg.get()), _debug(debug) {}

        //! 构建变量依赖关系到CPG中
        void buildDepnInCPG() {
            // 变量CFG所有结点
            for (auto it = _cfg->rbegin(); it != _cfg->rend(); ++it) {
                CFGBlock *block = *it;
                if (_debug)
                    block->print(llvm::outs(), _cfg.get(), LangOptions(), false);
                unsigned nodeID = block->getBlockID();
                _stmtHelper.setBlockID(nodeID);
                _doNodeIdUpdate(nodeID);
                int j = 1;
                for (const CFGElement &element: *block) {
                    _stmtHelper.setStmtID(j);
                    if (Optional < CFGStmt > cfgStmt = element.getAs<CFGStmt>()) {
                        const Stmt *stmt = cfgStmt->getStmt();
                        if(_debug){
                            llvm::outs() << "[" << nodeID << "."<< j <<"]\n";
                        }
                        _buildDepn(stmt);
                    }
                    ++j;
                }
                _doTerminatorCondition(block->getTerminatorCondition());
                _doAtNodeEnding();
            }
        }

        //! 处理参数声明的依赖关系
        void depnOfParamDecl(llvm::ArrayRef<ParmVarDecl *> params) {
            _doNodeIdUpdate(_nodeCnt - 1);
            for (const ParmVarDecl *paramVarDecl: params) {
                _depnOfDecl(paramVarDecl);
            }
        }
    };
}


#endif //WFG_GENERATOR_ABSTRACTDEPNHELPER_H
