#include <stdio.h>
#include <iostream>

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

#include "analyzer.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory NullabilintCategory("nullabilint-core options");

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

static cl::opt<bool> DebugOption("debug",
                                 cl::desc("Enable debug mode (reports more verbosely)"),
                                 cl::cat(NullabilintCategory));

class NullCheckActionFactory : public FrontendActionFactory {
public:
    explicit NullCheckActionFactory(NullCheckAction *action) {
        Action = std::unique_ptr<NullCheckAction>(action);
    }
    
    FrontendAction *create() override {
        return std::move(Action).get();
    }
    
private:
    std::unique_ptr<NullCheckAction> Action;
};

int main(int argc, const char **argv) {
    CommonOptionsParser OptionsParser(argc, argv, NullabilintCategory);
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    
    auto action = new NullCheckAction;
    action->setDebug(DebugOption);
    
    NullCheckActionFactory *factory = new NullCheckActionFactory(action);
    
    return Tool.run(factory);
}
