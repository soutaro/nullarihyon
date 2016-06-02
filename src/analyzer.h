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

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
    ASTContext &Context;
    QualType ReturnType;
    ExprNullabilityCalculator &NullabilityCalculator;
    NullabilityKindEnvironment &Env;
    
public:
    MethodBodyChecker(ASTContext &Context, QualType returnType, ExprNullabilityCalculator &Calculator, NullabilityKindEnvironment &env)
        : Context(Context), ReturnType(returnType), NullabilityCalculator(Calculator), Env(env) { }
    virtual ~MethodBodyChecker() {}

    virtual bool VisitDeclStmt(DeclStmt *decl);
    virtual bool VisitObjCMessageExpr(ObjCMessageExpr *callExpr);
    virtual bool VisitBinAssign(BinaryOperator *assign);
    virtual bool VisitReturnStmt(ReturnStmt *retStmt);
    virtual bool VisitObjCArrayLiteral(ObjCArrayLiteral *literal);
    virtual bool VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *literal);
    virtual bool TraverseBlockExpr(BlockExpr *blockExpr);
    virtual bool TraverseIfStmt(IfStmt *ifStmt);
    virtual bool VisitCStyleCastExpr(CStyleCastExpr *expr);

protected:
    DiagnosticBuilder WarningReport(SourceLocation location) {
        DiagnosticsEngine &diagEngine = Context.getDiagnostics();
        unsigned diagID = diagEngine.getCustomDiagID(DiagnosticsEngine::Warning, "%0") ;
        return diagEngine.Report(location, diagID);
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
