#include <clang/Frontend/FrontendActions.h>
#include <vector>

class NullCheckAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, clang::StringRef InFile);
    
    void setDebug(bool debug) {
        Debug = debug;
    }
    
private:
    bool Debug;
};
