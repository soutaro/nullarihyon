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

TEST(VariableNullabilityPropagation, nullability_propagation_from_init) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  NSObject * _Nonnull a;\n"
                       "  NSObject * _Nullable b;\n"
                       "  NSObject *x = a;\n"
                       "  NSObject *y = b;\n"
                       "  NSObject *z = x;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    VariableNullabilityPropagation prop(calculator, env);
    
    ObjCMethodDecl *method = builder.getMethodDecl();
    
    prop.propagate(method);
    
    ASSERT_TRUE(env->lookup(builder.getVarDecl("a")).isNonNull());
    ASSERT_FALSE(env->lookup(builder.getVarDecl("b")).isNonNull());
    ASSERT_TRUE(env->lookup(builder.getVarDecl("x")).isNonNull());
    ASSERT_FALSE(env->lookup(builder.getVarDecl("y")).isNonNull());
    ASSERT_TRUE(env->lookup(builder.getVarDecl("z")).isNonNull());
}

TEST(VariableNullabilityPropagation, nullability_propagation_in_block) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  id a = ^(NSString *x){\n"
                       "    NSString *y;"
                       "  };\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    VariableNullabilityPropagation prop(calculator, env);
    
    ObjCMethodDecl *method = builder.getMethodDecl();
    
    prop.propagate(method);
    
    ASSERT_TRUE(env->has(builder.getVarDecl("a")));
    ASSERT_FALSE(env->has(builder.getVarDecl("x")));
    ASSERT_FALSE(env->has(builder.getVarDecl("y")));
}
