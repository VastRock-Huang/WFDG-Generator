//
// Created by Unravel on 2022/3/14.
//

#include "WFGGen.h"
#include "FuncInfo.h"
#include "Config.h"
#include "GlobalInstance.h"
#include <llvm/ADT/StringRef.h>
#include <string>

namespace wfg {

    vector<pair<unsigned, unsigned >>
    WFGGenConsumer::findSensitiveLines(const SourceLocation &beginLoc, const SourceLocation &endLoc) const {
        FileID fileId = _manager.getMainFileID();
        StringRef funcContent{_manager.getCharacterData(beginLoc),
                           _manager.getCharacterData(endLoc) - _manager.getCharacterData(beginLoc) + 1};
        unsigned fileOffset = _manager.getFileOffset(beginLoc);
        vector<pair<unsigned,unsigned>> result;
        for (size_t i = 0; i < Config::KEYWORDS.size(); ++i) {
            StringRef keyword{Config::KEYWORDS[i]};
            size_t pos = 0;
            pos = funcContent.find(keyword, pos);
            while(pos != StringRef::npos) {
                unsigned line = _manager.getLineNumber(fileId, fileOffset + pos);
                result.emplace_back(line,i);
                pos += keyword.size();
                pos = funcContent.find(keyword, pos);
            }
        }
        return result;
    }

    bool WFGGenConsumer::VisitFunctionDecl(FunctionDecl *funcDecl) {
        if (funcDecl->doesThisDeclarationHaveABody() &&
            _manager.getFileID(funcDecl->getSourceRange().getBegin()) == _manager.getMainFileID()) {
            const string funcName = funcDecl->getQualifiedNameAsString();
            if(Config::matchFuncPrefix(funcName)) {
                FullSourceLoc beginLoc = _context.getFullLoc(funcDecl->getSourceRange().getBegin());
                FullSourceLoc endLoc = _context.getFullLoc(funcDecl->getSourceRange().getEnd());

                FuncInfo funcInfo(funcDecl->getQualifiedNameAsString(), beginLoc.getSpellingLineNumber(),
                                  endLoc.getSpellingLineNumber());
                funcInfo.setSensitiveLines(std::move(findSensitiveLines(beginLoc,endLoc)));
                GlobalInstance::funcInfoList.push_back(funcInfo);
            }
        }
        return true;
    }
}
