//
// Created by Anirudh Mathur on 14/11/25.
//
//

#include "AssemblyEmitterARM.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <cstdint>
#include <fstream>

inline int align_to_16(int n) { return (n + 15) & ~15; }

AssemblyEmitterARM::AssemblyEmitterARM(
    std::string p_file_name,
    std::unordered_map<std::string, int> p_var_offset_lookup, int p_stack_size)
    : file_name(p_file_name), var_offset_lookup(p_var_offset_lookup),
      stack_size(p_stack_size) {
    file_name = file_name.substr(0, file_name.find_last_of("."));
    output_file.open(file_name + ".s");
}

void AssemblyEmitterARM::emitPrologue(std::ofstream &out) {
    out << "\t.section\t__Text,__text,regular,pure_instructions\n";
    out << "\t.globl\t_main\n";
    out << "\t.p2align\t2\n";
    out << ":\n";

    out << "\tstp\tx29, x30, [sp, #-16]!\n";
    out << "\tmov\tx29, sp\n";

    int aligned_size = align_to_16(stack_size);
    if (aligned_size > 0) {
        out << "\tsub\tsp, sp, #" << aligned_size << "\n";
    }
}

void AssemblyEmitterARM::emitEpilogue(std::ofstream &out) {
    int aligned_size = align_to_16(stack_size);
    if (aligned_size > 0) {
        out << "\tadd\tsp, sp, #" << aligned_size << "\n";
    }

    out << "\tldp\tx29, x30, [sp], #16\n";
    out << "\tret\n";
    out.flush();
}

void AssemblyEmitterARM::emitExpr(const Expr *expr, std::ofstream &out) {
    // Literals
    if (auto *lit = dynamic_cast<const LiteralExpr *>(expr)) {
        if (auto intPtr = std::get_if<int64_t>(&lit->token.fd)) {
            out << "\tldr\tx0, =" << *intPtr << "\n";
            out << "\tstr\tx0, [sp, #-16]!\n";
        }
        // TODO: Add floating point and string later

        return;
    }

    if (auto *ident = dynamic_cast<const IdentifierExpr *>(expr)) {
        std::string name = ident->name;

        if (var_offset_lookup.find(name) == var_offset_lookup.end()) {
            std::cerr << "Error: Undefined identifier " << name << "\n";
            exit(1);
        }

        int offset = var_offset_lookup[name];

        out << "\tldur\tx0, [x29, #" << offset << "]\n";
        out << "\tstr\tx0, [sp, #-16]!\n";

        return;
    }

    if (auto *bin = dynamic_cast<const BinaryExpr *>(expr)) {
        emitExpr(bin->left.get(), out);
        emitExpr(bin->right.get(), out);

        out << "\tldr\tx1, [sp], #16\n";
        out << "\tldr\tx0, [sp], #16\n";

        TokenType type = bin->op.type;
        if (type == OPERATOR_PLUS) {
            out << "\tadd\tx0, x0, x1\n";
            out << "\tstr\tx0, [sp, #-16]!\n";
        } else if (type == OPERATOR_MINUS) {
            out << "\tsub\tx0, x0, x1\n";
            out << "\tstr\tx0, [sp, #-16]!\n";
        } else if (type == OPERATOR_ASTERISK) {
            out << "\tmul\tx0, x0, x1\n";
            out << "\tstr\tx0, [sp, #-16]!\n";
        } else if (type == OPERATOR_FORWARD_SLASH) {
            out << "\tdiv\tx0, x0, x1\n";
            out << "\tstr\tx0, [sp, #-16]!\n";
        } else if (type == OPERATOR_LESS) {
            out << "\tcmp\tx0, x1\n";
        }
    }

    if (auto *group = dynamic_cast<const GroupingExpr *>(expr)) {
        emitExpr(group->expr.get(), out);
    }

    if (auto *call = dynamic_cast<const FunctionCallExpr *>(expr)) {
        emitFunctionCall(call, out);
        return;
    }
}

void AssemblyEmitterARM::emitFunctionCall(const FunctionCallExpr *expr,
                                          std::ofstream &out) {
    for (const auto &arg : expr->args) {
        emitExpr(arg.get(), out);
    }

    for (int i = expr->args.size() - 1; i >= 0; i--) {
        out << "\tldr\tx" << i << ", [sp], #16\n";
    }

    out << "\tbl\t_" << expr->name_token.lexeme << "\n";

    out << "\tstr\tx0, [sp, #-16]!\n";
}

void AssemblyEmitterARM::emitExprStmt(const ExprStmt *expstmt,
                                      std::ofstream &out) {
    emitExpr(expstmt->expr.get(), out);

    out << "\tldr\tx0, [sp], #16\n";
}

void AssemblyEmitterARM::emitFunctionDecl(const FunctionDeclStmt *stmt,
                                          std::ofstream &out) {
    std::string function_name = stmt->name_token.lexeme;

    out << "\t.globl\t_" << function_name << "\n";
    out << "\t.p2align\t2\n";
    out << "_" << function_name << ":\n";

    // Does not dynamically allocate stack size, need to do that later.
    out << "\tstp\tx29, x30, [sp, #-16]!\n";
    out << "\tmov\tx29, sp\n";

    int param_size = stmt->params.size() * 16; // 16-byte alignemnt
    out << "\tsub\tsp, sp, #256\n";            // Lazy fix

    int local_offset = 0;
    int arg_reg = 0;

    for (const auto &param_stmt : stmt->params) {
        if (auto parameter =
                dynamic_cast<const FunctionParameterStmt *>(param_stmt.get())) {
            // align to 8 btyes to match x registers

            local_offset -= 8;
            out << "\tstr\tx" << arg_reg << ", [x29, #" << local_offset
                << "]\n";

            var_offset_lookup[parameter->name] = local_offset;

            arg_reg++;
        }
    }

    if (auto *block = dynamic_cast<const BlockStmt *>(stmt->body.get())) {
        for (const auto &s : block->statements) {
            emitStmt(s.get(), out);
        }
    } else {

        out << "\tmov\tsp, x29\n";
        out << "\tldp\tx29, x30, [sp], #16\n";
        out << "\tret\n";
    }
}

void AssemblyEmitterARM::emitReturn(const ReturnStmt *stmt,
                                    std::ofstream &out) {
    if (stmt->value) {
        emitExpr(stmt->value.value().get(), out);
        out << "\tldr\tx0, [sp], #16\n";
    }

    out << "\tmov\tsp, x29\n";

    out << "\tmov\tsp, x29\n";
    out << "\tldp\tx29, x30, [sp], #16\n";
    out << "\tret\n";
}

void AssemblyEmitterARM::emitStmt(const Stmt *s, std::ofstream &out) {
    if (auto *fs = dynamic_cast<const FunctionDeclStmt *>(s)) {
        emitFunctionDecl(fs, out);
        return;
    }

    if (auto *rs = dynamic_cast<const ReturnStmt *>(s)) {
        emitReturn(rs, out);
        return;
    }

    if (auto *exs = dynamic_cast<const ExprStmt *>(s)) {
        emitExprStmt(exs, out);
        return;
    }

    if (auto *vds = dynamic_cast<const VariableDeclStmt *>(s)) {
        emitVarDecl(vds, out);
    }

    if (auto *forStmt = dynamic_cast<const ForStmt *>(s)) {
        emitFor(forStmt, out);
        return;
    }
}

void AssemblyEmitterARM::emitFor(const ForStmt *stmt, std::ofstream &out) {
    std::string start_label = newLabel("for_start");
    std::string end_label = newLabel("for_end");

    if (stmt->initializer.has_value()) {
        emitStmt(stmt->initializer.value().get(), out);
    }

    out << start_label << ":\n";

    if (stmt->condition.has_value()) {
        emitExpr(stmt->condition.value().get(), out);

        out << "\tldr\tx0, [sp], #16\n";

        out << "\tcmp\tx0, #0\n";

        out << "\tb.eq\t" << end_label << "\n";
    }

    emitStmt(stmt->body.get(), out);

    if (stmt->increment.has_value()) {
        emitExpr(stmt->increment.value().get(), out);
        out << "\tldr\tx0, [sp], #16\n";
    }

    out << "\tb\t" << start_label << "\n";

    out << end_label << ":\n";
}

void AssemblyEmitterARM::emitVarDecl(const VariableDeclStmt *s,
                                     std::ofstream &out) {
    if (s->initializer.has_value()) {
        emitExpr(s->initializer.value().get(), out);
    }

    local_offset -= 8;

    var_offset_lookup[s->name] = local_offset;

    out << "\tldr\tx0, [sp], #16\n";
    out << "\tstur x0, [x29, #" << local_offset << "]\n";
}

std::string AssemblyEmitterARM::newLabel(const std::string &base) {
    return "_" + base + "_" + std::to_string(newLabelID++);
}

void AssemblyEmitterARM::emitProgram(const Program &prog) {
    for (auto &s : prog.statements) {
        emitStmt(s.get(), output_file);
    }
}
