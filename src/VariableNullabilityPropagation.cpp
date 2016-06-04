#include "ExpressionNullabilityCalculator.h"

using namespace clang;

class VariableNullabilityPropagationVisitor : public RecursiveASTVisitor<VariableNullabilityPropagationVisitor> {
    ExpressionNullabilityCalculator &_NullabilityCalculator;
    std::shared_ptr<VariableNullabilityEnvironment> _VarEnv;

public:
    explicit VariableNullabilityPropagationVisitor(ExpressionNullabilityCalculator &nullabilityCalculator,
                                                   std::shared_ptr<VariableNullabilityEnvironment> varEnv)
    : RecursiveASTVisitor(), _NullabilityCalculator(nullabilityCalculator), _VarEnv(varEnv) {
        
    }
    
    bool VisitVarDecl(VarDecl *decl) {
        const Type *type = decl->getType().getTypePtr();
        auto nullability = type->getNullability(_NullabilityCalculator.getASTContext());
        
        if (type->isPointerType() || type->isObjCObjectPointerType() || type->isBlockPointerType()) {
            if (!nullability.hasValue()) {
                auto init = decl->getInit();
                if (init && !llvm::isa<ImplicitValueInitExpr>(init)) {
                    ExpressionNullability n = _NullabilityCalculator.calculate(decl->getInit());
                    auto kind = n.getNullability();
                    _VarEnv->set(decl, type, kind);
                }
            }
        }
        
        return true;
    }
    
    bool TraverseBlockExpr(BlockExpr *block) {
        // Do nothing
        return true;
    }

    bool TraverseObjCForCollectionStmt(ObjCForCollectionStmt *stmt) {
        auto decl = llvm::dyn_cast<DeclStmt>(stmt->getElement());
        if (decl) {
            for (auto it : decl->getDeclGroup()) {
                auto varDecl = llvm::dyn_cast<VarDecl>(it);
                if (varDecl) {
                    auto type = varDecl->getType().getTypePtr();
                    
                    if (type->isPointerType() || type->isObjCObjectPointerType() || type->isBlockPointerType()) {
                        Optional<NullabilityKind> kind = type->getNullability(_NullabilityCalculator.getASTContext());
                        if (!kind.hasValue()) {
                            _VarEnv->set(varDecl, type, NullabilityKind::NonNull);
                        }
                    }
                }
            }
        }
        
        return TraverseStmt(stmt->getBody());
    }
};

void VariableNullabilityPropagation::propagate(clang::ObjCMethodDecl *methodDecl) {
    VariableNullabilityPropagationVisitor visitor(_NullabilityCalculator, _VarEnv);
    visitor.TraverseStmt(methodDecl->getBody());
}

void VariableNullabilityPropagation::propagate(clang::BlockExpr *blockExpr) {
    VariableNullabilityPropagationVisitor visitor(_NullabilityCalculator, _VarEnv);
    visitor.TraverseStmt(blockExpr->getBody());
}
