#include "ExpressionNullabilityCalculator.h"

#include <clang/AST/StmtVisitor.h>

using namespace clang;

void VariableNullabilityEnvironment::set(const clang::VarDecl *var, const clang::Type *type, clang::NullabilityKind kind) {
    (*_mapping)[var] = ExpressionNullability(type, kind);
}

ExpressionNullability VariableNullabilityEnvironment::lookup(const clang::VarDecl *var) const {
    if (has(var)) {
        return _mapping->at(var);
    } else {
        auto type = var->getType().getTypePtr();
        auto kind = type->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified);
        return ExpressionNullability(type, kind);
    }
}

bool VariableNullabilityEnvironment::has(const clang::VarDecl *var) const {
    return _mapping->find(var) != _mapping->end();
}

class ExpressionNullabilityCalculationVisitor : public ConstStmtVisitor<ExpressionNullabilityCalculationVisitor, ExpressionNullability> {
    ASTContext &_ASTContext;
    std::shared_ptr<const VariableNullabilityEnvironment> _VarEnv;
    
public:
    explicit ExpressionNullabilityCalculationVisitor(ASTContext &astContext, std::shared_ptr<const VariableNullabilityEnvironment> varEnv)
        : _ASTContext(astContext), _VarEnv(varEnv) {}
    
    ExpressionNullability recursion(const Expr *expr) {
        return Visit(expr->IgnoreParenImpCasts());
    }
    
    ExpressionNullability VisitExpr(const Expr *expr) {
        const Type *type = expr->getType().getTypePtr();
        
        if (mayExprNullable(expr)) {
            return ExpressionNullability(type, type->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified));
        } else {
            return ExpressionNullability(type, NullabilityKind::NonNull);
        }
    }
    
    /**
     Return false if expr cannot be nullable.
     */
    bool mayExprNullable(const Expr *expr) {
        const Type *type = expr->getType().getTypePtr();
        
        if (llvm::isa<ObjCStringLiteral>(expr)
            || llvm::isa<ObjCBoxedExpr>(expr)
            || llvm::isa<ObjCArrayLiteral>(expr)
            || llvm::isa<ObjCDictionaryLiteral>(expr)
            ) {
            return false;
        }
        
        return type->isPointerType() || type->isObjCObjectPointerType() || type->isObjCIdType();
    }
    
    ExpressionNullability VisitDeclRefExpr(const DeclRefExpr *ref) {
        std::string name = ref->getNameInfo().getAsString();
        const Type *type = ref->getType().getTypePtr();
        
        if (name == "self") {
            // Assume self is nonnull
            return ExpressionNullability(type, NullabilityKind::NonNull);
        } else {
            const VarDecl *varDecl = llvm::dyn_cast<VarDecl>(ref->getDecl());
            if (varDecl && _VarEnv->has(varDecl)) {
                return _VarEnv->lookup(varDecl);
            } else {
                return ExpressionNullability(type, type->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified));
            }
        }
    }
    
    ExpressionNullability VisitExprWithCleanups(const ExprWithCleanups *expr) {
        return recursion(expr->getSubExpr());
    }
    
    ExpressionNullability VisitObjCMessageExpr(const ObjCMessageExpr *messageExpr) {
        const Type *type = messageExpr->getType().getTypePtr();
        std::string selector = messageExpr->getSelector().getAsString();

        auto receiverKind = messageExpr->getReceiverKind();
        
        if (messageExpr->isInstanceMessage()) {
            ExpressionNullability receiverNullability;
            
            if (receiverKind == ObjCMessageExpr::ReceiverKind::Instance) {
                const Expr *receiver = messageExpr->getInstanceReceiver();
                receiverNullability = recursion(receiver);
            }
            if (receiverKind == ObjCMessageExpr::ReceiverKind::SuperInstance) {
                const Type *receiverType = messageExpr->getReceiverType().getTypePtr();
                receiverNullability = ExpressionNullability(receiverType, NullabilityKind::NonNull);
            }
            
            if (!receiverNullability.isNonNull()) {
                return ExpressionNullability(type, NullabilityKind::Nullable);
            }
            
            const Type *retType = messageExpr->getMethodDecl()->getReturnType().getTypePtr();
            
            NullabilityKind defaultKind = NullabilityKind::Unspecified;
            
            std::set<std::string> nonnullMethods;
            nonnullMethods.insert("class");
            nonnullMethods.insert("init");
            nonnullMethods.insert("alloc");
            
            if (nonnullMethods.find(selector) != nonnullMethods.end()) {
                defaultKind = NullabilityKind::NonNull;
            }
            
            return ExpressionNullability(retType, retType->getNullability(_ASTContext).getValueOr(defaultKind));
        }
        
        if (messageExpr->isClassMessage()) {
            const Type *retType = messageExpr->getMethodDecl()->getReturnType().getTypePtr();
            
            NullabilityKind defaultKind = NullabilityKind::Unspecified;
            
            std::set<std::string> nonnullMethods;
            nonnullMethods.insert("class");
            nonnullMethods.insert("alloc");
            
            if (nonnullMethods.find(selector) != nonnullMethods.end()) {
                defaultKind = NullabilityKind::NonNull;
            }
            
            return ExpressionNullability(retType, retType->getNullability(_ASTContext).getValueOr(defaultKind));
        }
        
        return ExpressionNullability(type, type->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified));
    }
    
    ExpressionNullability VisitBinaryConditionalOperator(const BinaryConditionalOperator *expr) {
        return recursion(expr->getFalseExpr());
    }
    
    ExpressionNullability VisitConditionalOperator(const ConditionalOperator *expr) {
        ExpressionNullability trueNullability = recursion(expr->getTrueExpr());
        ExpressionNullability falseNullability = recursion(expr->getFalseExpr());
        
        NullabilityKind kind = trueNullability.isNonNull() && falseNullability.isNonNull() ? NullabilityKind::NonNull : NullabilityKind::Nullable;
        return ExpressionNullability(expr->getType().getTypePtr(), kind);
    }

    ExpressionNullability VisitPseudoObjectExpr(const PseudoObjectExpr *expr) {
        return recursion(expr->getResultExpr());
    }
    
    ExpressionNullability VisitOpaqueValueExpr(const OpaqueValueExpr *expr) {
        return recursion(expr->getSourceExpr());
    }
    
    ExpressionNullability VisitBinAssign(const BinaryOperator *expr) {
        return recursion(expr->getRHS());
    }
    
    ExpressionNullability VisitObjCSelectorExpr(const ObjCSelectorExpr *expr) {
        return ExpressionNullability(expr->getType().getTypePtr(), NullabilityKind::NonNull);
    }
};

ExpressionNullability ExpressionNullabilityCalculator::calculate(const clang::Expr *expr) {
    auto visitor = ExpressionNullabilityCalculationVisitor(_ASTContext, _VarEnv);
    return visitor.Visit(expr->IgnoreParenImpCasts());
}
