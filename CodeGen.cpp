#include "CodeGen.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

CodeGen::CodeGen(const Program &prog, std::ostream &output)
    : prog(prog), out(output) {}

std::string CodeGen::nextLabel(const std::string &prefix) {
    return prefix + "_" + std::to_string(label_counter++);
}

void CodeGen::emit(const std::string &instr) { out << "\t" << instr << "\n"; }

void CodeGen::emitLabel(const std::string &label) { out << label << ":\n"; }

void CodeGen::pushRaw(const std::string &reg) {
    emit("str " + reg + ", [sp, #-16]!");
}

void CodeGen::popRaw(const std::string &reg) {
    emit("ldr " + reg + ", [sp], #16");
}

void CodeGen::generate() {
    out << ".globl _main\n";
    out << ".align 2\n\n";

    for (const auto &s : prog.statements) {
        genStmt(s.get());
    }

    if (!string_literals.empty()) {
        out << "\n.section __TEXT,__cstring,cstring_literals\n";
        for (const auto &[label, value] : string_literals) {
            emitLabel(label);
            out << "\t.asciz \"" << value << "\"\n";
        }
    }
}

void CodeGen::genStmt(const Stmt *stmt) {
    if (auto *s = dynamic_cast<const BlockStmt *>(stmt))
        visitBlock(s);
    else if (auto *s = dynamic_cast<const ReturnStmt *>(stmt))
        visitReturn(s);
    else if (auto *s = dynamic_cast<const VariableDeclStmt *>(stmt))
        visitVarDecl(s);
    else if (auto *s = dynamic_cast<const IfStmt *>(stmt))
        visitIf(s);
    else if (auto *s = dynamic_cast<const ForStmt *>(stmt))
        visitFor(s);
    else if (auto *s = dynamic_cast<const ExprStmt *>(stmt))
        visitExpressionStmt(s);
    else if (auto *s = dynamic_cast<const FunctionDeclStmt *>(stmt))
        visitFunctionDecl(s);
    else if (auto *s = dynamic_cast<const WhileStmt *>(stmt))
        visitWhile(s);
    else
        throw std::runtime_error("Unknown statement type encountered.");
}

void CodeGen::genExpr(const Expr *expr) {
    if (auto *e = dynamic_cast<const LiteralExpr *>(expr))
        visitLiteral(e);
    else if (auto *e = dynamic_cast<const IdentifierExpr *>(expr))
        visitIdentifier(e);
    else if (auto *e = dynamic_cast<const UnaryExpr *>(expr))
        visitUnary(e);
    else if (auto *e = dynamic_cast<const GroupingExpr *>(expr))
        visitGrouping(e);
    else if (auto *e = dynamic_cast<const FunctionCallExpr *>(expr))
        visitFunctionCall(e);
    else if (auto *e = dynamic_cast<const BinaryExpr *>(expr)) {
        if (e->op.type == OPERATOR_ASSIGNMENT)
            visitAssignment(e);
        else
            visitBinary(e);
    } else
        throw std::runtime_error("Unknown expression type encountered.");
}

void CodeGen::visitFunctionDecl(const FunctionDeclStmt *stmt) {
    std::string name = "_" + stmt->name_token.lexeme;
    emitLabel(name);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    int stackSize = prog.stack_size;
    if (stackSize % 16 != 0)
        stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("sub sp, sp, #" + std::to_string(stackSize));
    }

    int i = 0;
    for (const auto &param : stmt->params) {
        if (auto *p =
                dynamic_cast<const FunctionParameterStmt *>(param.get())) {
            if (prog.var_offset_lookup.count(p->name)) {
                int offset = prog.var_offset_lookup.at(p->name);
                std::string reg = "x" + std::to_string(i);

                emit("stur " + reg + ", [x29, #-" + std::to_string(offset) +
                     "]");
            }
        }

        i++;
        if (i > 7)
            break; // only upto 8 args in registers
    }

    genStmt(stmt->body.get());

    emit("mov x0, #0");
    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
}

void CodeGen::visitReturn(const ReturnStmt *stmt) {
    if (stmt->value) {
        genExpr(stmt->value.value().get()); // Evaluate result into x0
    } else {
        emit("mov x0, #0");
    }

    int stackSize = prog.stack_size;
    if (stackSize % 16 != 0)
        stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
}

void CodeGen::visitLiteral(const LiteralExpr *expr) {
    if (std::holds_alternative<int64_t>(expr->token.fd)) {
        int64_t val = std::get<int64_t>(expr->token.fd);

        emit("ldr x0, =" + std::to_string(val));
    } else if (std::holds_alternative<std::string>(expr->token.fd)) {
        std::string val = std::get<std::string>(expr->token.fd);
        std::string label = nextLabel("L_str");

        string_literals.push_back({label, val});

        emit("adrp x0, " + label + "@PAGE");
        emit("add x0, x0, " + label + "@PAGEOFF");
    }
}

void CodeGen::visitIdentifier(const IdentifierExpr *expr) {
    emit("ldur x0, [x29, #-" + std::to_string(expr->offset) + "]");
}

void CodeGen::visitAssignment(const BinaryExpr *expr) {
    auto *ident = dynamic_cast<const IdentifierExpr *>(expr->left.get());
    if (!ident)
        throw std::runtime_error("LHS of assignment must be a variable.");

    genExpr(expr->right.get());

    emit("stur x0, [x29, #-" + std::to_string(ident->offset) + "]");
}

void CodeGen::visitBlock(const BlockStmt *stmt) {
    for (const auto &s : stmt->statements) {
        genStmt(s.get());
    }
}

void CodeGen::visitVarDecl(const VariableDeclStmt *stmt) {
    if (stmt->initializer) {
        genExpr(stmt->initializer.value().get()); // Result in x0
        emit("stur x0, [x29, #-" + std::to_string(stmt->offset) + "]");
    }
}

void CodeGen::visitExpressionStmt(const ExprStmt *stmt) {
    genExpr(stmt->expr.get());
}

void CodeGen::visitIf(const IfStmt *stmt) {
    std::string labelElse = nextLabel("L_else");
    std::string labelEnd = nextLabel("L_if_end");

    genExpr(stmt->condition.get());
    emit("cmp x0, #0");
    emit("b.eq " + labelElse);

    genStmt(stmt->then_branch.get());
    emit("b " + labelEnd);

    emitLabel(labelElse);
    if (stmt->else_branch) {
        genStmt(stmt->else_branch.value().get());
    }
    emitLabel(labelEnd);
}

void CodeGen::visitFor(const ForStmt *stmt) {
    std::string labelStart = nextLabel("L_for_start");
    std::string labelEnd = nextLabel("L_for_end");

    if (stmt->initializer) {
        // Initializer is often a Stmt (VarDecl or ExprStmt)
        genStmt(stmt->initializer.value().get());
    }

    emitLabel(labelStart);

    if (stmt->condition) {
        genExpr(stmt->condition.value().get());
        emit("cmp x0, #0");
        emit("b.eq " + labelEnd);
    }

    genStmt(stmt->body.get());

    if (stmt->increment) {
        genExpr(stmt->increment.value().get());
    }

    emit("b " + labelStart);
    emitLabel(labelEnd);
}

void CodeGen::visitWhile(const WhileStmt *stmt) {
    std::string labelStart = nextLabel("L_while_start");
    std::string labelEnd = nextLabel("L_while_end");

    emitLabel(labelStart);

    genExpr(stmt->condition.get());
    emit("cmp x0, #0");
    emit("b.eq " + labelEnd);

    genStmt(stmt->body.get());

    emit("b " + labelStart);

    emitLabel(labelEnd);
}

void CodeGen::visitBinary(const BinaryExpr *expr) {
    genExpr(expr->left.get());
    pushRaw("x0");

    genExpr(expr->right.get());
    emit("mov x1, x0");
    popRaw("x0");

    switch (expr->op.type) {
    case OPERATOR_PLUS:
        emit("add x0, x0, x1");
        break;
    case OPERATOR_MINUS:
        emit("sub x0, x0, x1");
        break;
    case OPERATOR_ASTERISK:
        emit("mul x0, x0, x1");
        break;
    case OPERATOR_FORWARD_SLASH:
        emit("sdiv x0, x0, x1");
        break;

    case OPERATOR_LESS:
        emit("cmp x0, x1");
        emit("cset x0, lt");
        break;
    case OPERATOR_LESS_EQUALS:
        emit("cmp x0, x1");
        emit("cset x0, le");
        break;
    case OPERATOR_GREATER:
        emit("cmp x0, x1");
        emit("cset x0, gt");
        break;
    case OPERATOR_GREATER_EQUALS:
        emit("cmp x0, x1");
        emit("cset x0, ge");
        break;
    case OPERATOR_EQUALITY:
        emit("cmp x0, x1");
        emit("cset x0, eq");
        break;
    case EXCL_EQUAL:
        emit("cmp x0, x1");
        emit("cset x0, ne");
        break;

    default:
        throw std::runtime_error("Unknown binary operator in CodeGen");
    }
}

void CodeGen::visitGrouping(const GroupingExpr *expr) {
    genExpr(expr->expr.get());
}

void CodeGen::visitUnary(const UnaryExpr *expr) {
    genExpr(expr->right.get());

    switch (expr->op.type) {
    case OPERATOR_MINUS:
        emit("neg x0, x0");
        break;
    case EXCLAMATION:
        emit("cmp x0, #0");
        emit("cset x0, eq");
        break;
    default:
        throw std::runtime_error("Unknown unary operator");
    }
}

void CodeGen::visitFunctionCall(const FunctionCallExpr *expr) {
    for (const auto &arg : expr->args) {
        genExpr(arg.get());
        pushRaw("x0");
    }

    int argCount = expr->args.size();
    for (int i = argCount - 1; i >= 0; --i) {
        if (i < 8) {
            popRaw("x" + std::to_string(i));
        } else {
            // Arguments > 8 should stay on stack (standard convention).
            // But for simplicity in this V1, let's assume < 8 args.
        }
    }

    std::string funcName = "_" + expr->name_token.lexeme;
    emit("bl " + funcName);
}
