//
// Created by Anirudh Mathur on 13/11/25.
//

#ifndef CAPPUCCINO_DEBUGVISITOR_H
#define CAPPUCCINO_DEBUGVISITOR_H

#include "Visitor.h"
#include <string>

class DebugVisitor : public Visitor {
  public:
    // Expressions
    void visitLiteralExpr(const LiteralExpr *expr) override;
    void visitIdentifierExpr(const IdentifierExpr *expr) override;
    void visitUnaryExpr(const UnaryExpr *expr) override;
    void visitBinaryExpr(const BinaryExpr *expr) override;
    void visitGroupingExpr(const GroupingExpr *expr) override;
    void visitFunctionCallExpr(const FunctionCallExpr *expr) override;

    // Statements
    void visitExprStmt(const ExprStmt *stmt) override;
    void visitVariableDeclStmt(const VariableDeclStmt *stmt) override;
    void visitBlockStmt(const BlockStmt *stmt) override;
    void visitIfStmt(const IfStmt *stmt) override;
    void visitWhileStmt(const WhileStmt *stmt) override;
    void visitForStmt(const ForStmt *stmt) override;
    void visitReturnStmt(const ReturnStmt *stmt) override;
    void visitFunctionParameterStmt(const FunctionParameterStmt *stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt *stmt) override;

  private:
    int indent_level = 0;
    std::string pad() const;
};

#endif // CAPPUCCINO_DEBUGVISITOR_H
