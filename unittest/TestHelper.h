#ifndef TestHelper_hpp
#define TestHelper_hpp

#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include "TestHelper.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

class FunctionMatchCallback : public MatchFinder::MatchCallback {
    std::function<void(const MatchFinder::MatchResult &)> _Function;
    unsigned _count;
public:
    explicit FunctionMatchCallback(std::function<void(const MatchFinder::MatchResult &)> &function) : _Function(function), _count(0) {}
    
    virtual void run(const MatchFinder::MatchResult &result) {
        _Function(result);
        _count += 1;
    }
    
    unsigned getCount() {
        return _count;
    }
};

class ASTBuilder {
    std::unique_ptr<ASTUnit> _ASTUnit;
    std::vector<std::string> _args;
    
public:
    explicit ASTBuilder(const Twine &code) {
        _args = std::vector<std::string>{"-x", "objective-c", "-fobjc-arc", "-fmodules"};
        _ASTUnit = buildASTFromCodeWithArgs("__attribute__((objc_root_class))\n"
                                            "@interface NSObject\n"
                                            "  + (instancetype)alloc;\n"
                                            "  - (instancetype)init;\n"
                                            "  - (Class)class;\n"
                                            "@end\n"
                                            "@interface NSString : NSObject\n"
                                            "@end\n"
                                            "@interface NSNumber : NSObject\n"
                                            "  + (instancetype)numberWithInt:(int)x;\n"
                                            "@end\n"
                                            "@interface NSArray<ObjectType> : NSObject\n"
                                            "- (nonnull ObjectType)objectAtIndexedSubscript:(unsigned)idx;\n"
                                            "@end\n"
                                            "@interface NSDictionary<KeyType, ValueType> : NSObject\n"
                                            "- (nullable ValueType)objectForKeyedSubscript:(nonnull KeyType)key;\n"
                                            "@end\n"
                                            + code,
                                            _args);
    }
    
    const TranslationUnitDecl *getTranslationUnit() {
        return _ASTUnit->getASTContext().getTranslationUnitDecl();
    }
    
    ASTContext &getASTContext() {
        return _ASTUnit->getASTContext();
    }
    
    void matchWithCallback(const DeclarationMatcher &matcher, std::function<void(const MatchFinder::MatchResult &)> function);
    void matchWithCallback(const StatementMatcher &matcher, std::function<void(const MatchFinder::MatchResult &)> function);
    
    const VarDecl *getVarDecl(std::string name) {
        const VarDecl *decl = nullptr;
        
        matchWithCallback(varDecl(hasName(name)).bind("decl"),
                          [&](const MatchFinder::MatchResult &result) {
                              decl = result.Nodes.getStmtAs<VarDecl>("decl");
                          });
        
        return decl;
    }
    
    
    ObjCMethodDecl *getMethodDecl(std::string name = "test_method");
    
    /**
     Return expr where:
     id #{name} = #{expr};
     */
    const Expr *getTestExpr(std::string name = "testee", bool clean = false);
};

#endif /* TestHelper_hpp */
