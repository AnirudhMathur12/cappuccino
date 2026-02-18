#ifndef CAPPUCCINO_VISITOR_H
#define CAPPUCCINO_VISITOR_H

struct LiteralExpr;
struct IdentifierExpr;
struct UnaryExpr;
struct BinaryExpr;
struct GroupingExpr;
struct FunctionCallExpr;

struct ExprStmt;
struct VariableDeclStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ReturnStmt;
struct FunctionParameterStmt;
struct FunctionDeclStmt;

class Visitor {
  public:
    virtual ~Visitor() = default;

    // Expressions
    virtual void visitLiteralExpr(const LiteralExpr *expr) = 0;
    virtual void visitIdentifierExpr(const IdentifierExpr *expr) = 0;
    virtual void visitUnaryExpr(const UnaryExpr *expr) = 0;
    virtual void visitBinaryExpr(const BinaryExpr *expr) = 0;
    virtual void visitGroupingExpr(const GroupingExpr *expr) = 0;
    virtual void visitFunctionCallExpr(const FunctionCallExpr *expr) = 0;

    // Statements
    virtual void visitExprStmt(const ExprStmt *stmt) = 0;
    virtual void visitVariableDeclStmt(const VariableDeclStmt *stmt) = 0;
    virtual void visitBlockStmt(const BlockStmt *stmt) = 0;
    virtual void visitIfStmt(const IfStmt *stmt) = 0;
    virtual void visitWhileStmt(const WhileStmt *stmt) = 0;
    virtual void visitForStmt(const ForStmt *stmt) = 0;
    virtual void visitReturnStmt(const ReturnStmt *stmt) = 0;
    virtual void visitFunctionParameterStmt(const FunctionParameterStmt *stmt) = 0;
    virtual void visitFunctionDeclStmt(const FunctionDeclStmt *stmt) = 0;
};

#endif // CAPPUCCINO_VISITOR_H
