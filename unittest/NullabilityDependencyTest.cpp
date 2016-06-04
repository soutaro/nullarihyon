#include <gtest/gtest.h>
#include <iostream>

#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <ExpressionNullabilityCalculator.h>
#include <NullabilityDependencyCalculator.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

TEST(NullabilityDependencyCalculatorExpander, expand_conditional_operator) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  NSString * _Nullable x;\n"
                       "  NSString * _Nonnull y;\n"
                       "  id testee = x ? x : x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyExpandVisitor expander(builder.getASTContext(), builder.getMethodDecl());
    
    const ConditionalOperator *expr = llvm::dyn_cast<ConditionalOperator>(builder.getTestExpr()->IgnoreParenImpCasts());
    
    // The expr is nullable
    // both true and false
    auto deps = expander.Visit(expr);
    
    std::set<const Expr *> expected;
    expected.insert(expr->getTrueExpr());
    expected.insert(expr->getFalseExpr());
    
    ASSERT_EQ(deps, expected);
}

TEST(NullabilityDependencyCalculatorExpander, call_nonnull_method) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Foo * _Nonnull x;\n"
                       "  id testee = [x bar];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyExpandVisitor expander(builder.getASTContext(), builder.getMethodDecl());
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto deps = expander.Visit(expr);
    
    // Because method has nonnull return type, receiver is the only source for nil
    // Does not check receiver's nullability
    ASSERT_EQ(deps, std::set<const Expr *>{ expr->getInstanceReceiver() });
}

TEST(NullabilityDependencyCalculatorExpander, call_nullable_method) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Foo * _Nonnull x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyExpandVisitor expander(builder.getASTContext(), builder.getMethodDecl());
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto deps = expander.Visit(expr);
    
    // Because method has nullable return type, receiver and call expr may be source of il
    std::set<const Expr *> set;
    set.insert(expr);
    set.insert(expr->getInstanceReceiver());
    
    ASSERT_EQ(deps, set);
}

TEST(NullabilityDependencyCalculatorExpander, var_ref) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  NSString * _Nonnull x = @\"Hello\";\n"
                       "  id testee = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyExpandVisitor expander(builder.getASTContext(), builder.getMethodDecl());
    
    const Expr *expr = builder.getTestExpr("testee", true);
    
    auto deps = expander.Visit(expr);
    
    const VarDecl *decl = builder.getVarDecl("x");
    
    // Expand variable assignment one step
    std::set<const Expr *> set;
    set.insert(decl->getInit());

    ASSERT_EQ(deps, set);
}

TEST(NullabilityDependencyCalculatorExpander, var_ref_without_assignment) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  NSString * _Nonnull x;\n"
                       "  id testee = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyExpandVisitor expander(builder.getASTContext(), builder.getMethodDecl());
    
    const Expr *expr = builder.getTestExpr("testee", true);
    
    auto deps = expander.Visit(expr);
    
    // Expanded one step to empty
    std::set<const Expr *> set;
    
    ASSERT_EQ(deps, set);
}

TEST(NullabilityDependencyCalculatorExpander, expand_recursively) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  NSNumber *x = @0;\n"
                       "  NSNumber *y = @1;\n"
                       "  NSNumber *z = @2;\n"
                       "  x = y;\n"
                       "  y = z;\n"
                       "  id testee = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator nullabilityCalculator(builder.getASTContext(), env);
    NullabilityDependencyCalculator dependencyCalculator(builder.getASTContext());
    
    const Expr *expr = builder.getTestExpr("testee", true);
    
    auto deps = dependencyCalculator.calculate(builder.getMethodDecl(), expr);
    
    // deps is transitive closure
    ASSERT_NE(deps.find(builder.getVarDecl("x")->getInit()), deps.end());
    ASSERT_NE(deps.find(builder.getVarDecl("y")->getInit()), deps.end());
    ASSERT_NE(deps.find(builder.getVarDecl("z")->getInit()), deps.end());
}
