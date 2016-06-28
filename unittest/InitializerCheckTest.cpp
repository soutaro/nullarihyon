#include <gtest/gtest.h>
#include <iostream>
#include <set>

#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <ExpressionNullabilityCalculator.h>
#include <InitializerChecker.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

TEST(InitializerCheck, find_nonnull_ivars) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@property (nonatomic, nonnull) NSString *hello;\n"
                       "@property (nonatomic, nullable) NSNumber *good;\n"
                       "@end\n"
                       "@interface Test()\n"
                       "@property (nonatomic, nonnull) NSString *extension;\n"
                       "@property (nonatomic, nullable) NSNumber *extension2;\n"
                       "@end\n"
                       "@interface Test (Cat)\n"
                       "@property (nonatomic, nonnull) NSString *category;\n"
                       "@property (nonatomic, nullable) NSNumber *category2;\n"
                       "@end\n"
                       "@implementation Test {\n"
                       "  NSString * _Nonnull _impl1;\n"
                       "  NSString * _Nullable _impl2;\n"
                       "}\n"
                       "- (nonnull instancetype)init1 __attribute__((annotate(\"hoge\"))) {\n"
                       "  return self;\n"
                       "}\n"
                       "@end\n");
    
    std::shared_ptr<VariableNullabilityMapping> map(new VariableNullabilityMapping);
    std::shared_ptr<VariableNullabilityEnvironment> env(new VariableNullabilityEnvironment(builder.getASTContext(), map));
    ExpressionNullabilityCalculator calculator(builder.getASTContext(), env);
    VariableNullabilityPropagation prop(calculator, env);
    
    ObjCMethodDecl *method = builder.getMethodDecl("init1");
    
    for (auto attr : method->attrs()) {
        auto kind = attr->getKind();
        if (kind == clang::attr::Annotate) {
            auto annot = llvm::dyn_cast<AnnotateAttr>(attr);
            std::string name = annot->getAnnotation();
            ASSERT_EQ(name, "hoge");
        }
    }
    
    ObjCImplementationDecl *impl = builder.getImplementationDecl("Test");
    
    InitializerChecker checker(builder.getASTContext(), impl);
    
    auto ivars = checker.getNonnullIvars();
    
    std::set<std::string> actualIvarNames;
    for (auto ivar : ivars) {
        actualIvarNames.insert(ivar->getIvarDecl()->getNameAsString());
    }
    
    std::set<std::string> expectedIvarNames;
    expectedIvarNames.insert("_hello");
    expectedIvarNames.insert("_extension");
    expectedIvarNames.insert("_impl1");
    
    ASSERT_EQ(expectedIvarNames, actualIvarNames);
}