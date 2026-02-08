#include "CodeGen.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include "Type.h"
#include "capp_stdlib.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

CodeGen::CodeGen(const Program &prog, std::ostream &output) : prog(prog), out(output), current_type(TypeSystem::Int32) {}

std::string CodeGen::nextLabel(const std::string &prefix) { return prefix + "_" + std::to_string(label_counter++); }

void CodeGen::emit(const std::string &instr) { out << "\t" << instr << "\n"; }

void CodeGen::emitLabel(const std::string &label) { out << label << ":\n"; }

void CodeGen::pushRaw(const std::string &reg) { emit("str " + reg + ", [sp, #-16]!"); }

void CodeGen::popRaw(const std::string &reg) { emit("ldr " + reg + ", [sp], #16"); }

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

    out << STDLIB_ASM;
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
        if (e->op.type == TokenType::OPERATOR_ASSIGNMENT)
            visitAssignment(e);
        else
            visitBinary(e);
    } else
        throw std::runtime_error("Unknown expression type encountered.");
}

void CodeGen::visitFunctionDecl(const FunctionDeclStmt *stmt) {
    std::string name = "_" + stmt->name_token.lexeme;
    std::cout << "CodeGen for function: " << name << std::endl;

    int saved_stack_size = current_func_stack_size;
    current_func_stack_size = stmt->stack_size;

    std::cout << "  function stack_size = " << stmt->stack_size << std::endl;
    emitLabel(name);

    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");

    int stackSize = stmt->stack_size; // Use function's own stack size
    if (stackSize % 16 != 0) stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("sub sp, sp, #" + std::to_string(stackSize));
    }

    // Store parameters from registers
    int i = 0;
    for (const auto &param : stmt->params) {
        if (auto *p = dynamic_cast<const FunctionParameterStmt *>(param.get())) {
            int offset = p->offset; // Use offset from AST

            std::cout << "  Param '" << p->name << "' using offset: " << offset << std::endl;

            Type param_type = TypeSystem::from_string(p->type_token.lexeme);

            if (param_type.is_float) {
                std::string reg = (param_type.size_bytes == 4 ? "s" : "d") + std::to_string(i);
                emit("stur " + reg + ", [x29, #-" + std::to_string(offset) + "]");
            } else {
                std::string reg = (param_type.size_bytes < 8 ? "w" : "x") + std::to_string(i);

                if (param_type.size_bytes == 1) {
                    emit("sturb " + reg + ", [x29, #-" + std::to_string(offset) + "]");
                } else if (param_type.size_bytes == 2) {
                    emit("sturh " + reg + ", [x29, #-" + std::to_string(offset) + "]");
                } else if (param_type.size_bytes == 4) {
                    emit("stur " + reg + ", [x29, #-" + std::to_string(offset) + "]");
                } else {
                    emit("stur " + reg + ", [x29, #-" + std::to_string(offset) + "]");
                }
            }
        }
        i++;
        if (i > 7) break;
    }

    // Generate function body
    genStmt(stmt->body.get());

    // Epilogue
    emit("mov x0, #0");
    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    current_func_stack_size = saved_stack_size;
}

void CodeGen::visitReturn(const ReturnStmt *stmt) {
    if (stmt->value) {
        genExpr(stmt->value.value().get()); // Evaluate result into x0
    } else {
        emit("mov x0, #0");
    }

    int stackSize = current_func_stack_size;
    if (stackSize % 16 != 0) stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
}

void CodeGen::visitLiteral(const LiteralExpr *expr) {
    if (std::holds_alternative<uint64_t>(expr->token.fd)) {
        uint64_t val = std::get<uint64_t>(expr->token.fd);
        emit("ldr x0, =" + std::to_string(val));
        current_type = TypeSystem::Int64;
    } else if (std::holds_alternative<double>(expr->token.fd)) {
        double val = std::get<double>(expr->token.fd);
        std::string label = nextLabel("L_float");

        emit(".section __TEXT,__literal8,8byte_literals");
        emitLabel(label);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(15) << val;
        emit(".double " + oss.str());

        emit(".section __TEXT,__text,regular,pure_instructions");

        emit("adrp x0, " + label + "@PAGE");
        emit("ldr d0, [x0, " + label + "@PAGEOFF]");

        current_type = TypeSystem::Float64;
    } else if (std::holds_alternative<std::string>(expr->token.fd)) {
        std::string val = std::get<std::string>(expr->token.fd);
        std::string label = nextLabel("L_str");

        string_literals.push_back({label, val});

        emit("adrp x0, " + label + "@PAGE");
        emit("add x0, x0, " + label + "@PAGEOFF");
        current_type = TypeSystem::StringLiteral;
    }
}

void CodeGen::visitIdentifier(const IdentifierExpr *expr) {
    current_type = expr->type;

    if (current_type.is_float) {
        if (current_type.size_bytes == 4) {
            emit("ldur s0, [x29, #-" + std::to_string(expr->offset) + "]");
        } else {
            emit("ldur d0, [x29, #-" + std::to_string(expr->offset) + "]");
        }
    } else {
        if (current_type.size_bytes == 1) {
            if (current_type.is_signed) {
                emit("ldursb x0, [x29, #-" + std::to_string(expr->offset) + "]");
            } else {
                emit("ldurb w0, [x29, #-" + std::to_string(expr->offset) + "]");
            }
        } else if (current_type.size_bytes == 2) {
            if (current_type.is_signed) {
                emit("ldursh x0, [x29, #-" + std::to_string(expr->offset) + "]");
            } else {
                emit("ldurh w0, [x29, #-" + std::to_string(expr->offset) + "]");
            }
        } else if (current_type.size_bytes == 4) {
            emit("ldur w0, [x29, #-" + std::to_string(expr->offset) + "]");
        } else {
            emit("ldur x0, [x29, #-" + std::to_string(expr->offset) + "]");
        }
    }
}

void CodeGen::visitAssignment(const BinaryExpr *expr) {
    auto *ident = dynamic_cast<const IdentifierExpr *>(expr->left.get());
    if (!ident) throw std::runtime_error("LHS of assignment must be a variable.");

    genExpr(expr->right.get());

    Type varType = ident->type;

    if (varType.is_float) {
        if (!current_type.is_float) {
            emit("scvtf d0, x0");
            current_type = TypeSystem::Float64;
        }

        if (varType.size_bytes == 4 && current_type.size_bytes == 8)
            emit("fcvt s0, d0");
        else if (varType.size_bytes == 8 && current_type.size_bytes == 4)
            emit("fcvt d0, s0");

        if (varType.size_bytes == 4) {
            emit("stur s0, [x29, #-" + std::to_string(ident->offset) + "]");
        } else {
            emit("stur d0, [x29, #-" + std::to_string(ident->offset) + "]");
        }

    } else {
        if (current_type.is_float) {
            emit("fcvtzs x0, d0");
        }
        if (varType.size_bytes == 1) {
            emit("sturb w0, [x29, #-" + std::to_string(ident->offset) + "]");
        } else if (varType.size_bytes == 2) {
            emit("sturh w0, [x29, #-" + std::to_string(ident->offset) + "]");
        } else if (varType.size_bytes == 4) {
            emit("stur w0, [x29, #-" + std::to_string(ident->offset) + "]");
        } else {
            emit("stur d0, [x29, #-" + std::to_string(ident->offset) + "]");
        }
    }
}

void CodeGen::visitBlock(const BlockStmt *stmt) {
    for (const auto &s : stmt->statements) {
        genStmt(s.get());
    }
}

void CodeGen::visitVarDecl(const VariableDeclStmt *stmt) {
    if (stmt->initializer) {
        genExpr(stmt->initializer.value().get());

        Type varType = TypeSystem::from_string(stmt->type_token.lexeme);

        if (varType.is_float) {
            // Int -> Float
            if (!current_type.is_float) {
                emit("scvtf d0, x0");
                current_type = (current_type.size_bytes == 4) ? TypeSystem::Float32 : TypeSystem::Float64;
            }

            // Float64 -> Float32 (Downcast)
            if (varType.size_bytes == 4 && current_type.size_bytes == 8) {
                emit("fcvt s0, d0");
            }
            // Float32 -> Float64 (Upcast)
            else if (varType.size_bytes == 8 && current_type.size_bytes == 4) {
                emit("fcvt d0, s0");
            }
        }
        // 2. Handle Int conversions (Simple cast)
        else {
            if (current_type.is_float) {
                emit("fcvtzs x0, d0"); // Float -> Int
            }
        }

        if (varType.is_float) {
            if (varType.size_bytes == 4) {
                emit("stur s0, [x29, #-" + std::to_string(stmt->offset) + "]");
            } else {
                emit("stur d0, [x29, #-" + std::to_string(stmt->offset) + "]");
            }
        } else {
            if (varType.size_bytes == 1) {
                emit("sturb w0, [x29, #-" + std::to_string(stmt->offset) + "]");
            } else if (varType.size_bytes == 2) {
                emit("sturh w0, [x29, #-" + std::to_string(stmt->offset) + "]");
            } else if (varType.size_bytes == 4) {
                emit("stur w0, [x29, #-" + std::to_string(stmt->offset) + "]");
            } else {
                emit("stur x0, [x29, #-" + std::to_string(stmt->offset) + "]");
            }
        }
    }
}

void CodeGen::visitExpressionStmt(const ExprStmt *stmt) { genExpr(stmt->expr.get()); }

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
    Type leftType = current_type;

    if (leftType.is_float)
        emit("str d0, [sp, #-16]!");
    else
        emit("str x0, [sp, #-16]!");

    genExpr(expr->right.get());
    Type rightType = current_type;

    if (rightType.is_float)
        emit("fmov d1, d0");
    else
        emit("mov x1, x0");

    if (leftType.is_float)
        emit("ldr d0, [sp], #16");
    else
        emit("ldr x0, [sp], #16");

    if (!leftType.is_float && rightType.is_float) {
        emit("scvtf d0, x0");
        leftType = TypeSystem::Float64;
    } else if (leftType.is_float && !rightType.is_float) {
        emit("scvtf d1, x1");
        rightType = TypeSystem::Float64;
    }

    if (leftType.is_float && rightType.is_float) {
        if (rightType.size_bytes == 4 && leftType.size_bytes == 4) {
            current_type = TypeSystem::Float32;
            switch (expr->op.type) {
            case TokenType::OPERATOR_PLUS: emit("fadd s0, s0, s1"); break;
            case TokenType::OPERATOR_MINUS: emit("fsub s0, s0, s1"); break;
            case TokenType::OPERATOR_ASTERISK: emit("fmul s0, s0, s1"); break;
            case TokenType::OPERATOR_FORWARD_SLASH: emit("fdiv s0, s0, s1"); break;

            case TokenType::OPERATOR_LESS:
                emit("fcmp s0, s1");
                emit("cset x0, mi"); // mi = Minus (Less Than)
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_LESS_EQUALS:
                emit("fcmp s0, s1");
                emit("cset x0, le"); // le = Less or Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER:
                emit("fcmp s0, s1");
                emit("cset x0, gt"); // gt = Greater Than
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER_EQUALS:
                emit("fcmp s0, s1");
                emit("cset x0, ge"); // ge = Greater or Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_EQUALITY:
                emit("fcmp s0, s1");
                emit("cset x0, eq"); // eq = Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::EXCL_EQUAL:
                emit("fcmp s0, s1");
                emit("cset x0, ne"); // ne = Not Equal
                current_type = TypeSystem::Int64;
                break;

            default: break;
            }
        } else {
            current_type = TypeSystem::Float64;
            if (leftType.size_bytes == 4) emit("fcvt d0, s0");

            if (rightType.size_bytes == 4) emit("fcvt d1, s1");

            switch (expr->op.type) {
            case TokenType::OPERATOR_PLUS: emit("fadd d0, d0, d1"); break;
            case TokenType::OPERATOR_MINUS: emit("fsub d0, d0, d1"); break;
            case TokenType::OPERATOR_ASTERISK: emit("fmul d0, d0, d1"); break;
            case TokenType::OPERATOR_FORWARD_SLASH: emit("fdiv d0, d0, d1"); break;

            case TokenType::OPERATOR_LESS:
                emit("fcmp d0, d1");
                emit("cset x0, mi"); // mi = Minus (Less Than)
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_LESS_EQUALS:
                emit("fcmp d0, d1");
                emit("cset x0, le"); // le = Less or Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER:
                emit("fcmp d0, d1");
                emit("cset x0, gt"); // gt = Greater Than
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER_EQUALS:
                emit("fcmp d0, d1");
                emit("cset x0, ge"); // ge = Greater or Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_EQUALITY:
                emit("fcmp d0, d1");
                emit("cset x0, eq"); // eq = Equal
                current_type = TypeSystem::Int64;
                break;
            case TokenType::EXCL_EQUAL:
                emit("fcmp d0, d1");
                emit("cset x0, ne"); // ne = Not Equal
                current_type = TypeSystem::Int64;
                break;

            default: break;
            }
        }
    } else {
        current_type = TypeSystem::Int64;

        bool is_unsigned_math = (!leftType.is_signed || !rightType.is_signed);

        switch (expr->op.type) {
        case TokenType::OPERATOR_PLUS: emit("add x0, x0, x1"); break;
        case TokenType::OPERATOR_MINUS: emit("sub x0, x0, x1"); break;
        case TokenType::OPERATOR_ASTERISK: emit("mul x0, x0, x1"); break;
        case TokenType::OPERATOR_FORWARD_SLASH:
            if (is_unsigned_math)
                emit("udiv x0, x0, x1");
            else
                emit("sdiv x0, x0, x1");
            break;

        case TokenType::OPERATOR_LESS:
            if (leftType.is_signed && rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, lt");
            } else if (!leftType.is_signed && !rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, lo");
            } else {
                // Left < 0 or Left_Unsigned < Right_Unsigned
                if (leftType.is_signed) {
                    emit("cmp x0, #0");
                    emit("cset x2, mi"); // x2 = 1 if Left is negative
                    emit("cmp x0, x1");
                    emit("cset x3, lo"); // x3 = 1 if Left < Right (numerically)
                    emit("orr x0, x2, x3");
                } else {
                    // Right is Signed. Right >= 0 and Left < Right
                    emit("cmp x1, #0");
                    emit("cset x2, pl"); // x2 = 1 if Right is positive
                    emit("cmp x0, x1");
                    emit("cset x3, lo"); // x3 = 1 if Left < Right
                    emit("and x0, x2, x3");
                }
            }
            break;
        case TokenType::OPERATOR_LESS_EQUALS:
            if (leftType.is_signed && rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, le");
            } else if (!leftType.is_signed && !rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, ls");
            } else {
                if (leftType.is_signed) {
                    emit("cmp x0, #0");
                    emit("cset x2, mi");
                    emit("cmp x0, x1");
                    emit("cset x3, ls");
                    emit("orr x0, x2, x3");
                } else {
                    emit("cmp x1, #0");
                    emit("cset x2, pl");
                    emit("cmp x0, x1");
                    emit("cset x3, ls");
                    emit("and x0, x2, x3");
                }
            }
            break;
        case TokenType::OPERATOR_GREATER:
            if (leftType.is_signed && rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, gt");
            } else if (!leftType.is_signed && !rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, hi");
            } else {
                if (leftType.is_signed) {
                    // Left >= 0 and Left > Right
                    emit("cmp x0, #0");
                    emit("cset x2, pl");
                    emit("cmp x0, x1");
                    emit("cset x3, hi");
                    emit("and x0, x2, x3");
                } else {
                    // Right is Signed. Right < 0 OR Left > Right
                    emit("cmp x1, #0");
                    emit("cset x2, mi");
                    emit("cmp x0, x1");
                    emit("cset x3, hi");
                    emit("orr x0, x2, x3");
                }
            }
            break;
        case TokenType::OPERATOR_GREATER_EQUALS:
            if (leftType.is_signed && rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, ge");
            } else if (!leftType.is_signed && !rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, hs");
            } else {
                if (leftType.is_signed) {
                    emit("cmp x0, #0");
                    emit("cset x2, pl");
                    emit("cmp x0, x1");
                    emit("cset x3, hs");
                    emit("and x0, x2, x3");
                } else {
                    emit("cmp x1, #0");
                    emit("cset x2, mi");
                    emit("cmp x0, x1");
                    emit("cset x3, hs");
                    emit("orr x0, x2, x3");
                }
            }
            break;
        case TokenType::OPERATOR_EQUALITY:
            emit("cmp x0, x1");
            emit("cset x0, eq");
            break;
        case TokenType::EXCL_EQUAL:
            emit("cmp x0, x1");
            emit("cset x0, ne");
            break;
        default: break;
        }
    }
}

void CodeGen::visitGrouping(const GroupingExpr *expr) { genExpr(expr->expr.get()); }

void CodeGen::visitUnary(const UnaryExpr *expr) {
    genExpr(expr->right.get());

    switch (expr->op.type) {
    case TokenType::OPERATOR_MINUS:
        if (current_type.is_float) {
            if (current_type.size_bytes == 4) {
                emit("fneg s0, s0");
            } else {
                emit("fneg d0, d0");
            }
        } else {
            if (current_type.size_bytes == 4) {
                emit("neg w0, w0");
            } else {
                emit("neg x0, x0");
            }
        }
        break;
    case TokenType::EXCLAMATION:
        if (current_type.is_float) {
            if (current_type.size_bytes == 4) {
                emit("fcmp s0, #0.0");
                emit("cset x0, eq");
            } else {
                emit("fcmp d0, #0.0");
                emit("cset x0, eq");
            }
            current_type = TypeSystem::Int64;
        } else {
            if (current_type.size_bytes == 4) {
                emit("cmp w0, #0");
                emit("cset w0, eq");
            } else {
                emit("cmp x0, #0");
                emit("cset x0, eq");
            }
        }
        break;
    default: throw std::runtime_error("Unknown unary operator");
    }
}

void CodeGen::visitFunctionCall(const FunctionCallExpr *expr) {
    // Evaluate arguments and push them (preserving type info)
    std::vector<Type> argTypes;
    for (int i = 0; i < expr->args.size(); i++) {
        const auto &arg = expr->args[i];
        genExpr(arg.get());

        if (i < expr->param_types.size()) {
            Type expected = expr->param_types[i];

            // Float Casts
            if (expected.is_float) {
                if (!current_type.is_float) {
                    emit("scvtf d0, x0");               // Int -> Float
                    current_type = TypeSystem::Float64; // Treat as double initially
                }

                if (expected.size_bytes == 8 && current_type.size_bytes == 4) {
                    emit("fcvt d0, s0"); // float -> double
                    current_type = TypeSystem::Float64;
                } else if (expected.size_bytes == 4 && current_type.size_bytes == 8) {
                    emit("fcvt s0, d0"); // double -> float
                    current_type = TypeSystem::Float32;
                }
            }
            // Int Casts (Optional, but good for safety)
            else if (!expected.is_float && current_type.is_float) {
                emit("fcvtzs x0, d0"); // Float -> Int
                current_type = TypeSystem::Int64;
            }
        }

        argTypes.push_back(current_type);

        if (current_type.is_float) {
            emit("str d0, [sp, #-16]!"); // Push float register directly
        } else {
            emit("str x0, [sp, #-16]!"); // Push int register
        }
    }

    // Pop arguments into parameter registers in reverse order
    int argCount = expr->args.size();
    for (int i = argCount - 1; i >= 0; --i) {
        if (i < 8) {
            if (argTypes[i].is_float) {
                if (argTypes[i].size_bytes == 4)
                    emit("ldr s" + std::to_string(i) + ", [sp], #16");
                else
                    emit("ldr d" + std::to_string(i) + ", [sp], #16");
            } else {
                if (argTypes[i].size_bytes == 4)
                    emit("ldr w" + std::to_string(i) + ", [sp], #16");
                else
                    emit("ldr x" + std::to_string(i) + ", [sp], #16");
            }
        }
    }

    std::string funcName = "_" + expr->name_token.lexeme;
    emit("bl " + funcName);

    current_type = expr->return_type;

    if (!current_type.is_float && current_type.size_bytes < 8) {
        if (current_type.is_signed) {
            if (current_type.size_bytes == 1)
                emit("sxtb x0, w0"); // Sign-extend byte
            else if (current_type.size_bytes == 2)
                emit("sxth x0, w0"); // Sign-extend half
            else if (current_type.size_bytes == 4)
                emit("sxtw x0, w0"); // Sign-extend word
        } else {
            if (current_type.size_bytes == 1)
                emit("uxtb x0, w0"); // Zero-extend byte
            else if (current_type.size_bytes == 2)
                emit("uxth x0, w0"); // Zero-extend half
            else if (current_type.size_bytes == 4)
                emit("uxtw x0, w0"); // Zero-extend word
        }
    }
}
