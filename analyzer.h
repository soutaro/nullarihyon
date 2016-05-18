#include <clang/Frontend/FrontendActions.h>
#include <vector>

#include "ErrorMessage.h"

class NullCheckAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, clang::StringRef InFile);
  virtual bool BeginSourceFileAction(clang::CompilerInstance &CI, clang::StringRef Filename);
  virtual void EndSourceFileAction();

private:
  std::vector<ErrorMessage> *Errors;
};
