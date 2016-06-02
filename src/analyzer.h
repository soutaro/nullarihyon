#include <unordered_map>

#include <clang/Frontend/FrontendActions.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>

using namespace clang;

typedef std::unordered_map<VarDecl *, NullabilityKind> NullabilityKindEnvironment;

class ExprNullabilityCalculator : public clang::StmtVisitor<ExprNullabilityCalculator, NullabilityKind> {
    ASTContext &Context;
    const NullabilityKindEnvironment &Env;
    bool Debug;
    
    NullabilityKind getNullability(const QualType &qualType);
    NullabilityKind UnexpectedUnspecified(Expr *expr);

public:
    ExprNullabilityCalculator(ASTContext &context, const NullabilityKindEnvironment &env, bool debug)
        : Context(context), Env(env), Debug(debug) { }
    
    const NullabilityKindEnvironment &getEnvironment() {
        return Env;
    }
    
    bool isDebug() {
        return Debug;
    }
    
    NullabilityKind VisitExpr(Expr *expr);
    NullabilityKind VisitDeclRefExpr(DeclRefExpr *refExpr);
    NullabilityKind VisitObjCPropertyRefExpr(ObjCPropertyRefExpr *expr);
    NullabilityKind VisitPseudoObjectExpr(PseudoObjectExpr *expr);
    NullabilityKind VisitObjCMessageExpr(ObjCMessageExpr *expr);
    NullabilityKind VisitObjCSubscriptRefExpr(ObjCSubscriptRefExpr *expr);
    NullabilityKind VisitBinAssign(BinaryOperator *expr);
    NullabilityKind VisitConditionalOperator(ConditionalOperator *expr);
    NullabilityKind VisitVarDecl(VarDecl *varDecl);

    NullabilityKind VisitObjCIvarRefExpr(ObjCIvarRefExpr *expr) { return getNullability(expr->getType()); }
    NullabilityKind VisitOpaqueValueExpr(OpaqueValueExpr *expr) { return Visit(expr->getSourceExpr()->IgnoreParenImpCasts()); }
    NullabilityKind VisitExprWithCleanups(ExprWithCleanups *expr) { return Visit(expr->getSubExpr()->IgnoreParenImpCasts()); }
    NullabilityKind VisitPredefinedExpr(PredefinedExpr *expr) { return NullabilityKind::NonNull; }
    NullabilityKind VisitCallExpr(CallExpr *expr) { return getNullability(expr->getType()); }
    NullabilityKind VisitMemberExpr(MemberExpr *expr) { return Visit(expr->getBase()->IgnoreParenImpCasts()); }
    NullabilityKind VisitBinaryOperator(BinaryOperator *expr) { return NullabilityKind::NonNull; }
    NullabilityKind VisitBinaryConditionalOperator(BinaryConditionalOperator *expr) { return Visit(expr->getFalseExpr()->IgnoreParenImpCasts()); }
    NullabilityKind VisitBlockExpr(BlockExpr *expr) { return NullabilityKind::NonNull; }
    NullabilityKind VisitStmtExpr(StmtExpr *expr) { return getNullability(expr->getType()); }
    NullabilityKind VisitCastExpr(CastExpr *expr) { return getNullability(expr->getType()); }
};

class NullCheckAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, clang::StringRef InFile);
    
    void setDebug(bool debug) {
        Debug = debug;
    }
    
private:
    bool Debug;
};
