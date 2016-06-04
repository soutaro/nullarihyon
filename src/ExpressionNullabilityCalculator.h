#ifndef ExpressionNullabilityCalculator_h
#define ExpressionNullabilityCalculator_h

#include <unordered_map>

#include <clang/AST/AST.h>

class ExpressionNullability {
    const clang::Type *_type;
    clang::NullabilityKind _nullability;
    
public:
    explicit ExpressionNullability(const clang::Type *type, clang::NullabilityKind nullability) : _type(type), _nullability(nullability) {}
    explicit ExpressionNullability() : _type(NULL), _nullability(clang::NullabilityKind::Unspecified) {}
    
    const clang::Type *getType() const {
        return _type;
    }
    
    clang::NullabilityKind getNullability() const {
        return _nullability;
    }
    
    bool isNonNull() {
        return _nullability == clang::NullabilityKind::NonNull;
    }
};

typedef std::unordered_map<const clang::VarDecl *, ExpressionNullability> VariableNullabilityMapping;

class VariableNullabilityEnvironment {
    clang::ASTContext &_ASTContext;
    std::shared_ptr<VariableNullabilityMapping> _mapping;
    
public:
    explicit VariableNullabilityEnvironment(clang::ASTContext &astContext, std::shared_ptr<VariableNullabilityMapping> mapping)
    : _ASTContext(astContext), _mapping(mapping) {}
    
    void set(const clang::VarDecl *var, const clang::Type *type, clang::NullabilityKind kind);
    ExpressionNullability lookup(const clang::VarDecl *var) const;
    bool has(const clang::VarDecl *var) const;
    
    VariableNullabilityEnvironment *newCopy() {
        auto copy = std::shared_ptr<VariableNullabilityMapping>(new VariableNullabilityMapping);
        
        for (auto it : *_mapping) {
            (*copy)[it.first] = it.second;
        }
        
        return new VariableNullabilityEnvironment(_ASTContext, copy);
    }
};

class ExpressionNullabilityCalculator {
    clang::ASTContext &_ASTContext;
    std::shared_ptr<VariableNullabilityEnvironment> _VarEnv;
    
public:
    explicit ExpressionNullabilityCalculator(clang::ASTContext &astContext, std::shared_ptr<VariableNullabilityEnvironment> varEnv)
        : _ASTContext(astContext), _VarEnv(varEnv) {}
    virtual ~ExpressionNullabilityCalculator(){};
    
    virtual ExpressionNullability calculate(const clang::Expr *expr);
    
    virtual clang::ASTContext &getASTContext() {
        return _ASTContext;
    }
};

class VariableNullabilityPropagation {
    ExpressionNullabilityCalculator &_NullabilityCalculator;
    std::shared_ptr<VariableNullabilityEnvironment> _VarEnv;
    
public:
    explicit VariableNullabilityPropagation(ExpressionNullabilityCalculator &nullabilityCalculator,
                                            std::shared_ptr<VariableNullabilityEnvironment> varEnv)
    : _NullabilityCalculator(nullabilityCalculator), _VarEnv(varEnv) {}
    
    void propagate(clang::ObjCMethodDecl *methodDecl);
    void propagate(clang::BlockExpr *blockExpr);
};


#endif
