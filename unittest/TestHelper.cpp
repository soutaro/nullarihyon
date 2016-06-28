#include <gtest/gtest.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

void ASTBuilder::matchWithCallback(const DeclarationMatcher &matcher, std::function<void(const MatchFinder::MatchResult &)> function) {
    FunctionMatchCallback callback(function);
    
    MatchFinder finder;
    finder.addMatcher(matcher, &callback);
    
    finder.matchAST(_ASTUnit->getASTContext());
    
    ASSERT_GT(callback.getCount(), 0);
}

void ASTBuilder::matchWithCallback(const StatementMatcher &matcher, std::function<void(const MatchFinder::MatchResult &)> function) {
    FunctionMatchCallback callback(function);
    
    MatchFinder finder;
    finder.addMatcher(matcher, &callback);
    
    finder.matchAST(_ASTUnit->getASTContext());
    
    ASSERT_GT(callback.getCount(), 0);
}

const Expr *ASTBuilder::getTestExpr(std::string name, bool clean) {
    const Expr *expr = nullptr;

    matchWithCallback(varDecl(hasName(name)) .bind("decl"),
                      [&](const MatchFinder::MatchResult &result) {
                          const VarDecl *decl = result.Nodes.getStmtAs<VarDecl>("decl");
                          expr = decl->getInit();
                      });

    if (clean) {
        while (true) {
            auto clean = expr->IgnoreParenImpCasts();
            
            const ExprWithCleanups *cleanup = llvm::dyn_cast<ExprWithCleanups>(clean);
            if (cleanup) {
                clean = cleanup->getSubExpr();
            }
            
            if (clean == expr) {
                break;
            }
            
            expr = clean;
        }
    }
    return expr;
}


ObjCMethodDecl *ASTBuilder::getMethodDecl(std::string name) {
    ObjCMethodDecl *decl = nullptr;
    
    TranslationUnitDecl *translationUnit = _ASTUnit->getASTContext().getTranslationUnitDecl();
    for (auto d : translationUnit->decls()) {
        auto impl = llvm::dyn_cast<ObjCImplDecl>(d);
        if (impl) {
            for (auto m: impl->methods()) {
                if (m->getNameAsString() == name) {
                    decl = m;
                }
            }
        }
    }
    
    return decl;
}

ObjCInterfaceDecl *ASTBuilder::getInterfaceDecl(std::string name) {
    TranslationUnitDecl *translationUnit = _ASTUnit->getASTContext().getTranslationUnitDecl();
    for (auto d : translationUnit->decls()) {
        auto interface = llvm::dyn_cast<ObjCInterfaceDecl>(d);
        if (interface) {
            if (interface->getNameAsString() == name) {
                return interface;
            }
        }
    }
    
    return nullptr;
}

ObjCImplementationDecl *ASTBuilder::getImplementationDecl(std::string name) {
    TranslationUnitDecl *translationUnit = _ASTUnit->getASTContext().getTranslationUnitDecl();
    for (auto d : translationUnit->decls()) {
        auto impl = llvm::dyn_cast<ObjCImplementationDecl>(d);
        if (impl) {
            if (impl->getNameAsString() == name) {
                return impl;
            }
        }
    }
    
    return nullptr;
}