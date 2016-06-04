#include <gtest/gtest.h>
#include <iostream>

#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <analyzer.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

TEST(MethodUtility, find_containers) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (nullable NSString *)foo;\n"
                       "  - (nonnull NSString *)bar;\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Foo *x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("Foo");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, find_containers_via_protocol) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@protocol FooProtocol\n"
                       "  - (id)foo;"
                       "@end\n"
                       "@interface Foo : NSObject<FooProtocol>\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Foo *x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("FooProtocol");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, find_containers_inherited) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (id)foo;"
                       "@end\n"
                       "@interface Bar : Foo\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Bar *x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("Foo");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, find_containers_inherited_implementation) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@interface Foo : NSObject\n"
                       "  - (id)foo;"
                       "@end\n"
                       "@interface Bar : Foo\n"
                       "  - (id)foo;"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Bar *x;\n"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("Bar");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, find_containers_class_method) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  id testee = [Test alloc];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("NSObject");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, id_with_protocol) {
    ASTBuilder builder("@protocol P\n"
                       "  - (nullable NSString *)foo;\n"
                       "@end\n"
                       "@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  id<P> x;"
                       "  id testee = [x foo];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    expected.insert("P");
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, id_without_protocol) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  id x;"
                       "  id testee = [x init];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    
    ASSERT_EQ(names, expected);
}

TEST(MethodUtility, Class) {
    ASTBuilder builder("@interface Test : NSObject\n"
                       "@end\n"
                       "@implementation Test\n"
                       "- (void)test_method {\n"
                       "  Class c;"
                       "  id testee = [c alloc];\n"
                       "}\n"
                       "@end\n");
    
    MethodUtility utility;
    
    const ObjCMessageExpr *expr = llvm::dyn_cast<ObjCMessageExpr>(builder.getTestExpr("testee", true));
    
    auto containers = utility.enumerateContainers(expr);
    
    std::set<std::string> names;
    
    for (auto container : containers) {
        names.insert(container->getNameAsString());
    }
    
    std::set<std::string> expected;
    
    ASSERT_EQ(names, expected);
}
