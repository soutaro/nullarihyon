#include <stdio.h>
#include <iostream>
#include <stack>
#include <unordered_map>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "analyzer.h"
#include "ErrorMessage.h"

using namespace llvm;
using namespace clang;

typedef std::unordered_map<VarDecl *, NullabilityKind> NullabilityKindEnvironment;

class VariableNullabilityInference: public RecursiveASTVisitor<VariableNullabilityInference> {
public:
  VariableNullabilityInference(ASTContext &context, NullabilityKindEnvironment &env)
    : Context(context), Env(env) {
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
            Env[vd] = kind.getValueOr(NullabilityKind::Unspecified);
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
};

class ExprNullabilityCalculator : public StmtVisitor<ExprNullabilityCalculator, NullabilityKind> {
public:
  ExprNullabilityCalculator(ASTContext &context, NullabilityKindEnvironment &env) : Context(context), Env(env) {

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
      std::cerr << "VisitExpr; unknown expr" << std::endl;
      expr->dump();
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

    // Shortcut when receiver is nullable
    if (isReceiverNullable) {
      return NullabilityKind::Nullable;
    }

    if (decl) {
      // If the method is alloc/class and no nullability is given, assume it returns nonnull
      if (name == "alloc" || name == "class") {
        const Type *type = decl->getReturnType().getTypePtrOrNull();
        if (type) {
          Optional<NullabilityKind> kind = type->getNullability(Context);
          if (!kind.hasValue()) {
            return NullabilityKind::NonNull;
          }
        }
      }

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
    return getNullability(expr->getType());
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
      return NullabilityKind::Unspecified;
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
    std::cerr << "Unexpected unspecified:" << std::endl;
    expr->dump();
    return NullabilityKind::Unspecified;
  }

private:
  ASTContext &Context;
  NullabilityKindEnvironment &Env;
};

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
public:
  MethodBodyChecker(ASTContext &Context, std::vector<ErrorMessage> &errors, QualType returnType, ExprNullabilityCalculator &Calculator)
    : Context(Context), Errors(errors), ReturnType(returnType), NullabilityCalculator(Calculator) {
  }

  bool VisitDeclStmt(DeclStmt *decl) {
    DeclGroupRef group = decl->getDeclGroup();
    DeclGroupRef::iterator it;
    for(it = group.begin(); it != group.end(); it++) {
      VarDecl *vd = llvm::dyn_cast<VarDecl>(*it);
      if (vd) {
        NullabilityKind varKind = NullabilityCalculator.VisitVarDecl(vd);

        Expr *init = vd->getInit();
        if (init) {
          if (!isNullabilityCompatible(varKind, calculateNullability(init))) {
            std::string loc = vd->getLocation().printToString(Context.getSourceManager());
            std::ostringstream s;
            s << "var decl (" << vd->getNameAsString() << ")";
            Errors.push_back(ErrorMessage(loc, s));
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
          std::string loc = arg->getExprLoc().printToString(Context.getSourceManager());
          std::ostringstream s;
          s << "method call (" << decl->getNameAsString() << ", " << index << ")";
          Errors.push_back(ErrorMessage(loc, s));
        }

        index++;
      }
    }

    return true;
  }

  bool VisitBinAssign(BinaryOperator *assign) {
    Expr *lhs = assign->getLHS();
    Expr *rhs = assign->getRHS();

    NullabilityKind lhsNullability = calculateNullability(lhs);
    NullabilityKind rhsNullability = calculateNullability(rhs);

    if (!isNullabilityCompatible(lhsNullability, rhsNullability)) {
      std::string loc = rhs->getExprLoc().printToString(Context.getSourceManager());
      std::ostringstream s;
      s << "assignment";
      Errors.push_back(ErrorMessage(loc, s));
    }

    return true;
  }

  bool VisitReturnStmt(ReturnStmt *retStmt) {
    Expr *value = retStmt->getRetValue();
    if (value) {
      if (!isNullabilityCompatible(ReturnType, calculateNullability(value))) {
        std::string loc = value->getExprLoc().printToString(Context.getSourceManager());
        std::ostringstream s;
        s << "return";
        Errors.push_back(ErrorMessage(loc, s));
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
        MethodBodyChecker checker(Context, Errors, retType, NullabilityCalculator);
        checker.TraverseStmt(blockExpr->getBody());
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
  std::vector<ErrorMessage> &Errors;
  QualType ReturnType;
  ExprNullabilityCalculator &NullabilityCalculator;
};

class NullCheckVisitor : public RecursiveASTVisitor<NullCheckVisitor> {
public:
  NullCheckVisitor(ASTContext &context, std::vector<ErrorMessage> &errors) : Context(context), Errors(errors) {}

  bool VisitDecl(Decl *decl) {
    ObjCMethodDecl *methodDecl = llvm::dyn_cast<ObjCMethodDecl>(decl);
    if (methodDecl) {
      if (methodDecl->hasBody()) {
        NullabilityKindEnvironment env;
        VariableNullabilityInference varInference(Context, env);
        varInference.TraverseStmt(methodDecl->getBody());

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

          std::cerr << decl->getNameAsString() << ": " << x << std::endl;
        }

        QualType returnType = methodDecl->getReturnType();
        ExprNullabilityCalculator calculator(Context, env);

        MethodBodyChecker checker(Context, Errors, returnType, calculator);
        checker.TraverseStmt(methodDecl->getBody());
      }
    }
    return true;
  }

private:
  ASTContext &Context;
  std::vector<ErrorMessage> &Errors;
};

class NullCheckConsumer : public ASTConsumer {
public:
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    std::vector<ErrorMessage> errors;

    NullCheckVisitor visitor(Context, errors);
    visitor.TraverseDecl(Context.getTranslationUnitDecl());

    std::vector<ErrorMessage>::iterator it;
    for (it = errors.begin(); it != errors.end(); it++) {
      std::cout << it->getLocation() << " " << it->getMessage() << std::endl;
    }    
  }
};

std::unique_ptr<clang::ASTConsumer> NullCheckAction::CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile) {
  return std::unique_ptr<ASTConsumer>(new NullCheckConsumer);
}



