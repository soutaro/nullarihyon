#include "iostream"

#include "analyzer.h"
#include "NullabilityDependencyCalculator.h"

using namespace clang;

enum class NullabilityCompatibility : uint8_t {
    Compatible = 0,
    IncompatibleTopLevel,
    IncompatibleNested
};

DiagnosticBuilder MethodBodyChecker::WarningReport(SourceLocation location, std::set<std::string> &subjects) {
    DiagnosticsEngine &diagEngine = _ASTContext.getDiagnostics();
    DiagnosticsEngine::Level level;
    
    if (!_Filter.empty() && !subjects.empty()) {
        level = DiagnosticsEngine::Ignored;
        for (auto it : _Filter) {
            if (subjects.find(it) != subjects.end()) {
                level = DiagnosticsEngine::Warning;
            }
        }
    } else {
        level = DiagnosticsEngine::Warning;
    }
    
    unsigned diagID = diagEngine.getCustomDiagID(level, "%0");
    return diagEngine.Report(location, diagID);
}

DiagnosticBuilder MethodBodyChecker::WarningReport(clang::SourceLocation location, std::set<const clang::ObjCContainerDecl *> &subjects) {
    std::set<std::string> names;
    
    for (auto decl : subjects) {
        auto name = decl->getNameAsString();
        names.insert(name);
    }
    
    return WarningReport(location, names);
}

NullabilityCompatibility calculateNullabilityCompatibility(ASTContext &context, const Type *lhsType, NullabilityKind lhsKind, const Type *rhsType, NullabilityKind rhsKind) {
    if (lhsKind == NullabilityKind::NonNull) {
        if (rhsKind != NullabilityKind::NonNull) {
            return NullabilityCompatibility::IncompatibleTopLevel;
        }
    }
    
    lhsType = lhsType->getUnqualifiedDesugaredType();
    rhsType = rhsType->getUnqualifiedDesugaredType();
    
    if (lhsType->isBlockPointerType() && rhsType->isBlockPointerType()) {
        const FunctionProtoType *lhsFuncType = llvm::dyn_cast<FunctionProtoType>(llvm::dyn_cast<BlockPointerType>(lhsType)->getPointeeType().IgnoreParens().getTypePtr());
        const FunctionProtoType *rhsFuncType = llvm::dyn_cast<FunctionProtoType>(llvm::dyn_cast<BlockPointerType>(rhsType)->getPointeeType().IgnoreParens().getTypePtr());
        
        if (lhsFuncType && rhsFuncType) {
            const Type *lhsRetType = lhsFuncType->getReturnType().getTypePtr();
            const Type *rhsRetType = rhsFuncType->getReturnType().getTypePtr();
            
            // Covariant
            if (calculateNullabilityCompatibility(context,
                                                  lhsRetType, lhsRetType->getNullability(context).getValueOr(NullabilityKind::Nullable),
                                                  rhsRetType, rhsRetType->getNullability(context).getValueOr(NullabilityKind::Nullable)) != NullabilityCompatibility::Compatible) {
                return NullabilityCompatibility::IncompatibleNested;
            }
            
            unsigned lhsNumParams = lhsFuncType->getNumParams();
            unsigned rhsNumParams = rhsFuncType->getNumParams();
            
            if (lhsNumParams != rhsNumParams) {
                return NullabilityCompatibility::IncompatibleNested;
            }
            
            for (unsigned index = 0; index < lhsNumParams; index++) {
                const Type * lhsParamType = lhsFuncType->getParamType(index).getTypePtr();
                const Type * rhsParamType = rhsFuncType->getParamType(index).getTypePtr();
                
                // Contravariant
                if (calculateNullabilityCompatibility(context,
                                                      rhsParamType, rhsParamType->getNullability(context).getValueOr(NullabilityKind::Nullable),
                                                      lhsParamType, lhsParamType->getNullability(context).getValueOr(NullabilityKind::Nullable)) != NullabilityCompatibility::Compatible) {
                    return NullabilityCompatibility::IncompatibleNested;
                }
            }
        }
    }
    
    return NullabilityCompatibility::Compatible;
}

NullabilityCompatibility calculateNullabilityCompatibility(ASTContext &context, const ExpressionNullability &lhs, const ExpressionNullability &rhs) {
    return calculateNullabilityCompatibility(context, lhs.getType(), lhs.getNullability(), rhs.getType(), rhs.getNullability());
}

std::set<const ObjCContainerDecl *> MethodBodyChecker::subjectDecls(const Expr *expr) {
    std::set<const ObjCContainerDecl *> decls;
    
    MethodUtility utility;
    
    NullabilityDependencyCalculator calculator(_ASTContext);
    auto exprs = calculator.calculate(&_CheckContext.getMethodDecl(), expr);
    
    for (auto e : exprs) {
        const ObjCMessageExpr *messageExpr = llvm::dyn_cast<ObjCMessageExpr>(e->IgnoreParenImpCasts());
        if (messageExpr) {
            auto ds = utility.enumerateContainers(messageExpr);
            decls.insert(ds.begin(), ds.end());
        }
    }
    
    return decls;
}

bool MethodBodyChecker::VisitDeclStmt(DeclStmt *decl) {
    for (auto it : decl->getDeclGroup()) {
        const VarDecl *vd = llvm::dyn_cast<VarDecl>(it);
        if (vd) {
            const Type *declType = vd->getType().getTypePtr();
            NullabilityKind varKind = declType->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified);
            
            const Expr *init = vd->getInit();
            if (init && !llvm::isa<ImplicitValueInitExpr>(init)) {
                ExpressionNullability initNullability = _NullabilityCalculator.calculate(init);
                NullabilityCompatibility compatibility = calculateNullabilityCompatibility(_ASTContext,
                                                                                           declType, varKind,
                                                                                           initNullability.getType(), initNullability.getNullability());
                
                auto subjects = subjectDecls(init);
                subjects.insert(&_CheckContext.getInterfaceDecl());
                
                switch (compatibility) {
                    case NullabilityCompatibility::IncompatibleTopLevel:
                        WarningReport(init->getExprLoc(), subjects) << "Nullability mismatch on variable declaration";
                        break;
                    case NullabilityCompatibility::IncompatibleNested:
                        WarningReport(init->getExprLoc(), subjects) << "Nullability mismatch inside block type on variable declaration";
                        break;
                    case NullabilityCompatibility::Compatible:
                        // ok
                        break;
                }
            }
        }
    }
    
    return true;
}

ObjCContainerDecl *MethodBodyChecker::InterfaceForSelector(const Expr *receiver, Selector selector) {
    const ObjCObjectPointerType *pointerType = llvm::dyn_cast<ObjCObjectPointerType>(receiver->getType().getTypePtr()->getUnqualifiedDesugaredType());
    if (pointerType) {
        ObjCInterfaceDecl *decl = pointerType->getInterfaceDecl();
        if (decl) {
            // SomeClass *
            while (decl) {
                ObjCMethodDecl *method = decl->getMethod(selector, true);
                if (method) {
                    return decl;
                }
                
                for (auto it = decl->protocol_begin(); it != decl->protocol_end(); it++) {
                    ObjCProtocolDecl *protocol = *it;
                    method = protocol->lookupMethod(selector, true);
                    if (method) {
                        return decl;
                    }
                }
                
                decl = decl->getSuperClass();
            }
        } else {
            // id<Protocol>
            unsigned protocols = pointerType->getNumProtocols();
            
            for (unsigned index = 0; index < protocols; index++) {
                ObjCProtocolDecl *protocol = pointerType->getProtocol(index);
                ObjCMethodDecl *method = protocol->lookupMethod(selector, true);
                if (method) {
                    return protocol;
                }
            }
        }
    }
    
    return nullptr;
}

std::string MethodBodyChecker::MethodCallSubjectAsString(const ObjCMessageExpr &expr) {
    if (expr.isInstanceMessage()) {
        const ObjCContainerDecl *interface = expr.getMethodDecl()->getClassInterface();
        if (!interface) {
            // Class interface lookup through method decl may fail, because of Protocols
            interface = InterfaceForSelector(expr.getInstanceReceiver(), expr.getSelector());
        }
        
        if (interface) {
            return interface->getNameAsString();
        }
    }
    
    if (expr.isClassMessage()) {
        QualType type = expr.getClassReceiver();
        return type.getAsString();
    }
    
    return "(not found)";
}

std::string MethodBodyChecker::MethodNameAsString(const ObjCMessageExpr &messageExpr) {
    std::string interfaceName = MethodCallSubjectAsString(messageExpr);
    std::string selector = messageExpr.getSelector().getAsString();
    
    std::string kind;
    if (messageExpr.isInstanceMessage()) {
        kind = "-";
    } else {
        kind = "+";
    }
    
    return kind + "[" + interfaceName + " " + selector + "]";
}

bool MethodBodyChecker::VisitObjCMessageExpr(ObjCMessageExpr *callExpr) {
    const ObjCMethodDecl *decl = callExpr->getMethodDecl();
    if (decl) {
        unsigned index = 0;
        
        std::string name = MethodNameAsString(*callExpr);

        auto subjects = subjectDecls(callExpr);
        subjects.insert(&_CheckContext.getInterfaceDecl());

        for (auto it : decl->params()) {
            const ParmVarDecl *d = it;
            QualType paramQType = d->getType();
            NullabilityKind paramNullability = paramQType.getTypePtr()->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified);
            
            const Expr *arg = callExpr->getArg(index);
            ExpressionNullability argNullability = _NullabilityCalculator.calculate(arg);
            
            NullabilityCompatibility compatibility = calculateNullabilityCompatibility(_ASTContext,
                                                                                       paramQType.getTypePtr(), paramNullability,
                                                                                       argNullability.getType(), argNullability.getNullability());
                                                                                       
            if (compatibility != NullabilityCompatibility::Compatible) {
                std::string message;
                
                switch (compatibility) {
                    case NullabilityCompatibility::IncompatibleTopLevel:
                        message = name + " expects nonnull argument";
                        break;
                    case NullabilityCompatibility::IncompatibleNested:
                        message = "Argument does not have expected block type to " + name;
                        break;
                    case NullabilityCompatibility::Compatible:
                        // ok
                        break;
                }
                
                WarningReport(arg->getExprLoc(), subjects) << message;
            }
            
            index++;
        }
    }
    
    return true;
}

const VarDecl *declRefOrNULL(const Expr *expr) {
    if (llvm::isa<OpaqueValueExpr>(expr)) {
        expr = llvm::dyn_cast<OpaqueValueExpr>(expr)->getSourceExpr();
    }
    
    auto ref = llvm::dyn_cast<DeclRefExpr>(expr->IgnoreParenImpCasts());
    
    if (ref) {
        auto valueDecl = ref->getDecl();
        return llvm::dyn_cast<VarDecl>(valueDecl);
    } else {
        return nullptr;
    }
}

bool MethodBodyChecker::VisitBinAssign(BinaryOperator *assign) {
    const DeclRefExpr *lhs = llvm::dyn_cast<DeclRefExpr>(assign->getLHS());
    const Expr *rhs = assign->getRHS();
    
    if (lhs) {
        auto var = declRefOrNULL(lhs);
        if (var && var->getNameAsString() == "self") {
            // Skip if assignment to self
            return true;
        }
        
        auto lhsNullability = _NullabilityCalculator.calculate(lhs);
        auto rhsNullability = _NullabilityCalculator.calculate(rhs);
        
        auto subjects = subjectDecls(rhs);
        subjects.insert(&_CheckContext.getInterfaceDecl());

        NullabilityCompatibility compatibility = calculateNullabilityCompatibility(_ASTContext, lhsNullability, rhsNullability);

        switch (compatibility) {
            case NullabilityCompatibility::IncompatibleTopLevel:
                WarningReport(rhs->getExprLoc(), subjects) << "Nullability mismatch on assignment";
                break;
            case NullabilityCompatibility::IncompatibleNested:
                WarningReport(rhs->getExprLoc(), subjects) << "Nullability mismatch inside block type on assignment";
                break;
            case NullabilityCompatibility::Compatible:
                // ok
                break;
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitReturnStmt(ReturnStmt *retStmt) {
    const Expr *value = retStmt->getRetValue();
    if (value) {
        const Type *returnType = _CheckContext.getReturnType().getTypePtr();
        NullabilityKind returnKind = returnType->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified);
        
        ExpressionNullability valueNullability = _NullabilityCalculator.calculate(value);
        
        NullabilityCompatibility compatibility = calculateNullabilityCompatibility(_ASTContext,
                                                                                   returnType, returnKind,
                                                                                   valueNullability.getType(), valueNullability.getNullability());
        if (compatibility != NullabilityCompatibility::Compatible) {
            std::string className = _CheckContext.getInterfaceDecl().getNameAsString();
            std::string methodName = _CheckContext.getMethodDecl().getSelector().getAsString();
            std::string kind = _CheckContext.getMethodDecl().isClassMethod() ? "+" : "-";
            std::string name = kind + "[" + className + " " + methodName + "]";
            
            std::string message;
            
            switch (compatibility) {
                case NullabilityCompatibility::IncompatibleTopLevel:
                    if (_CheckContext.getBlockExpr()) {
                        message = "Block in " + name + " expects nonnull to return";
                    } else {
                        message = name + " expects nonnull to return";
                    }
                    break;
                case NullabilityCompatibility::IncompatibleNested:
                    if (_CheckContext.getBlockExpr()) {
                        message = "Does not return expected type for block in " + name;
                    } else {
                        message = "Does not return expected type for " + name;
                    }
                    break;
                case NullabilityCompatibility::Compatible:
                    // ok
                    break;
            }
            
            std::set<const ObjCContainerDecl *> subjects{ &_CheckContext.getInterfaceDecl() };
            
            WarningReport(value->getExprLoc(), subjects) << message;
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitObjCArrayLiteral(ObjCArrayLiteral *literal) {
    unsigned count = literal->getNumElements();
    
    for (unsigned index = 0; index < count; index++) {
        auto element = literal->getElement(index);
        auto nullability = _NullabilityCalculator.calculate(element);
        
        if (!nullability.isNonNull()) {
            std::string name = _CheckContext.getInterfaceDecl().getNameAsString();
            auto subjects = std::set<std::string>{ name };

            WarningReport(element->getExprLoc(), subjects) << "Array element should be nonnull";
        }
    }
    
    return true;
}

bool MethodBodyChecker::VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *literal) {
    unsigned count = literal->getNumElements();
    
    for (unsigned index = 0; index < count; index++) {
        auto element = literal->getKeyValueElement(index);
        
        auto subjects = subjectDecls(literal);
        subjects.insert(&_CheckContext.getInterfaceDecl());

        auto keyNullability = _NullabilityCalculator.calculate(element.Key);
        if (!keyNullability.isNonNull()) {
            WarningReport(element.Key->getExprLoc(), subjects) << "Dictionary key should be nonnull";
        }
        
        auto valueNullability = _NullabilityCalculator.calculate(element.Value);
        if (!valueNullability.isNonNull()) {
            WarningReport(element.Value->getExprLoc(), subjects) << "Dictionary value should be nonnull";
        }
    }
    
    return true;
}

bool MethodBodyChecker::TraverseBlockExpr(BlockExpr *blockExpr) {
    auto blockContext = _CheckContext.newContextForBlock(blockExpr);
    
    VariableNullabilityPropagation prop(_NullabilityCalculator, _VarEnv);
    prop.propagate(blockExpr);
    
    MethodBodyChecker checker(_ASTContext, blockContext, _NullabilityCalculator, _VarEnv, _Filter);
    checker.TraverseStmt(blockExpr->getBody());
    
    return true;
}

bool MethodBodyChecker::VisitBinaryConditionalOperator(clang::BinaryConditionalOperator *S) {
    auto cond = S->getCond();
    
    ExpressionNullability nullability = _NullabilityCalculator.calculate(cond);
    const Type *type = nullability.getType();
    
    if (type->isObjCObjectPointerType() || type->isBlockPointerType() || type->isObjCIdType()) {
        if (nullability.isNonNull()) {
            auto subjects = subjectDecls(cond);
            subjects.insert(&_CheckContext.getInterfaceDecl());
            
            WarningReport(cond->getExprLoc(), subjects) << "Conditional operator looks redundant";
        }
    }

    return true;
}

bool MethodBodyChecker::TraverseIfStmt(IfStmt *ifStmt) {
    auto condition = ifStmt->getCond();
    auto thenStmt = ifStmt->getThen();
    auto elseStmt = ifStmt->getElse();
    
    std::shared_ptr<VariableNullabilityEnvironment> varEnv(_VarEnv->newCopy());
    ExpressionNullabilityCalculator calculator(_ASTContext, varEnv);
    LAndExprChecker exprChecker(_ASTContext, _CheckContext, calculator, varEnv, _Filter);
    
    const VarDecl *decl = declRefOrNULL(condition);
    if (decl) {
        varEnv->set(decl, decl->getType().getTypePtr(), NullabilityKind::NonNull);
    }
    
    exprChecker.TraverseStmt(condition);
    exprChecker.TraverseStmt(thenStmt);
    
    if (elseStmt) {
        TraverseStmt(elseStmt);
    }
    
    return true;
}

bool MethodBodyChecker::TraverseBinLAnd(BinaryOperator *land) {
    std::shared_ptr<VariableNullabilityEnvironment> env(_VarEnv->newCopy());
    ExpressionNullabilityCalculator calculator(_ASTContext, env);
    LAndExprChecker checker = LAndExprChecker(_ASTContext, _CheckContext, calculator, env, _Filter);
    
    checker.TraverseStmt(land);
    
    return true;
}

bool MethodBodyChecker::VisitCStyleCastExpr(CStyleCastExpr *expr) {
    auto srcExpr = expr->getSubExpr();
    
    auto srcNullability = _NullabilityCalculator.calculate(srcExpr);
    auto destNullability = _NullabilityCalculator.calculate(expr);
    
    auto sourceType = srcExpr->getType();
    auto destType = expr->getType();
    
    if (destNullability.isNonNull()) {
        bool castToSame = sourceType.getDesugaredType(_ASTContext) == destType.getDesugaredType(_ASTContext);
        bool castFromID = srcNullability.getType()->isObjCIdType() || srcNullability.getType()->isObjCQualifiedIdType();
        
        std::set<const ObjCContainerDecl *> subjects{ &_CheckContext.getInterfaceDecl() };
        
        if (srcNullability.isNonNull()) {
            if (castToSame || castFromID) {
                WarningReport(expr->getExprLoc(), subjects) << "Redundant cast to nonnull";
            }
        } else {
            if (castToSame || castFromID) {
                // Cast to same type with nonnull is okay
                // Cast from ID is okay
            } else {
                WarningReport(expr->getExprLoc(), subjects) << "Cast on nullability cannot change base type";
            }
        }
    }
    
    return true;
}

bool LAndExprChecker::TraverseUnaryLNot(UnaryOperator *S) {
    std::shared_ptr<VariableNullabilityEnvironment> env(_VarEnv->newCopy());
    ExpressionNullabilityCalculator calculator(_ASTContext, env);
    MethodBodyChecker checker(_ASTContext, _CheckContext, calculator, env, _Filter);
    
    return checker.TraverseStmt(S);
}

bool LAndExprChecker::TraverseBinLOr(BinaryOperator *lor) {
    std::shared_ptr<VariableNullabilityEnvironment> env(_VarEnv->newCopy());
    ExpressionNullabilityCalculator calculator(_ASTContext, env);
    MethodBodyChecker checker(_ASTContext, _CheckContext, calculator, env, _Filter);
    
    return checker.TraverseStmt(lor);
}

bool LAndExprChecker::TraverseBinLAnd(BinaryOperator *land) {
    auto lhs = land->getLHS();
    
    auto lhsDecl = declRefOrNULL(lhs);
    if (lhsDecl) {
        _VarEnv->set(lhsDecl, lhsDecl->getType().getTypePtr(), NullabilityKind::NonNull);
    } else {
        TraverseStmt(lhs);
    }
    
    auto rhs = land->getRHS();
    auto rhsDecl = declRefOrNULL(rhs);
    if (rhsDecl) {
        _VarEnv->set(rhsDecl, rhsDecl->getType().getTypePtr(), NullabilityKind::NonNull);
    } else {
        TraverseStmt(rhs);
    }
    
    return true;
}



