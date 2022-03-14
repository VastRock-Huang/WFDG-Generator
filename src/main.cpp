//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"
#include "WFGGen.h"
#include "GlobalInstance.h"

#include <iostream>

using namespace wfg;

int main(int argc, const char **argv) {
    // 构建命令行选项类别
    llvm::cl::OptionCategory opCategory("WFG Generation");
    CommonOptionsParser op(argc, argv, opCategory);
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    int ret = tool.run(newFrontendActionFactory<WFGGenAction>().get());
    for (auto &funcInfo: GlobalInstance::funcInfoList) {
        cout << funcInfo.toString() << endl;
    }
    return ret;
}
