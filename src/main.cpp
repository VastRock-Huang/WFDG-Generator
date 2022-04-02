//
// Created by Unravel on 2022/3/14.
//

#include "FuncInfo.h"
#include "FuncInfoGen.h"
#include "GlobalInstance.h"
#include "WFGGen.h"
#include "WFGGen/WFG.h"
#include <clang/Basic/Diagnostic.h>
#include <llvm/Support/CommandLine.h>
#include <iostream>

using namespace wfg;
using namespace llvm;

int main(int argc, const char **argv) {
    // 构建命令行选项类别
    cl::OptionCategory opCategory("WFG Generation");
    cl::opt <string> destFunc("f", cl::desc("the function to generate WFG"), cl::value_desc("funcName"),
                              cl::cat(opCategory));
    cl::opt<unsigned> sensitiveLine("s", cl::desc("the sensitive line of the function"),
                                    cl::value_desc("sensitiveLine"),
                                    cl::cat(opCategory));
    cl::opt<bool> showFuncInfo("i", cl::desc("show all FuncInfo"), cl::cat(opCategory));
    CommonOptionsParser op(argc, argv, opCategory);
    GlobalInstance::getInstance().Config.init(destFunc, sensitiveLine);
    ClangTool tool(op.getCompilations(), op.getSourcePathList());
    // 取消错误输出
    class : public DiagnosticConsumer {
    public:
        virtual bool IncludeInDiagnosticCounts()const {
            return false;
        }
    } diagConsumer;
    tool.setDiagnosticConsumer(&diagConsumer);
    int ret = tool.run(newFrontendActionFactory<FuncInfoGenAction>().get());
    for (auto &funcInfo: GlobalInstance::getInstance().FuncInfoList) {
        if (showFuncInfo) {
            cout << "********************* FuncInfo *********************\n"
                 << funcInfo.toString() << endl
                 << "****************************************************\n";
        }
        WFGGenerator wfgGenerator(GlobalInstance::getInstance().Config, funcInfo);
        vector<WFG> wfgs = wfgGenerator.genWFGs();
        for (auto &w: wfgs) {
            cout << w.toString() << endl << endl;
        }
    }
    return ret;
}
