#include <stdio.h>
#include <iostream>
#include <stack>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "analyzer.h"
#include "ErrorMessage.h"

using namespace llvm;
using namespace clang;

class ExprNullabilityCalculator : public StmtVisitor<ExprNullabilityCalculator, NullabilityKind> {
public:
  ExprNullabilityCalculator(ASTContext &context) : Context(context) {

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
        return getNullability(decl->getType());
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

    if (receiverKind == ObjCMessageExpr::ReceiverKind::Instance) {
      Expr *receiver = expr->getInstanceReceiver();
      NullabilityKind receiverNullability = Visit(receiver->IgnoreParenImpCasts());
      if (receiverNullability != NullabilityKind::NonNull) {
        return NullabilityKind::Nullable;
      }
    }

    ObjCMethodDecl *decl = expr->getMethodDecl();
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
    return getNullability(expr->getType());
  }

  NullabilityKind VisitStmtExpr(StmtExpr *expr) {
    return getNullability(expr->getType());
  }

  NullabilityKind VisitCastExpr(CastExpr *expr) {
    return getNullability(expr->getType());
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
};

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
public:
  MethodBodyChecker(ASTContext &Context, std::vector<ErrorMessage> &errors, QualType returnType)
    : Context(Context), Errors(errors), ReturnType(returnType) {
  }

  bool VisitDeclStmt(DeclStmt *decl) {
    DeclGroupRef group = decl->getDeclGroup();
    DeclGroupRef::iterator it;
    for(it = group.begin(); it != group.end(); it++) {
      VarDecl *vd = llvm::dyn_cast<VarDecl>(*it);
      if (vd) {
        QualType varType = vd->getType();

        Expr *init = vd->getInit();
        if (init) {
          QualType valType = this->getType(init);
          if (!this->isNonNullExpr(init)) {
            if (!this->testTypeNullability(varType, valType)) {
              std::string loc = vd->getLocation().printToString(Context.getSourceManager());
              std::ostringstream s;
              s << "var decl (" << vd->getNameAsString() << ")";
              Errors.push_back(ErrorMessage(loc, s));
            }
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
        QualType argType = this->getType(arg);

        if (!this->isNonNullExpr(arg)) {
          if (!this->testTypeNullability(paramQType, argType)) {
            std::string loc = arg->getExprLoc().printToString(Context.getSourceManager());
            std::ostringstream s;
            s << "method call (" << decl->getNameAsString() << ", " << index << ")";
            Errors.push_back(ErrorMessage(loc, s));
          }
        }

        index++;
      }
    }

    return true;
  }

  bool VisitBinAssign(BinaryOperator *assign) {
    Expr *lhs = assign->getLHS();
    Expr *rhs = assign->getRHS();

    if (!this->isNonNullExpr(rhs)) {
      if (!this->testTypeNullability(this->getType(lhs), this->getType(rhs))) {
        std::string loc = rhs->getExprLoc().printToString(Context.getSourceManager());
        std::ostringstream s;
        s << "assignment";
        Errors.push_back(ErrorMessage(loc, s));
      }
    }

    return true;
  }

  bool VisitReturnStmt(ReturnStmt *retStmt) {
    Expr *value = retStmt->getRetValue();
    if (value) {
      if (!this->isNonNullExpr(value)) {
        QualType type = this->getType(value);
        if (!this->testTypeNullability(ReturnType, type)) {
          std::string loc = value->getExprLoc().printToString(Context.getSourceManager());
          std::ostringstream s;
          s << "return";
          Errors.push_back(ErrorMessage(loc, s));
        }
      }
    }

    return true;
  }

  bool VisitExpr(Expr *expr) {
    ExprNullabilityCalculator calculator = ExprNullabilityCalculator(Context);
    NullabilityKind kind = calculator.Visit(expr->IgnoreParenImpCasts());

    std::string s = "unknown";
    switch (kind) {
      case NullabilityKind::Unspecified:
        s = "Unspecified";
        break;
      case NullabilityKind::NonNull:
        s = "NonNull";
        break;
      case NullabilityKind::Nullable:
        s = "Nullable";
        break;
    }

    std::string loc = expr->getExprLoc().printToString(Context.getSourceManager());
    // std::cerr << "VisitExpr nullability = " << s << " (" << loc << ")" << std::endl;
    return true;
  }

  bool TraverseBlockExpr(BlockExpr *blockExpr) {
    const Type *type = blockExpr->getType().getTypePtr();
    const BlockPointerType *blockType = llvm::dyn_cast<BlockPointerType>(type);
    if (blockType) {
      const FunctionProtoType *funcType = llvm::dyn_cast<FunctionProtoType>(blockType->getPointeeType().getTypePtr());
      if (funcType) {
        QualType retType = funcType->getReturnType();
        MethodBodyChecker checker(Context, Errors, retType);
        checker.TraverseStmt(blockExpr->getBody());
      }
    }

    return true;
  }

  QualType getType(const Expr *expr) {
    const Expr *e = expr->IgnoreParenImpCasts();

    const PseudoObjectExpr *poe = llvm::dyn_cast<PseudoObjectExpr>(e);
    if (poe) {
      return this->getType(poe->getSyntacticForm());
    }

    const ObjCPropertyRefExpr *propRef = llvm::dyn_cast<ObjCPropertyRefExpr>(e);
    if (propRef) {
      if (propRef->isImplicitProperty()) {
        const ObjCMethodDecl *getter = propRef->getImplicitPropertyGetter();
        return getter->getReturnType();
      }
      if (propRef->isExplicitProperty()) {
        ObjCPropertyDecl *decl = propRef->getExplicitProperty();
        return decl->getType();
      }
    }

    return e->getType();
  }

  bool testTypeNullability(const QualType &expectedType, const QualType &actualType) {
    Optional<NullabilityKind> expectedKind = this->nullability(expectedType);
    Optional<NullabilityKind> actualKind = this->nullability(actualType);
    if (expectedKind.getValueOr(NullabilityKind::Unspecified) == NullabilityKind::NonNull) {
      if (actualKind.getValueOr(NullabilityKind::Unspecified) != NullabilityKind::NonNull) {
        return false;
      }
    }

    return true;
  }

  bool isNonNullExpr(Expr *expr) {
    Expr *e = expr->IgnoreParenCasts();
    if (llvm::dyn_cast<ObjCStringLiteral>(e)) return true;
    if (llvm::dyn_cast<ObjCBoxedExpr>(e)) return true;
    if (llvm::dyn_cast<ObjCArrayLiteral>(e)) return true;
    if (llvm::dyn_cast<ObjCDictionaryLiteral>(e)) return true;
    if (llvm::dyn_cast<ObjCSelectorExpr>(e)) return true;
    if (llvm::dyn_cast<DeclRefExpr>(e)) {
      DeclRefExpr *ref = llvm::dyn_cast<DeclRefExpr>(e);
      if (ref) {
        return ref->getNameInfo().getAsString() == "self";
      }
    }

    return false;
  }

  Optional<NullabilityKind> nullability(const QualType &qtype) {
    const Type *type = qtype.getTypePtr();
    if (type) {
      return type->getNullability(Context);
    } else {
      return Optional<NullabilityKind>::create(NULL);
    }
  }

private:
  ASTContext &Context;
  std::vector<ErrorMessage> &Errors;
  QualType ReturnType;
};

class NullCheckVisitor : public RecursiveASTVisitor<NullCheckVisitor> {
public:
  NullCheckVisitor(ASTContext &context, std::vector<ErrorMessage> &errors) : Context(context), Errors(errors) {}

  bool VisitDecl(Decl *decl) {
    ObjCMethodDecl *methodDecl = llvm::dyn_cast<ObjCMethodDecl>(decl);
    if (methodDecl) {
      if (methodDecl->hasBody()) {
        QualType returnType = methodDecl->getReturnType();

        MethodBodyChecker checker(Context, Errors, returnType);
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



