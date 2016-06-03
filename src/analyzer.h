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

class NullabilityCheckContext {
    ObjCInterfaceDecl &InterfaceDecl;
    ObjCMethodDecl &MethodDecl;
    BlockExpr *BlockExpr;
    
public:
    NullabilityCheckContext(ObjCInterfaceDecl &interfaceDecl, ObjCMethodDecl &methodDecl, clang::BlockExpr *blockExpr)
        : InterfaceDecl(interfaceDecl), MethodDecl(methodDecl), BlockExpr(blockExpr) {}
    
    NullabilityCheckContext(ObjCInterfaceDecl &interfaceDecl, ObjCMethodDecl &methodDecl)
        : InterfaceDecl(interfaceDecl), MethodDecl(methodDecl), BlockExpr(nullptr) {}
    
    ObjCInterfaceDecl &getInterfaceDecl() {
        return InterfaceDecl;
    }
    
    ObjCMethodDecl &getMethodDecl() {
        return MethodDecl;
    }
    
    clang::BlockExpr *getBlockExpr() {
        return BlockExpr;
    }
    
    QualType getReturnType();
    
    NullabilityCheckContext newContextForBlock(clang::BlockExpr *blockExpr) {
        return NullabilityCheckContext(InterfaceDecl, MethodDecl, blockExpr);
    }
};

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
public:
    MethodBodyChecker(ASTContext &Context, NullabilityCheckContext &checkContext, ExprNullabilityCalculator &Calculator, NullabilityKindEnvironment &env)
        : Context(Context), CheckContext(checkContext), NullabilityCalculator(Calculator), Env(env) { }
    virtual ~MethodBodyChecker() {}

    virtual bool VisitDeclStmt(DeclStmt *decl);
    virtual bool VisitObjCMessageExpr(ObjCMessageExpr *callExpr);
    virtual bool VisitBinAssign(BinaryOperator *assign);
    virtual bool VisitReturnStmt(ReturnStmt *retStmt);
    virtual bool VisitObjCArrayLiteral(ObjCArrayLiteral *literal);
    virtual bool VisitObjCDictionaryLiteral(ObjCDictionaryLiteral *literal);
    virtual bool TraverseBlockExpr(BlockExpr *blockExpr);
    virtual bool TraverseIfStmt(IfStmt *ifStmt);
    virtual bool TraverseBinLAnd(BinaryOperator *land);
    virtual bool VisitCStyleCastExpr(CStyleCastExpr *expr);
    
    virtual bool TraverseBinLOr(BinaryOperator *lor) { return RecursiveASTVisitor::TraverseBinLOr(lor); };
    virtual bool TraverseUnaryLNot(UnaryOperator *lnot) { return RecursiveASTVisitor::TraverseUnaryLNot(lnot); };

protected:
    ASTContext &Context;
    NullabilityCheckContext &CheckContext;
    ExprNullabilityCalculator &NullabilityCalculator;
    NullabilityKindEnvironment &Env;

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

class LAndExprChecker : public MethodBodyChecker {
public:
    LAndExprChecker(ASTContext &Context, NullabilityCheckContext &checkContext, ExprNullabilityCalculator &Calculator, NullabilityKindEnvironment &env)
        : MethodBodyChecker(Context, checkContext, Calculator, env) {};

    virtual bool TraverseBinLAnd(BinaryOperator *land);
    virtual bool TraverseBinLOr(BinaryOperator *lor);
    virtual bool TraverseUnaryLNot(UnaryOperator *S);
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
