//
// Created by Anirudh Mathur on 14/11/25.
//

#ifndef CAPPUCCINO_ASSEMBLYEMITTERARM_H
#define CAPPUCCINO_ASSEMBLYEMITTERARM_H
#include "AbstractSyntaxTree.h"

class AssemblyEmitterARM {
  public:
    void emitProgram(const Program &program);
    std::unordered_map<std::string, int> var_offset_lookup;

    int stack_size = 0;

  private:
    int newLabelID = 0;
    std::string newLabel(const std::string &base);

    void emitPrologue(std::ostream &out);
    void emitEpilogue(std::ostream &out);

    void emitStmt(const Stmt *s, std::ostream &out);
    void emitExprStmt(const ExprStmt *s, std::ostream &out, int order);
    void emitVarDecl(const VariableDeclStmt *s, std::ostream &out);
    void emitBlock(const BlockStmt *s, std::ostream &out);
    void emitIf(const IfStmt *s, std::ostream &out);
    void emitFor(const ForStmt *s, std::ostream &out);
    void emitWhile(const WhileStmt *s, std::ostream &out);
};

#endif // CAPPUCCINO_ASSEMBLYEMITTERARM_H
