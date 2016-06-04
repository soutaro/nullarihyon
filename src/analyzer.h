#include <unordered_map>
#include <set>

#include <clang/Frontend/FrontendActions.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/StmtVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "ExpressionNullabilityCalculator.h"

using namespace clang;

class MethodUtility {
public:
    std::set<const clang::ObjCContainerDecl *> enumerateContainers(const clang::ObjCMessageExpr *expr);
};

class NullabilityCheckContext {
    const ObjCInterfaceDecl &InterfaceDecl;
    const ObjCMethodDecl &MethodDecl;
    const BlockExpr *BlockExpr;
    
public:
    NullabilityCheckContext(const ObjCInterfaceDecl &interfaceDecl, const ObjCMethodDecl &methodDecl, const clang::BlockExpr *blockExpr)
        : InterfaceDecl(interfaceDecl), MethodDecl(methodDecl), BlockExpr(blockExpr) {}
    
    NullabilityCheckContext(const ObjCInterfaceDecl &interfaceDecl, const ObjCMethodDecl &methodDecl)
        : InterfaceDecl(interfaceDecl), MethodDecl(methodDecl), BlockExpr(nullptr) {}
    
    const ObjCInterfaceDecl &getInterfaceDecl() const {
        return InterfaceDecl;
    }
    
    const ObjCMethodDecl &getMethodDecl() const {
        return MethodDecl;
    }
    
    const clang::BlockExpr *getBlockExpr() const {
        return BlockExpr;
    }
    
    QualType getReturnType() const;
    
    NullabilityCheckContext newContextForBlock(const clang::BlockExpr *blockExpr) {
        return NullabilityCheckContext(InterfaceDecl, MethodDecl, blockExpr);
    }
};

class MethodBodyChecker : public RecursiveASTVisitor<MethodBodyChecker> {
protected:
    ASTContext &_ASTContext;
    NullabilityCheckContext &_CheckContext;
    ExpressionNullabilityCalculator &_NullabilityCalculator;
    std::shared_ptr<VariableNullabilityEnvironment> _VarEnv;
    std::vector<std::string> &_Filter;
    
    DiagnosticBuilder WarningReport(SourceLocation location, std::set<std::string> &subjects);
    DiagnosticBuilder WarningReport(SourceLocation location, std::set<const clang::ObjCContainerDecl *> &subjects);
    
public:
    explicit MethodBodyChecker(ASTContext &astContext,
                               NullabilityCheckContext &checkContext,
                               ExpressionNullabilityCalculator &nullabilityCalculator,
                               std::shared_ptr<VariableNullabilityEnvironment> &env,
                               std::vector<std::string> &filter)
    : _ASTContext(astContext), _CheckContext(checkContext), _NullabilityCalculator(nullabilityCalculator), _VarEnv(env), _Filter(filter) {}
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
    
    virtual ObjCContainerDecl *InterfaceForSelector(const Expr *receiver, const Selector selector);
    virtual std::string MethodNameAsString(const ObjCMessageExpr &messageExpr);
    virtual std::string MethodCallSubjectAsString(const ObjCMessageExpr &messageExpr);
    
    virtual std::set<const ObjCContainerDecl *> subjectDecls(const Expr *expr);
};

class LAndExprChecker : public MethodBodyChecker {
public:
    explicit LAndExprChecker(ASTContext &astContext,
                             NullabilityCheckContext &checkContext,
                             ExpressionNullabilityCalculator &nullabilityCalculator,
                             std::shared_ptr<VariableNullabilityEnvironment> &env,
                             std::vector<std::string> &filter)
    : MethodBodyChecker(astContext, checkContext, nullabilityCalculator, env, filter) {}

    virtual bool TraverseBinLAnd(BinaryOperator *land);
    virtual bool TraverseBinLOr(BinaryOperator *lor);
    virtual bool TraverseUnaryLNot(UnaryOperator *S);
};

class NullCheckAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, clang::StringRef InFile);
    
    explicit NullCheckAction() : clang::ASTFrontendAction(), Debug(false), Filter(std::vector<std::string>()) {}
    
    void setDebug(bool debug) {
        Debug = debug;
    }
    
    void setFilter(std::vector<std::string> &filter) {
        Filter = filter;
    }
    
private:
    bool Debug;
    std::vector<std::string> Filter;
};
