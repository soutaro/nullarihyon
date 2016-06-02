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



