//
// Created by Unravel on 2022/4/12.
//

#ifndef WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H
#define WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H

#include "AbstractDepnHelper.h"
#include "CustomCPG.h"
#include <queue>

namespace wfdg {
    class SimplifiedDepnHelper : public AbstractDepnHelper {
    private:
        const CustomCPG &_customCPG;
        vector<unordered_set<VarIdPair, util::pair_hash>> _writtenVarVec;
        set<pair<unsigned, unsigned>> &_depnEdges;

        bool _noneWrittenVarInNode(unsigned nodeID, const VarIdPair &ids) const {
            return _writtenVarVec.at(nodeID).count(ids) == 0;
        }

        bool _noneWrittenStructInNode(unsigned nodeID, const VarIdPair &varIds, const VarIdPair &memIds) const {
            return _writtenVarVec[nodeID].count(varIds) == 0 &&
                   _writtenVarVec[nodeID].count(memIds) == 0;
        }

        void _addDepnEdge(unsigned cur, unsigned pred) {
            _depnEdges.emplace(cur, pred);
        }


        void _traceReadVar(const VarIdPair &ids);

        void _traceReadStructVar(const VarIdPair &memIds);

        void _recordWrittenVar(const VarIdPair &ids) {
            _writtenVarVec.at(_nodeID).emplace(ids);
        }


        static string _getStructName(const MemberExpr *memberExpr) {
            const Stmt *childStmt = *(memberExpr->child_begin());
            while (!isa<DeclRefExpr>(childStmt) && !isa<MemberExpr>(childStmt)) {
                if (childStmt->child_begin() == childStmt->child_end()) {
                    return {};
                }
                childStmt = *(childStmt->child_begin());
            }
            string name{};
            if (isa<DeclRefExpr>(childStmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(childStmt);
                name.append(refExpr->getNameInfo().getAsString());
            } else if (isa<MemberExpr>(childStmt)) {
                const MemberExpr *childMem = cast<MemberExpr>(childStmt);
                name.append(_getStructName(childMem));
            }
            name.append(memberExpr->isArrow() ? "->" : ".");
            name.append(memberExpr->getMemberDecl()->getNameAsString());
            return name;
        }

        void _doDepnOfReadRef(const DeclRefExpr *refExpr) override {
            _traceReadVar(_getRefVarIds(refExpr));
            if (_debug)
                llvm::outs() << "R_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
        };

        void _doDepnOfReadMember(const MemberExpr *memberExpr) override {
            VarIdPair ids = _getStructIds(memberExpr);
            _traceReadStructVar(ids);
            if (_debug)
                llvm::outs() << "R_Mem:" << _getStructName(memberExpr) << '\n';
        }

        void _doDepnOfWrittenVar(const Stmt *writtenExpr, const Stmt *readExpr) override {
            if (isa<DeclRefExpr>(writtenExpr)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(writtenExpr);
                VarIdPair ids = _getRefVarIds(refExpr);
                _recordWrittenVar(ids);
                if (_debug)
                    llvm::outs() << "W_Ref:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(writtenExpr)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(writtenExpr);
                VarIdPair ids = _getStructIds(memberExpr);
                if (ids.second != 0) {
                    _recordWrittenVar(ids);
                }
                if (_debug)
                    llvm::outs() << "W_Mem:" << _getStructName(memberExpr) << '\n';
            }
        }

        void _doDepnOfRWVar(const Stmt *stmt) override {
            VarIdPair ids{};
            if (isa<DeclRefExpr>(stmt)) {
                const DeclRefExpr *refExpr = cast<DeclRefExpr>(stmt);
                ids = _getRefVarIds(refExpr);
                _traceReadVar(ids);
                if (_debug)
                    llvm::outs() << "RW_Decl:" << refExpr->getNameInfo().getAsString() << '\n';
            } else if (isa<MemberExpr>(stmt)) {
                const MemberExpr *memberExpr = cast<MemberExpr>(stmt);
                ids = _getStructIds(memberExpr);
                _traceReadStructVar(ids);
                if (_debug)
                    llvm::outs() << "RW_Mem:" << _getStructName(memberExpr) << '\n';
            }
            _recordWrittenVar(ids);
        }

        void _depnOfDecl(const VarDecl *varDecl) override {
            VarIdPair ids = make_pair(0, varDecl->getID());
            if (const Expr *initExpr = varDecl->getInit()) {
                _buildDepn(initExpr);
            }
            _recordWrittenVar(ids);
            if (_debug)
                llvm::outs() << "W_DefDecl: " << varDecl->getNameAsString() << '\n';
        }

        void _doAtNodeEnding() override {
            if (_debug)
                llvm::outs() << "depnEdges: "
                             << util::setToString(_customCPG.getDataDepnEdges(),
                                                  util::numPairToString<unsigned, unsigned>)
                             << '\n';
        }

    public:
        SimplifiedDepnHelper(const unique_ptr <CFG> &cfg, const CustomCPG &customCPG,
                             set<pair<unsigned, unsigned>> &depnEdges, bool debug)
                : AbstractDepnHelper(cfg, debug), _customCPG(customCPG),
                  _writtenVarVec(cfg->size()), _depnEdges(depnEdges) {}
    };
}

#endif //WFG_GENERATOR_SIMPLIFIEDDEPNHELPER_H
