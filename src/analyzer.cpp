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

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
public:
    MethodBodyChecker(ASTContext &Context, QualType returnType, ExprNullabilityCalculator &Calculator, NullabilityKindEnvironment &env)
        : Context(Context), ReturnType(returnType), NullabilityCalculator(Calculator), Env(env) {
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
                MethodBodyChecker checker(Context, retType, NullabilityCalculator, Env);
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

protected:
    ASTContext &Context;
    QualType ReturnType;
    ExprNullabilityCalculator &NullabilityCalculator;
    NullabilityKindEnvironment &Env;
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
                MethodBodyChecker checker(Context, returnType, calculator, env);
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



