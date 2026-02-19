#ifndef CAPPUCCINO_CODEGEN_H_
#define CAPPUCCINO_CODEGEN_H_

#include "AbstractSyntaxTree.h"
#include "Type.h"
#include "Visitor.h"
#include <iostream>
#include <string>
#include <vector>

class CodeGen : public Visitor {
  public:
    CodeGen(const Program &prog, std::ostream &output);

    void generate();

    // Visitor Implementation
    void visitLiteralExpr(const LiteralExpr *expr) override;
    void visitIdentifierExpr(const IdentifierExpr *expr) override;
    void visitUnaryExpr(const UnaryExpr *expr) override;
    void visitBinaryExpr(const BinaryExpr *expr) override;
    void visitGroupingExpr(const GroupingExpr *expr) override;
    void visitFunctionCallExpr(const FunctionCallExpr *expr) override;
    void visitArrayAccessExpr(const ArrayAccessExpr *expr) override;
    void visitArrayLiteralExpr(const ArrayLiteralExpr *expr) override;

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
    const Program &prog;
    std::ostream &out;
    int label_counter = 0;
    std::vector<std::pair<std::string, std::string>> string_literals;
    bool requires_bounds_panic = false;

    Type current_type = TypeSystem::Int32;
    int current_func_stack_size = 0;

    // State to track parameter index during function declaration
    int current_param_index = 0;

    std::string nextLabel(const std::string &prefix);
    void emit(const std::string &instr);
    void emitLabel(const std::string &label);

    // Helpers
    void visitAssignment(const BinaryExpr *expr);
    void genStmt(const Stmt *stmt);
    void genExpr(const Expr *expr);
};

#endif // CAPPUCCINO_CODEGEN_H_
