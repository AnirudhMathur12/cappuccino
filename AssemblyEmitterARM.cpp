//
// Created by Anirudh Mathur on 14/11/25.
//

#include "AssemblyEmitterARM.h"
#include "AbstractSyntaxTree.h"
#include <memory>

void AssemblyEmitterARM::emitPrologue(std::ostream &out) {
    out << "\t.section\t__Text,__text,regular,pure_instructions\n";
    out << "\t.globl\t_main\n";
    out << "\t.p2align\t2\n";
    out << "_main\n";
    out << "\t.cfi_startproc\n";
    out << "\tsub sp, sp, #" << stack_size << "\n";
}

void AssemblyEmitterARM::emitEpilogue(std::ostream &out) {
    out << "\tadd sp, sp, #" << stack_size << "\n";
    out.flush();
}

void AssemblyEmitterARM::emitExprStmt(const ExprStmt *s, std::ostream &out,
                                      int order) {
    if (auto es = dynamic_cast<const LiteralExpr *>(s->expr.get())) {
        if (auto intPtr = std::get_if<int>(&es->token.fd)) {
            int v = *intPtr;
        } else if (auto floatPtr = std::get_if<float>(&es->token.fd)) {
            float f = *floatPtr;
            // use f
        } else if (auto strPtr = std::get_if<std::string>(&es->token.fd)) {
            const std::string &str = *strPtr;
            // use str
        }
    }
}

void AssemblyEmitterARM::emitStmt(const Stmt *s, std::ostream &out) {
    // if (auto es = dynamic_cast<const ExprStmt*>(s)) {emitExprStmt(es, out);
    // return;} if (auto vd = dynamic_cast<const VariableDecl*>(s)) {
    // emitVarDecl(vd, out); return; } if (auto bs = dynamic_cast<const
    // BlockStmt*>(s)) { emitBlock(bs, out); return; } if (auto is =
    // dynamic_cast<const IfStmt*>(s)) { emitIf(is, out); return; } if (auto ws
    // = dynamic_cast<const WhileStmt*>(s)) { emitWhile(ws, out); return; } if
    // (auto fs = dynamic_cast<const ForStmt*>(s)) { emitFor(fs, out); return; }
    // if (auto rs = dynamic_cast<const ReturnStmt*>(s)) { emitReturn(rs, out);
    // return; }
}
