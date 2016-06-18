#include <gtest/gtest.h>
#include <iostream>

#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <ExpressionNullabilityCalculator.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

TEST(ExpressionNullabilityCalculator, base_case) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  unsigned testee = 123;\n"
                       "}\n"
                       "@end\n");

    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto init = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(init).isNonNull());
}

TEST(ExpressionNullabilityCalculator, string_literal_is_nonnull) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = @\"hello\";\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto init = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(init).isNonNull());
}

TEST(ExpressionNullabilityCalculator, self_is_nonnull) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = self;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto init = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(init).isNonNull());
}

TEST(ExpressionNullabilityCalculator, var_ref_nullability_from_expression) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject * _Nullable x;\n"
                       "  id testee = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto init = builder.getTestExpr();
    ASSERT_FALSE(calculator.calculate(init).isNonNull());
};

TEST(ExpressionNullabilityCalculator, var_ref_nullability_from_env) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject * _Nullable x;\n"
                       "  id testee = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto x = builder.getVarDecl("x");
    env->set(x, x->getType().getTypePtr(), NullabilityKind::NonNull);

    auto init = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(init).isNonNull());
};

TEST(ExpressionNullabilityCalculator, message_expr_is_nullable_if_method_returns_nullable) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (nullable id)foo { return @\"\"; }\n"
                       "- (void)hello {\n"
                       "  Test * _Nonnull x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto init = builder.getTestExpr();
    ASSERT_FALSE(calculator.calculate(init).isNonNull());
};

TEST(ExpressionNullabilityCalculator, message_expr_is_nonnull_if_method_returns_nonnull) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (nonnull id)foo { return @\"\"; }\n"
                       "- (void)hello {\n"
                       "  Test * _Nonnull x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, message_expr_is_nullable_if_receiver_is_nullable) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (nonnull id)foo { return @\"\"; }\n"
                       "- (void)hello {\n"
                       "  Test * _Nullable x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_FALSE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, instance_method_class_is_nonnull) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  Test * _Nonnull x;\n"
                       "  id testee = [x class];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, instance_method_class_is_nullable_if_declaration_has_nullability) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "- (nullable Class)class;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (nullable Class)class { Class x; return x; }\n"
                       "- (void)hello {\n"
                       "  Test * _Nonnull x;\n"
                       "  id testee = [x class];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_FALSE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, class_method_nullability) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Test2 : NSObject\n"
                       "+ (nonnull id)foo;\n"
                       "@end"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = [Test2 foo];\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, super_method_nullability__super_is_assumed_nonnull) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface NSObject (cat)\n"
                       "- (nonnull id)foo;"
                       "@end\n"
                       "@implementation Test\n"
                       "- (nonnull id)foo {\n"
                       "  id testee = [super foo];\n"
                       "  return testee;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, property_read) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@property (nonatomic, nonnull) id prop;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = self.prop;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    auto expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, binary_conditional) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject * _Nullable x;\n"
                       "  NSObject * _Nonnull y;\n"
                       "  id testee = x ?: y;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const VarDecl *y = builder.getVarDecl("y");
    
    env->set(y, y->getType().getTypePtr(), NullabilityKind::NonNull);
    
    const Expr *expr = builder.getTestExpr();
    
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, binary_conditional2) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject * _Nullable x;\n"
                       "  NSObject * _Nonnull y;\n"
                       "  id testee = x ?: y;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const VarDecl *y = builder.getVarDecl("y");
    
    env->set(y, y->getType().getTypePtr(), NullabilityKind::Nullable);
    
    const Expr *expr = builder.getTestExpr();
    
    ASSERT_FALSE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, conditional) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject *x;\n"
                       "  NSObject *y;\n"
                       "  NSObject *z;"
                       "  id testee = x ? y : z;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const VarDecl *y = builder.getVarDecl("y");
    const VarDecl *z = builder.getVarDecl("z");
    
    env->set(y, y->getType().getTypePtr(), NullabilityKind::NonNull);
    env->set(z, z->getType().getTypePtr(), NullabilityKind::NonNull);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, conditional2) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject *x;\n"
                       "  NSObject *y;\n"
                       "  NSObject *z;\n"
                       "  id testee = x ? y : z;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const VarDecl *y = builder.getVarDecl("y");
    const VarDecl *z = builder.getVarDecl("z");
    
    env->set(y, y->getType().getTypePtr(), NullabilityKind::Nullable);
    env->set(z, z->getType().getTypePtr(), NullabilityKind::NonNull);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_FALSE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, cast) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSObject *x;\n"
                       "  id testee = (NSObject * _Nonnull)x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, number_literal) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = @13;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
};

TEST(ExpressionNullabilityCalculator, block_expr) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = ^(){};"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
}

TEST(ExpressionNullabilityCalculator, bin_assign) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id x;\n"
                       "  id y;\n"
                       "  id testee = x=y;"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const VarDecl *y = builder.getVarDecl("y");
    env->set(y, y->getType().getTypePtr(), NullabilityKind::NonNull);

    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
}

TEST(ExpressionNullabilityCalculator, subscript_ref) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  NSArray * _Nonnull x;\n"
                       "  id testee = x[0];"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
}

TEST(ExpressionNullabilityCalculator, selector) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = @selector(hello);"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
}

TEST(ExpressionNullabilityCalculator, new) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "+ (instancetype)new;"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)hello {\n"
                       "  id testee = [Test new];"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    
    const Expr *expr = builder.getTestExpr();
    ASSERT_TRUE(calculator.calculate(expr).isNonNull());
}
