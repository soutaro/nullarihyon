#ifndef InitializerChecker_h
#define InitializerChecker_h

#include <unordered_map>
#include <set>

#include <clang/AST/AST.h>

class IvarInfo {
    const clang::ObjCIvarDecl *_IvarDecl;
    const clang::ObjCPropertyDecl *_PropertyDecl;
public:
    IvarInfo(const clang::ObjCIvarDecl *ivarDecl) : _IvarDecl(ivarDecl), _PropertyDecl(nullptr) {}
    
    const clang::ObjCIvarDecl *getIvarDecl() {
        return _IvarDecl;
    }
    
    const clang::ObjCPropertyDecl *getPropertyDecl() {
        return _PropertyDecl;
    }
    
    void setPropertyDecl(const clang::ObjCPropertyDecl *propDecl) {
        _PropertyDecl = propDecl;
    }
};

class InitializerChecker {
    clang::ASTContext &_ASTContext;
    const clang::ObjCImplementationDecl *_ImplementationDecl;
    std::set<std::shared_ptr<IvarInfo>> _NonnullIvars;
    
public:
    InitializerChecker(clang::ASTContext &astContext, const clang::ObjCImplementationDecl *implementationDecl)
    : _ASTContext(astContext), _ImplementationDecl(implementationDecl) {
        _NonnullIvars = getNonnullIvars();
    }
    
    std::set<std::shared_ptr<IvarInfo>> check(clang::ObjCMethodDecl *methodDecl);
    bool isInitializer(clang::ObjCMethodDecl *methodDecl);
    std::set<std::shared_ptr<IvarInfo>> getNonnullIvars();
};

#endif /* InitializerChecker_hpp */
