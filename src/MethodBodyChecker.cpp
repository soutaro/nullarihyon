#include "iostream"

#include "analyzer.h"

using namespace clang;

bool MethodBodyChecker::VisitDeclStmt(DeclStmt *decl) {
    DeclGroupRef group = decl->getDeclGroup();
    DeclGroupRef::iterator it;
    for(it = group.begin(); it != group.end(); it++) {
        VarDecl *vd = llvm::dyn_cast<VarDecl>(*it);
        if (vd) {
            NullabilityKind varKind = NullabilityCalculator.VisitVarDecl(vd);
            
            Expr *init = vd->getInit();
            if (init && llvm::dyn_cast<ImplicitValueInitExpr>(init) == nullptr) {
                if (!isNullabilityCompatible(varKind, calculateNullability(init))) {
                    WarningReport(init->getExprLoc()) << "Nullability mismatch on variable declaration";
                }
            }
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitObjCMessageExpr(ObjCMessageExpr *callExpr) {
    ObjCMethodDecl *decl = callExpr->getMethodDecl();
    if (decl) {
        unsigned index = 0;
        
        ObjCMethodDecl::param_iterator it;
        for (it = decl->param_begin(); it != decl->param_end(); it++) {
            ParmVarDecl *d = *it;
            QualType paramQType = d->getType();
            
            Expr *arg = callExpr->getArg(index);
            NullabilityKind argNullability = calculateNullability(arg);
            
            if (!isNullabilityCompatible(paramQType, argNullability)) {
                std::string interfaceName = decl->getClassInterface()->getNameAsString();
                std::string selector = decl->getSelector().getAsString();
                std::string kind;
                ObjCMessageExpr::ReceiverKind receiverKind = callExpr->getReceiverKind();
                if (receiverKind == ObjCMessageExpr::ReceiverKind::Instance || receiverKind == ObjCMessageExpr::ReceiverKind::SuperInstance) {
                    kind = "-";
                } else {
                    kind = "+";
                }
                
                std::string message = kind + "[" + interfaceName + " " +selector + "] expects nonnull argument";
                
                WarningReport(arg->getExprLoc()) << message;
            }
            
            index++;
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitBinAssign(BinaryOperator *assign) {
    DeclRefExpr *lhs = llvm::dyn_cast<DeclRefExpr>(assign->getLHS());
    Expr *rhs = assign->getRHS();
    
    if (lhs) {
        NullabilityKind lhsNullability = calculateNullability(lhs);
        NullabilityKind rhsNullability = calculateNullability(rhs);
        
        if (!isNullabilityCompatible(lhsNullability, rhsNullability)) {
            WarningReport(rhs->getExprLoc()) << "Nullability mismatch on assignment";
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitReturnStmt(ReturnStmt *retStmt) {
    Expr *value = retStmt->getRetValue();
    if (value) {
        if (!isNullabilityCompatible(ReturnType, calculateNullability(value))) {
            WarningReport(value->getExprLoc()) << "Nullability mismatch on return";
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitObjCArrayLiteral(ObjCArrayLiteral *literal) {
    unsigned count = literal->getNumElements();
    
    for (unsigned index = 0; index < count; index++) {
        Expr *element = literal->getElement(index);
        NullabilityKind elementKind = calculateNullability(element);
        
        if (elementKind != NullabilityKind::NonNull) {
            WarningReport(element->getExprLoc()) << "Array element should be nonnull";
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *literal) {
    unsigned count = literal->getNumElements();
    
    for (unsigned index = 0; index < count; index++) {
        auto element = literal->getKeyValueElement(index);
        
        auto keyKind = calculateNullability(element.Key);
        if (keyKind != NullabilityKind::NonNull) {
            WarningReport(element.Key->getExprLoc()) << "Dictionary key should be nonnull";
        }
        
        auto valueKind = calculateNullability(element.Value);
        if (valueKind != NullabilityKind::NonNull) {
            WarningReport(element.Value->getExprLoc()) << "Dictionary value should be nonnull";
        }
    }
    
    return true;
}

bool MethodBodyChecker::TraverseBlockExpr(BlockExpr *blockExpr) {
    const Type *type = blockExpr->getType().getTypePtr();
    const BlockPointerType *blockType = llvm::dyn_cast<BlockPointerType>(type);
    if (blockType) {
        const FunctionProtoType *funcType = llvm::dyn_cast<FunctionProtoType>(blockType->getPointeeType().getTypePtr());
        if (funcType) {
            QualType retType = funcType->getReturnType();
            MethodBodyChecker checker(Context, retType, NullabilityCalculator, Env);
            checker.TraverseStmt(blockExpr->getBody());
        }
    }
    
    return true;
}

bool MethodBodyChecker::TraverseIfStmt(IfStmt *ifStmt) {
    Expr *condition = ifStmt->getCond();
    Stmt *thenStmt = ifStmt->getThen();
    Stmt *elseStmt = ifStmt->getElse();
    
    DeclRefExpr *refExpr = llvm::dyn_cast<DeclRefExpr>(condition->IgnoreParenImpCasts());
    if (refExpr) {
        VarDecl *varDecl = llvm::dyn_cast<VarDecl>(refExpr->getDecl());
        if (varDecl) {
            const Type *type = varDecl->getType().getTypePtrOrNull();
            NullabilityKindEnvironment environment = NullabilityCalculator.getEnvironment();
            if (type && type->isObjectType()) {
                environment[varDecl] = NullabilityKind::NonNull;
            }
            ExprNullabilityCalculator calculator(Context, environment, NullabilityCalculator.isDebug());
            MethodBodyChecker checker = MethodBodyChecker(Context, ReturnType, calculator, environment);
            checker.TraverseStmt(thenStmt);
        } else {
            this->TraverseStmt(thenStmt);
        }
        
        if (elseStmt) {
            this->TraverseStmt(elseStmt);
        }
        
        return true;
    } else {
        return RecursiveASTVisitor::TraverseIfStmt(ifStmt);
    }
}

bool MethodBodyChecker::VisitCStyleCastExpr(CStyleCastExpr *expr) {
    Expr *subExpr = expr->getSubExpr();
    const Type *subExprType = subExpr->getType().getTypePtrOrNull();
    
    const Type *type = expr->getType().getTypePtrOrNull();
    
    if (subExprType && type) {
        Optional<NullabilityKind> subExprKind = subExprType->getNullability(Context);
        Optional<NullabilityKind> exprKind = type->getNullability(Context);
        
        if (exprKind.getValueOr(NullabilityKind::Unspecified) == NullabilityKind::NonNull) {
            if (subExprKind.getValueOr(NullabilityKind::Unspecified) != NullabilityKind::NonNull) {
                if (subExpr->getType().getDesugaredType(Context) != expr->getType().getDesugaredType(Context)) {
                    if (subExprType->isObjCIdType() || subExprType->isObjCQualifiedIdType()) {
                        // cast from id is okay
                    } else {
                        WarningReport(expr->getExprLoc()) << "Cast on nullability cannot change base type";
                    }
                }
            }
        }
    }
    
    return true;
}
