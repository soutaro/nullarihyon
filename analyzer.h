#include <clang/Frontend/FrontendActions.h>

class NullCheckAction : public clang::ASTFrontendAction {
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, clang::StringRef InFile);
};
