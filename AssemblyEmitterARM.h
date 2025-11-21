//
// Created by Anirudh Mathur on 14/11/25.
//

#ifndef CAPPUCCINO_ASSEMBLYEMITTERARM_H
#define CAPPUCCINO_ASSEMBLYEMITTERARM_H
#include "AbstractSyntaxTree.h"
#include <fstream>

class AssemblyEmitterARM {
  public:
    void emitProgram(const Program &program);
    std::unordered_map<std::string, int> var_offset_lookup;
    std::string file_name;

    int stack_size = 0;
    int current_function_stack_size = 0;
    int local_offset = 0;

    std::ofstream output_file;
    AssemblyEmitterARM(std::string p_file_name,
                       std::unordered_map<std::string, int> p_var_offset_lookup,
                       int p_stack_size);

  private:
    int newLabelID = 0;
    std::string newLabel(const std::string &base);

    void emitPrologue(std::ofstream &out);
    void emitEpilogue(std::ofstream &out);

    void emitStmt(const Stmt *s, std::ofstream &out);
    void emitExpr(const Expr *expr, std::ofstream &out);
    void emitExprStmt(const ExprStmt *s, std::ofstream &out);
    void emitVarDecl(const VariableDeclStmt *s, std::ofstream &out);
    void emitBlock(const BlockStmt *s, std::ofstream &out);
    void emitIf(const IfStmt *s, std::ofstream &out);
    void emitFor(const ForStmt *s, std::ofstream &out);
    void emitWhile(const WhileStmt *s, std::ofstream &out);
    void emitFunctionCall(const FunctionCallExpr *expr, std::ofstream &out);
    void emitFunctionDecl(const FunctionDeclStmt *stmt, std::ofstream &out);
    void emitReturn(const ReturnStmt *stmt, std::ofstream &out);
};

#endif // CAPPUCCINO_ASSEMBLYEMITTERARM_H
