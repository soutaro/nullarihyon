#include "NullabilityDependencyCalculator.h"

using namespace clang;

class VariableReferenceExpander : public RecursiveASTVisitor<VariableReferenceExpander> {
    std::set<const Expr *> &_Set;
    const VarDecl *_Decl;
    
public:
    
    explicit VariableReferenceExpander(const VarDecl *decl, std::set<const Expr *> &set) : _Set(set), _Decl(decl) {}
    
    bool VisitDeclStmt(const DeclStmt *declStmt) {
        for (auto decl : declStmt->getDeclGroup()) {
            if (decl == _Decl) {
                auto varDecl = llvm::dyn_cast<VarDecl>(decl);
                const Expr *init = varDecl->getInit();
                
                if (init && !llvm::isa<ImplicitValueInitExpr>(init)) {
                    _Set.insert(init->IgnoreParenImpCasts());
                }
            }
        }
        
        return true;
    }
    
    bool VisitBinAssign(const BinaryOperator *assign) {
        DeclRefExpr *lhs = llvm::dyn_cast<DeclRefExpr>(assign->getLHS());
        if (lhs && lhs->getDecl() == _Decl) {
            _Set.insert(assign->getRHS()->IgnoreParenImpCasts());
        }
        return true;
    }
};

std::set<const clang::Expr *> NullabilityDependencyExpandVisitor::VisitExpr(const clang::Expr *expr) {
    std::set<const Expr *> set;
    
    const ExprWithCleanups *exprWithCleanups = llvm::dyn_cast<ExprWithCleanups>(expr);
    if (exprWithCleanups) {
        set.insert(exprWithCleanups->getSubExpr());
        return set;
    }
    
    if (const PseudoObjectExpr *pseudoObjectExpr = llvm::dyn_cast<PseudoObjectExpr>(expr)) {
        set.insert(pseudoObjectExpr->getResultExpr());
        return set;
    }
    
    if (const OpaqueValueExpr *opaqueValueExpr = llvm::dyn_cast<OpaqueValueExpr>(expr)) {
        set.insert(opaqueValueExpr->getSourceExpr());
        return set;
    }

    set.insert(expr);
    
    return set;
}

std::set<const clang::Expr *> NullabilityDependencyExpandVisitor::VisitConditionalOperator(const clang::ConditionalOperator *expr) {
    std::set<const Expr *> set;
    
    set.insert(expr->getTrueExpr());
    set.insert(expr->getFalseExpr());

    return set;
}

std::set<const clang::Expr *> NullabilityDependencyExpandVisitor::VisitBinaryConditionalOperator(const clang::BinaryConditionalOperator *expr) {
    std::set<const Expr *> set;
    
    set.insert(expr->getFalseExpr());
    
    return set;
}

std::set<const clang::Expr *> NullabilityDependencyExpandVisitor::VisitObjCMessageExpr(const clang::ObjCMessageExpr *expr) {
    std::set<const Expr *> set;
    
    const Expr *receiver = expr->getInstanceReceiver();
    if (receiver) {
        set.insert(receiver);
    }
    
    const ObjCMethodDecl *method = expr->getMethodDecl();
    NullabilityKind kind = method->getReturnType().getTypePtr()->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified);
    
    if (kind != NullabilityKind::NonNull) {
        set.insert(expr);
    }
    
    return set;
}

std::set<const clang::Expr *> NullabilityDependencyExpandVisitor::VisitDeclRefExpr(const clang::DeclRefExpr *expr) {
    std::set<const Expr *> set;
    
    const VarDecl *varDecl = llvm::dyn_cast<VarDecl>(expr->getDecl());
    if (varDecl) {
        VariableReferenceExpander e(varDecl, set);
        e.TraverseStmt(_MethodDecl->getBody());
    }
    
    return set;
}

std::set<const Expr *> NullabilityDependencyCalculator::calculate(const clang::ObjCMethodDecl *method, const clang::Expr *expr) {
    NullabilityDependencyExpandVisitor visitor(_ASTContext, method);
    std::set<const Expr *> set{expr};
    
    while (true) {
        std::set<const Expr *> nextSet = set;
        
        for (const Expr *e : set) {
            auto set = visitor.Visit(e->IgnoreParenImpCasts());
            nextSet.insert(set.begin(), set.end());
        }
        
        if (nextSet == set) {
            break;
        } else {
            set = nextSet;
        }
    }
    
    return set;
 }