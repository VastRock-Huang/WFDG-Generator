//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include "GlobalInstance.h"
#include "WFGGen.h"
#include "WFGGen/WFG.h"
#include <iostream>

using namespace wfg;


int main(int argc, const char **argv) {
    // 构建命令行选项类别
    llvm::cl::OptionCategory opCategory("WFG Generation");
    CommonOptionsParser op(argc, argv, opCategory);
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    int ret = tool.run(newFrontendActionFactory<FuncInfoGenAction>().get());
    for (auto &funcInfo: GlobalInstance::FuncInfoList) {
        vector<WFG> wfgs = genWFGs(funcInfo);
        for (auto &w: wfgs) {
            cout << w.toString() << endl;
        }
    }
    return ret;
}
