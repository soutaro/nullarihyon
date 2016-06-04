#include "iostream"

#include "analyzer.h"

using namespace clang;

NullabilityKind ExprNullabilityCalculator::VisitExpr(Expr *expr) {
    if (llvm::dyn_cast<ObjCStringLiteral>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<ObjCBoxedExpr>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<ObjCArrayLiteral>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<ObjCDictionaryLiteral>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<ObjCSelectorExpr>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<IntegerLiteral>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<StringLiteral>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<ObjCBoolLiteralExpr>(expr)) return NullabilityKind::NonNull;
    if (llvm::dyn_cast<UnaryOperator>(expr)) return NullabilityKind::NonNull;
    
    const Type *type = expr->getType().getTypePtrOrNull();
    if (type && type->isObjectType()) {
        if (Debug) {
            DiagnosticsEngine &diagEngine = Context.getDiagnostics();
            unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Remark, "%0") ;
            diagEngine.Report(expr->getExprLoc(), diagID) << "VisitExpr: unknown expr";
        }
        return type->getNullability(Context).getValueOr(NullabilityKind::Unspecified);
    } else {
        return NullabilityKind::NonNull;
    }
}


NullabilityKind ExprNullabilityCalculator::VisitDeclRefExpr(DeclRefExpr *refExpr) {
    std::string name = refExpr->getNameInfo().getAsString();
    if (name == "self" || name == "super") {
        // Assume self is nonnull
        return NullabilityKind::NonNull;
    } else {
        ValueDecl *decl = refExpr->getDecl();
        if (decl) {
            VarDecl *varDecl = llvm::dyn_cast<VarDecl>(decl);
            if (varDecl && Env.find(varDecl) != Env.end()) {
                return Env.at(varDecl);
            } else {
                return getNullability(decl->getType());
            }
        } else {
            return UnexpectedUnspecified(refExpr);
        }
    }
}

NullabilityKind ExprNullabilityCalculator::VisitObjCPropertyRefExpr(ObjCPropertyRefExpr *expr) {
    // FIXME: Ignore receivers nullability
    
    if (expr->isImplicitProperty()) {
        const ObjCMethodDecl *getter = expr->getImplicitPropertyGetter();
        return getNullability(getter->getReturnType());
    }
    if (expr->isExplicitProperty()) {
        ObjCPropertyDecl *decl = expr->getExplicitProperty();
        return getNullability(decl->getType());
    }
    
    return UnexpectedUnspecified(expr);
}

NullabilityKind ExprNullabilityCalculator::VisitPseudoObjectExpr(PseudoObjectExpr *expr) {
    Expr *result = expr->getResultExpr();
    if (result) {
        result = result->IgnoreParenImpCasts();
        return Visit(result);
    } else {
        return UnexpectedUnspecified(expr);
    }
}

NullabilityKind ExprNullabilityCalculator::VisitObjCMessageExpr(ObjCMessageExpr *expr) {
    ObjCMessageExpr::ReceiverKind receiverKind = expr->getReceiverKind();
    ObjCMethodDecl *decl = expr->getMethodDecl();
    Selector selector = expr->getSelector();
    std::string name = selector.getAsString();
    bool isReceiverNullable = getNullability(expr->getReceiverType()) != NullabilityKind::NonNull;
    
    if (receiverKind == ObjCMessageExpr::ReceiverKind::Instance) {
        Expr *receiver = expr->getInstanceReceiver();
        NullabilityKind receiverNullability = Visit(receiver->IgnoreParenImpCasts());
        isReceiverNullable = (receiverNullability != NullabilityKind::NonNull);
    }
    
    if (receiverKind == ObjCMessageExpr::ReceiverKind::Class) {
        isReceiverNullable = false;
    }
    
    if (receiverKind == ObjCMessageExpr::ReceiverKind::SuperInstance) {
        isReceiverNullable = false;
    }
    
    // Shortcut when receiver is nullable
    if (isReceiverNullable) {
        return NullabilityKind::Nullable;
    }
    
    // alloc, class, and init are assumed to return nonnull
    if (name == "alloc" || name == "class" || name == "init") {
        Optional<NullabilityKind> kind;
        if (decl) {
            const Type *type = decl->getReturnType().getTypePtrOrNull();
            kind = type->getNullability(Context);
        }
        
        if (!kind.hasValue()) {
            return NullabilityKind::NonNull;
        }
    }
    
    if (decl) {
        return getNullability(decl->getReturnType());
    } else {
        return NullabilityKind::Unspecified;
    }
}

NullabilityKind ExprNullabilityCalculator::VisitObjCSubscriptRefExpr(ObjCSubscriptRefExpr *expr) {
    Expr *receiver = expr->getBaseExpr();
    NullabilityKind receiverKind = Visit(receiver->IgnoreParenImpCasts());
    if (receiverKind != NullabilityKind::NonNull) {
        return NullabilityKind::Nullable;
    }
    
    ObjCMethodDecl *decl = expr->getAtIndexMethodDecl();
    if (decl) {
        return getNullability(decl->getReturnType());
    } else {
        return UnexpectedUnspecified(expr);
    }
}

NullabilityKind ExprNullabilityCalculator::VisitBinAssign(BinaryOperator *expr) {
    Expr *rhs = expr->getRHS();
    return Visit(rhs->IgnoreParenImpCasts());
}

NullabilityKind ExprNullabilityCalculator::VisitConditionalOperator(ConditionalOperator *expr) {
    NullabilityKind trueKind = Visit(expr->getTrueExpr()->IgnoreParenImpCasts());
    NullabilityKind falseKind = Visit(expr->getFalseExpr()->IgnoreParenImpCasts());

    if (trueKind == NullabilityKind::NonNull && falseKind == NullabilityKind::NonNull) {
        return NullabilityKind::NonNull;
    } else {
        return NullabilityKind::Nullable;
    }
}

NullabilityKind ExprNullabilityCalculator::VisitVarDecl(VarDecl *varDecl) {
    NullabilityKindEnvironment::const_iterator it = Env.find(varDecl);
    if (it != Env.end()) {
        return it->second;
    } else {
        const Type *type = varDecl->getType().getTypePtrOrNull();
        if (type) {
            return type->getNullability(Context).getValueOr(NullabilityKind::Unspecified);
        } else {
            return NullabilityKind::Unspecified;
        }
    }
}


NullabilityKind ExprNullabilityCalculator::getNullability(const QualType &qualType) {
    const Type *type = qualType.getTypePtrOrNull();
    if (type) {
        return type->getNullability(Context).getValueOr(NullabilityKind::Unspecified);
    } else {
        std::cerr << "GetNullability" << std::endl;
        qualType.dump();
        return NullabilityKind::Unspecified;
    }
}

NullabilityKind ExprNullabilityCalculator::UnexpectedUnspecified(Expr *expr) {
    if (Debug) {
        DiagnosticsEngine &diagEngine = Context.getDiagnostics();
        unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Remark, "%0") ;
        diagEngine.Report(expr->getExprLoc(), diagID) << "Unexpected unspecified";
    }
    return NullabilityKind::Unspecified;
}
