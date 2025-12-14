#ifndef CAPPUCCINO_CODEGEN_H_
#define CAPPUCCINO_CODEGEN_H_

#include "AbstractSyntaxTree.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class CodeGen {
  public:
    CodeGen(const Program &prog, std::ostream &output);

    void generate();

  private:
    const Program &prog;
    std::ostream &out;
    int label_counter = 0;
    std::vector<std::pair<std::string, std::string>> string_literals;

    std::string current_type = "int";
    std::string nextLabel(const std::string &prefix);
    int current_func_stack_size = 0;

    void emit(const std::string &instr);

    void emitLabel(const std::string &label);

    void pushRaw(const std::string &reg);

    void popRaw(const std::string &reg);

    void genStmt(const Stmt *stmt);
    void genExpr(const Expr *expr);

    void visitLiteral(const LiteralExpr *expr);
    void visitIdentifier(const IdentifierExpr *expr);
    void visitBinary(const BinaryExpr *expr);
    void visitUnary(const UnaryExpr *expr);
    void visitGrouping(const GroupingExpr *expr);
    void visitFunctionCall(const FunctionCallExpr *expr);
    void visitAssignment(const BinaryExpr *expr);

    void visitBlock(const BlockStmt *stmt);
    void visitReturn(const ReturnStmt *stmt);
    void visitVarDecl(const VariableDeclStmt *stmt);
    void visitIf(const IfStmt *stmt);
    void visitWhile(const WhileStmt *stmt);
    void visitFor(const ForStmt *stmt);
    void visitExpressionStmt(const ExprStmt *stmt);
    void visitFunctionDecl(const FunctionDeclStmt *stmt);
};

#endif // CAPPUCCINO_CODEGEN_H_
