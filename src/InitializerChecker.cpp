#include "InitializerChecker.h"

using namespace clang;
using namespace std;

weak_ptr<IvarInfo> findIvarInfo(set<shared_ptr<IvarInfo>> &set, const ObjCIvarDecl *ivarDecl) {
    weak_ptr<IvarInfo> result;
    
    if (ivarDecl) {
        for (auto &info : set) {
            if (info->getIvarDecl() == ivarDecl) {
                result = weak_ptr<IvarInfo>(info);
                break;
            }
        }
    }
    
    return result;
}

weak_ptr<IvarInfo> findIvarInfo(set<shared_ptr<IvarInfo>> &set, const ObjCMethodDecl *setterMethod) {
    weak_ptr<IvarInfo> result;
    
    if (setterMethod) {
        for (auto &info : set) {
            auto propDecl = info->getPropertyDecl();
            if (propDecl && propDecl->getSetterMethodDecl() == setterMethod) {
                result = weak_ptr<IvarInfo>(info);
                break;
            }
        }
    }
    
    return result;
}

bool isInitializerMethod(clang::ObjCMethodDecl *methodDecl) {
    for (auto attr : methodDecl->attrs()) {
        auto annot = llvm::dyn_cast<AnnotateAttr>(attr);
        if (annot) {
            std::string annotation = annot->getAnnotation();
            if (annotation == "nlh_initializer") {
                return true;
            }
        }
    }
    
    return false;
}

class InitializerCheckerImpl : public RecursiveASTVisitor<InitializerCheckerImpl> {
    clang::ASTContext &_ASTContext;
    set<shared_ptr<IvarInfo>> &_NonnullIvars;
public:
    InitializerCheckerImpl(clang::ASTContext &astContext, set<shared_ptr<IvarInfo>> &nonnullIvars)
    : _ASTContext(astContext), _NonnullIvars(nonnullIvars) {}
    
    bool VisitObjCMessageExpr(ObjCMessageExpr *messageExpr) {
        ObjCMethodDecl *decl = messageExpr->getMethodDecl();
        auto info = findIvarInfo(_NonnullIvars, decl);
        if (!info.expired()) {
            _NonnullIvars.erase(info.lock());
        }
        
        if (isInitializerMethod(decl)) {
            auto receiver = messageExpr->getInstanceReceiver();
            if (receiver) {
                auto varRef = llvm::dyn_cast<DeclRefExpr>(receiver->IgnoreParenImpCasts());
                
                if (varRef) {
                    if (varRef->getDecl()->getNameAsString() == "self") {
                        _NonnullIvars.clear();
                    }
                }
            }
        }
        
        return true;
    }
    
    bool VisitBinAssign(BinaryOperator *expr) {
        ObjCIvarRefExpr *ivarRef = llvm::dyn_cast<ObjCIvarRefExpr>(expr->getLHS()->IgnoreParenImpCasts());
        if (ivarRef) {
            ObjCIvarDecl *ivarDecl = ivarRef->getDecl();
            auto info = findIvarInfo(_NonnullIvars, ivarDecl);
            if (!info.expired()) {
                _NonnullIvars.erase(info.lock());
            }
        }
        return true;
    }
};

std::set<shared_ptr<IvarInfo>> InitializerChecker::getNonnullIvars() {
    std::set<shared_ptr<IvarInfo>> set;
    
    for (auto ivarDecl : _ImplementationDecl->ivars()) {
        const Type *type = ivarDecl->getType().getTypePtrOrNull();
        if (type) {
            if (type->getNullability(_ASTContext).getValueOr(NullabilityKind::Unspecified) == NullabilityKind::NonNull) {
                set.insert(std::shared_ptr<IvarInfo>{ new IvarInfo(ivarDecl) });
            }
        }
    }
    
    auto interfaceDecl = _ImplementationDecl->getClassInterface();
    
    for (auto propDecl : interfaceDecl->properties()) {
        weak_ptr<IvarInfo> ivarInfo = findIvarInfo(set, propDecl->getPropertyIvarDecl());
        if (!ivarInfo.expired()) {
            auto shared = ivarInfo.lock();
            shared->setPropertyDecl(propDecl);
        }
    }
    
    for (auto extensionDecl : interfaceDecl->visible_extensions()) {
        for (auto propDecl : extensionDecl->properties()) {
            weak_ptr<IvarInfo> ivarInfo = findIvarInfo(set, propDecl->getPropertyIvarDecl());
            if (!ivarInfo.expired()) {
                auto shared = ivarInfo.lock();
                shared->setPropertyDecl(propDecl);
            }
        }
    }
    
    return set;
}

set<shared_ptr<IvarInfo>> InitializerChecker::check(clang::ObjCMethodDecl *methodDecl) {
    if (!isInitializerMethod(methodDecl)) {
        return set<shared_ptr<IvarInfo>>();
    }
    
    auto nonnullIvars = _NonnullIvars;
    
    InitializerCheckerImpl impl(_ASTContext, nonnullIvars);
    impl.TraverseStmt(methodDecl->getBody());
    
    return nonnullIvars;
}

