#ifndef NullabilityDependencyCalculator_h
#define NullabilityDependencyCalculator_h

#include <unordered_map>
#include <set>

#include <clang/AST/AST.h>
#include "ExpressionNullabilityCalculator.h"

class NullabilityDependencyExpandVisitor : public clang::ConstStmtVisitor<NullabilityDependencyExpandVisitor, std::set<const clang::Expr *>> {
    clang::ASTContext &_ASTContext;
    const clang::ObjCMethodDecl *_MethodDecl;
    
public:
    explicit NullabilityDependencyExpandVisitor(clang::ASTContext &astContext, const clang::ObjCMethodDecl *methodDecl)
    : _ASTContext(astContext), _MethodDecl(methodDecl) {}
    
    std::set<const clang::Expr *> VisitExpr(const clang::Expr *expr);
    std::set<const clang::Expr *> VisitConditionalOperator(const clang::ConditionalOperator *expr);
    std::set<const clang::Expr *> VisitBinaryConditionalOperator(const clang::BinaryConditionalOperator *expr);
    std::set<const clang::Expr *> VisitObjCMessageExpr(const clang::ObjCMessageExpr *expr);
    std::set<const clang::Expr *> VisitDeclRefExpr(const clang::DeclRefExpr *expr);
};


class NullabilityDependencyCalculator {
    clang::ASTContext &_ASTContext;
    
public:
    explicit NullabilityDependencyCalculator(clang::ASTContext &astContext) : _ASTContext(astContext) {}
    
    std::set<const clang::Expr *> calculate(const clang::ObjCMethodDecl *method, const clang::Expr *expr);
};

#endif
