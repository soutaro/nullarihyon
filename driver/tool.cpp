#include <stdio.h>
#include <iostream>

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

#include "analyzer.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory NullarihyonCategory("nullarihyon-core options");

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

static cl::opt<bool> DebugOption("debug",
                                 cl::desc("Enable debug mode (reports more verbosely)"),
                                 cl::cat(NullarihyonCategory));

static cl::list<std::string> FilterOption("filter",
                                          cl::desc("Class name to filter output"),
                                          cl::cat(NullarihyonCategory));

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
    CommonOptionsParser OptionsParser(argc, argv, NullarihyonCategory);
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    
    auto action = new NullCheckAction;
    action->setDebug(DebugOption);
    
    for (auto f : FilterOption) {
        if (*f.begin() == '/' && *(f.end()-1) == '/') {
            f.erase(f.begin());
            f.erase(f.end()-1);
            
            std::shared_ptr<RegexpFilteringClause> clause{ new RegexpFilteringClause(std::regex(f)) };
            action->addFilterClause(clause);
        } else {
            std::shared_ptr<TextFilteringClause> clause{ new TextFilteringClause(f) };
            action->addFilterClause(clause);
        }
    }
    
    NullCheckActionFactory *factory = new NullCheckActionFactory(action);
    
    return Tool.run(factory);
}
