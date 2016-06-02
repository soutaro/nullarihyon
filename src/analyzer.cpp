#include <stdio.h>
#include <iostream>
#include <stack>
#include <unordered_map>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "analyzer.h"

using namespace llvm;
using namespace clang;

typedef std::unordered_map<VarDecl *, NullabilityKind> NullabilityKindEnvironment;

class ExprNullabilityCalculator : public StmtVisitor<ExprNullabilityCalculator, NullabilityKind> {
public:
    ExprNullabilityCalculator(ASTContext &context, NullabilityKindEnvironment &env, bool debug) : Context(context), Env(env), Debug(debug) {

    }

    NullabilityKind VisitExpr(Expr *expr) {
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

    NullabilityKind VisitDeclRefExpr(DeclRefExpr *refExpr) {
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

    NullabilityKind VisitObjCIvarRefExpr(ObjCIvarRefExpr *expr) {
        return getNullability(expr->getType());
    }

    NullabilityKind VisitObjCPropertyRefExpr(ObjCPropertyRefExpr *expr) {
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

    NullabilityKind VisitPseudoObjectExpr(PseudoObjectExpr *expr) {
        Expr *result = expr->getResultExpr();
        if (result) {
            result = result->IgnoreParenImpCasts();
            return Visit(result);
        } else {
            return UnexpectedUnspecified(expr);
        }
    }

    NullabilityKind VisitOpaqueValueExpr(OpaqueValueExpr *expr) {
        return Visit(expr->getSourceExpr()->IgnoreParenImpCasts());
    }

    NullabilityKind VisitExprWithCleanups(ExprWithCleanups *expr) {
        return Visit(expr->getSubExpr()->IgnoreParenImpCasts());
    }

    NullabilityKind VisitPredefinedExpr(PredefinedExpr *expr) {
        return NullabilityKind::NonNull;
    }

    NullabilityKind VisitObjCMessageExpr(ObjCMessageExpr *expr) {
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
        
        // If the method is alloc/class and no nullability is given, assume it returns nonnull
        if (name == "alloc" || name == "class" || "init") {
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

    NullabilityKind VisitCallExpr(CallExpr *expr) {
        return getNullability(expr->getType());
    }

    NullabilityKind VisitObjCSubscriptRefExpr(ObjCSubscriptRefExpr *expr) {
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

    NullabilityKind VisitMemberExpr(MemberExpr *expr) {
        return Visit(expr->getBase()->IgnoreParenImpCasts());
    }

    NullabilityKind VisitBinAssign(BinaryOperator *expr) {
        Expr *rhs = expr->getRHS();
        return Visit(rhs->IgnoreParenImpCasts());
    }

    NullabilityKind VisitBinaryOperator(BinaryOperator *expr) {
        return NullabilityKind::NonNull;
    }

    NullabilityKind VisitBinaryConditionalOperator(BinaryConditionalOperator *expr) {
        return Visit(expr->getFalseExpr()->IgnoreParenImpCasts());
    }

    NullabilityKind VisitConditionalOperator(ConditionalOperator *expr) {
        NullabilityKind trueKind = Visit(expr->getTrueExpr()->IgnoreParenImpCasts());
        NullabilityKind falseKind = Visit(expr->getFalseExpr()->IgnoreParenImpCasts());

        if (trueKind == NullabilityKind::NonNull && falseKind == NullabilityKind::NonNull) {
            return NullabilityKind::NonNull;
        } else {
            return NullabilityKind::Nullable;
        }
    }

    NullabilityKind VisitBlockExpr(BlockExpr *expr) {
        return NullabilityKind::NonNull;
    }

    NullabilityKind VisitStmtExpr(StmtExpr *expr) {
        return getNullability(expr->getType());
    }

    NullabilityKind VisitCastExpr(CastExpr *expr) {
        return getNullability(expr->getType());
    }

    NullabilityKind VisitVarDecl(VarDecl *varDecl) {
        NullabilityKindEnvironment::iterator it = Env.find(varDecl);
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

    NullabilityKind getNullability(const QualType &qualType) {
        const Type *type = qualType.getTypePtrOrNull();
        if (type) {
            return type->getNullability(Context).getValueOr(NullabilityKind::Unspecified);
        } else {
            std::cerr << "GetNullability" << std::endl;
            qualType.dump();
            return NullabilityKind::Unspecified;
        }
    }

    NullabilityKind UnexpectedUnspecified(Expr *expr) {
        if (Debug) {
            DiagnosticsEngine &diagEngine = Context.getDiagnostics();
            unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Remark, "%0") ;
            diagEngine.Report(expr->getExprLoc(), diagID) << "Unexpected unspecified";
        }
        return NullabilityKind::Unspecified;
    }
    
    NullabilityKindEnvironment &getEnvironment() {
        return Env;
    }
    
    bool isDebug() {
        return Debug;
    }

private:
    ASTContext &Context;
    NullabilityKindEnvironment &Env;
    bool Debug;
};

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
public:
    MethodBodyChecker(ASTContext &Context, QualType returnType, ExprNullabilityCalculator &Calculator)
        : Context(Context), ReturnType(returnType), NullabilityCalculator(Calculator) {
    }
    
    DiagnosticBuilder WarningReport(SourceLocation location) {
        DiagnosticsEngine &diagEngine = Context.getDiagnostics();
        unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Warning, "%0") ;
        return diagEngine.Report(location, diagID);
    }
    
    bool VisitDeclStmt(DeclStmt *decl) {
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

    bool VisitObjCMessageExpr(ObjCMessageExpr *callExpr) {
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

    bool VisitBinAssign(BinaryOperator *assign) {
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

    bool VisitReturnStmt(ReturnStmt *retStmt) {
        Expr *value = retStmt->getRetValue();
        if (value) {
            if (!isNullabilityCompatible(ReturnType, calculateNullability(value))) {
                WarningReport(value->getExprLoc()) << "Nullability mismatch on return";
            }
        }

        return true;
    }
    
    bool VisitObjCArrayLiteral(ObjCArrayLiteral *literal) {
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
    
    bool VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *literal) {
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

    bool TraverseBlockExpr(BlockExpr *blockExpr) {
        const Type *type = blockExpr->getType().getTypePtr();
        const BlockPointerType *blockType = llvm::dyn_cast<BlockPointerType>(type);
        if (blockType) {
            const FunctionProtoType *funcType = llvm::dyn_cast<FunctionProtoType>(blockType->getPointeeType().getTypePtr());
            if (funcType) {
                QualType retType = funcType->getReturnType();
                MethodBodyChecker checker(Context, retType, NullabilityCalculator);
                checker.TraverseStmt(blockExpr->getBody());
            }
        }

        return true;
    }
    
    bool TraverseIfStmt(IfStmt *ifStmt) {
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
                MethodBodyChecker checker = MethodBodyChecker(Context, ReturnType, calculator);
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

    bool VisitCStyleCastExpr(CStyleCastExpr *expr) {
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

    NullabilityKind calculateNullability(Expr *expr) {
        return NullabilityCalculator.Visit(expr->IgnoreParenImpCasts());
    }

    bool isNullabilityCompatible(const NullabilityKind expectedKind, const NullabilityKind actualKind) {
        if (expectedKind == NullabilityKind::NonNull) {
            if (actualKind != NullabilityKind::NonNull) {
                return false;
            }
        }

        return true;
    }

    bool isNullabilityCompatible(const QualType expectedType, const NullabilityKind actualKind) {
        const Type *type = expectedType.getTypePtrOrNull();
        if (type) {
            NullabilityKind expectedKind = type->getNullability(Context).getValueOr(NullabilityKind::Unspecified);
            return isNullabilityCompatible(expectedKind, actualKind);
        } else {
            return true;
        }
    }

private:
    ASTContext &Context;
    QualType ReturnType;
    ExprNullabilityCalculator &NullabilityCalculator;
};

class VariableNullabilityInference: public RecursiveASTVisitor<VariableNullabilityInference> {
public:
    VariableNullabilityInference(ASTContext &context, NullabilityKindEnvironment &env, ExprNullabilityCalculator &calculator)
        : Context(context), Env(env), Calculator(calculator) {
    }

    bool VisitDeclStmt(DeclStmt *decl) {
        DeclGroupRef group = decl->getDeclGroup();
        DeclGroupRef::iterator it;
        for(it = group.begin(); it != group.end(); it++) {
            VarDecl *vd = llvm::dyn_cast<VarDecl>(*it);
            if (vd) {
                QualType varType = vd->getType();
                const Type *type = varType.getTypePtrOrNull();

                if (type) {
                    if (Env.find(vd) == Env.end()) {
                        Optional<NullabilityKind> kind = type->getNullability(Context);
                        Expr *init = vd->getInit();

                        if (type->isObjCObjectPointerType()) {
                            if (init && llvm::dyn_cast<ImplicitValueInitExpr>(init) == nullptr && !kind.hasValue()) {
                                Env[vd] = Calculator.Visit(init);
                            }
                        }
                    }
                }
            }
        }        

        return true;
    }

    bool VisitObjCForCollectionStmt(ObjCForCollectionStmt *stmt) {
        DeclStmt *decl = llvm::dyn_cast<DeclStmt>(stmt->getElement());
        if (decl) {
            DeclGroupRef group = decl->getDeclGroup();
            DeclGroupRef::iterator it;
            for(it = group.begin(); it != group.end(); it++) {
                VarDecl *vd = llvm::dyn_cast<VarDecl>(*it);
                if (vd) {
                    QualType varType = vd->getType();
                    const Type *type = varType.getTypePtrOrNull();

                    if (type) {
                        Optional<NullabilityKind> kind = type->getNullability(Context);
                        if (!kind.hasValue()) {
                            Env[vd] = NullabilityKind::NonNull;
                        }
                    }
                }
            }        
        }
        return true;
    }

private:
    ASTContext &Context;
    NullabilityKindEnvironment &Env;
    ExprNullabilityCalculator &Calculator;
};

class NullCheckVisitor : public RecursiveASTVisitor<NullCheckVisitor> {
public:
    NullCheckVisitor(ASTContext &context, bool debug) : Context(context), Debug(debug) {}

    bool VisitDecl(Decl *decl) {
        ObjCMethodDecl *methodDecl = llvm::dyn_cast<ObjCMethodDecl>(decl);
        if (methodDecl) {
            if (methodDecl->hasBody()) {
                NullabilityKindEnvironment env;

                ExprNullabilityCalculator calculator(Context, env, Debug);
                VariableNullabilityInference varInference(Context, env, calculator);

                varInference.TraverseStmt(methodDecl->getBody());

                if (Debug) {
                    NullabilityKindEnvironment::iterator it;
                    for (it = env.begin(); it != env.end(); it++) {
                        VarDecl *decl = it->first;
                        NullabilityKind kind = it->second;

                        std::string x = "";
                        switch (kind) {
                            case NullabilityKind::Unspecified:
                                x = "unspecified";
                                break;
                            case NullabilityKind::NonNull:
                                x = "nonnull";
                                break;
                            case NullabilityKind::Nullable:
                                x = "nullable";
                                break;
                        }
                        
                        DiagnosticsEngine &engine = Context.getDiagnostics();
                        unsigned id = engine.getCustomDiagID(DiagnosticsEngine::Remark, "Variable nullability: %0");
                        engine.Report(decl->getLocation(), id) << x;
                    }
                }

                QualType returnType = methodDecl->getReturnType();
                MethodBodyChecker checker(Context, returnType, calculator);
                checker.TraverseStmt(methodDecl->getBody());
            }
        }
        return true;
    }

private:
    ASTContext &Context;
    bool Debug;
};

class NullCheckConsumer : public ASTConsumer {
public:
    explicit NullCheckConsumer(bool debug) : ASTConsumer(), Debug(debug) {
        
    }
    
    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        NullCheckVisitor visitor(Context, Debug);
        visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
    
private:
    bool Debug;
};

std::unique_ptr<clang::ASTConsumer> NullCheckAction::CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile) {
    return std::unique_ptr<ASTConsumer>(new NullCheckConsumer(Debug));
}



