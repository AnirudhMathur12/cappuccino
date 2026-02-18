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

// Dispatch Methods (Visitor Entry Points)

void CodeGen::genStmt(const Stmt *stmt) {
    if (stmt) stmt->accept(*this);
}

void CodeGen::genExpr(const Expr *expr) {
    if (expr) expr->accept(*this);
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

    out << STDLIB_ASM;
}

// Statement Visitors

void CodeGen::visitBlockStmt(const BlockStmt *stmt) {
    for (const auto &s : stmt->statements) {
        genStmt(s.get());
    }
}

void CodeGen::visitReturnStmt(const ReturnStmt *stmt) {
    if (stmt->value) {
        genExpr(stmt->value.value().get()); // Evaluate result into x0/d0
    } else {
        emit("mov x0, #0");
    }

    // Stack Cleanup
    int stackSize = current_func_stack_size;
    if (stackSize % 16 != 0) stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
}

void CodeGen::visitVariableDeclStmt(const VariableDeclStmt *stmt) {
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
        // Handle Int conversions (Simple cast or Float->Int)
        else {
            if (current_type.is_float) {
                emit("fcvtzs x0, d0"); // Float -> Int
            }
        }

        // Store result
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

void CodeGen::visitExprStmt(const ExprStmt *stmt) { genExpr(stmt->expr.get()); }

void CodeGen::visitIfStmt(const IfStmt *stmt) {
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

void CodeGen::visitWhileStmt(const WhileStmt *stmt) {
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

void CodeGen::visitForStmt(const ForStmt *stmt) {
    std::string labelStart = nextLabel("L_for_start");
    std::string labelEnd = nextLabel("L_for_end");

    if (stmt->initializer) {
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

void CodeGen::visitFunctionDeclStmt(const FunctionDeclStmt *stmt) {
    std::string name = "_" + stmt->name_token.lexeme;

    // Save previous function state
    int saved_stack_size = current_func_stack_size;
    current_func_stack_size = stmt->stack_size;

    emitLabel(name);

    // Prologue
    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");

    int stackSize = stmt->stack_size;
    if (stackSize % 16 != 0) stackSize += (16 - (stackSize % 16));

    if (stackSize > 0) {
        emit("sub sp, sp, #" + std::to_string(stackSize));
    }

    // Process parameters
    // We use current_param_index to track which register (w0/x0/s0/d0 etc) to use
    current_param_index = 0;
    for (const auto &param : stmt->params) {
        genStmt(param.get()); // Dispatches to visitFunctionParameterStmt
        current_param_index++;
    }

    // Generate function body
    genStmt(stmt->body.get());

    // Epilogue (implicit return 0 if no return stmt reached)
    emit("mov x0, #0");
    if (stackSize > 0) {
        emit("add sp, sp, #" + std::to_string(stackSize));
    }
    emit("ldp x29, x30, [sp], #16");
    emit("ret");

    // Restore state
    current_func_stack_size = saved_stack_size;
}

void CodeGen::visitFunctionParameterStmt(const FunctionParameterStmt *stmt) {
    // This method is called via genStmt loop in visitFunctionDeclStmt
    int offset = stmt->offset;
    Type param_type = TypeSystem::from_string(stmt->type_token.lexeme);

    // Limit to 8 registers for arguments
    if (current_param_index > 7) return;

    if (param_type.is_float) {
        std::string reg = (param_type.size_bytes == 4 ? "s" : "d") + std::to_string(current_param_index);
        emit("stur " + reg + ", [x29, #-" + std::to_string(offset) + "]");
    } else {
        std::string reg = (param_type.size_bytes < 8 ? "w" : "x") + std::to_string(current_param_index);

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

// Expression Visitors

void CodeGen::visitLiteralExpr(const LiteralExpr *expr) {
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

void CodeGen::visitIdentifierExpr(const IdentifierExpr *expr) {
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

void CodeGen::visitUnaryExpr(const UnaryExpr *expr) {
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
    case TokenType::OPERATOR_ASTERISK: {
        if (current_type.kind != TypeKind::POINTER) {
            throw std::runtime_error("Semantic Error: Cannot dereference a non-pointer type.");
        }
        current_type = *current_type.baseType;

        if (current_type.is_float) {
            if (current_type.size_bytes == 4) {
                emit("ldr s0, [x0]");
            } else {
                emit("ldr d0, [x0]");
            }
        } else {
            if (current_type.size_bytes == 1) {
                if (current_type.is_signed)
                    emit("ldrsb x0, [x0]");
                else
                    emit("ldrb w0, [x0]");
            } else if (current_type.size_bytes == 2) {
                if (current_type.is_signed)
                    emit("ldrsh x0, [x0]");
                else
                    emit("ldrh w0, [x0]");
            } else if (current_type.size_bytes == 4) {
                emit("ldr w0, [x0]");
            } else {
                emit("ldr x0, [x0]");
            }
        }
        break;
    }
    case TokenType::OPERATOR_AMPERSAND: {
        auto *ident = dynamic_cast<IdentifierExpr *>(expr->right.get());
        if (!ident) {
            throw std::runtime_error("Semantic Error: '&' operator requires a variable identifier.");
        }

        emit("sub x0, x29, #" + std::to_string(ident->offset));

        current_type = TypeSystem::createPointer(ident->type);
        break;
    }
    default: throw std::runtime_error("Unknown unary operator");
    }
}

void CodeGen::visitGroupingExpr(const GroupingExpr *expr) { genExpr(expr->expr.get()); }

void CodeGen::visitBinaryExpr(const BinaryExpr *expr) {
    if (expr->op.type == TokenType::OPERATOR_ASSIGNMENT) {
        visitAssignment(expr);
        return;
    }

    genExpr(expr->left.get());
    Type leftType = current_type;

    // Push Left
    if (leftType.is_float)
        emit("str d0, [sp, #-16]!");
    else
        emit("str x0, [sp, #-16]!");

    genExpr(expr->right.get());
    Type rightType = current_type;

    // Move Right to Reg 1
    if (rightType.is_float)
        emit("fmov d1, d0");
    else
        emit("mov x1, x0");

    // Pop Left to Reg 0
    if (leftType.is_float)
        emit("ldr d0, [sp], #16");
    else
        emit("ldr x0, [sp], #16");

    // Implicit Casting for math
    if (!leftType.is_float && rightType.is_float) {
        emit("scvtf d0, x0");
        leftType = TypeSystem::Float64;
    } else if (leftType.is_float && !rightType.is_float) {
        emit("scvtf d1, x1");
        rightType = TypeSystem::Float64;
    }

    if (leftType.is_float && rightType.is_float) {
        // Floating Point Math
        if (rightType.size_bytes == 4 && leftType.size_bytes == 4) {
            current_type = TypeSystem::Float32;
            switch (expr->op.type) {
            case TokenType::OPERATOR_PLUS: emit("fadd s0, s0, s1"); break;
            case TokenType::OPERATOR_MINUS: emit("fsub s0, s0, s1"); break;
            case TokenType::OPERATOR_ASTERISK: emit("fmul s0, s0, s1"); break;
            case TokenType::OPERATOR_FORWARD_SLASH: emit("fdiv s0, s0, s1"); break;
            case TokenType::OPERATOR_LESS:
                emit("fcmp s0, s1");
                emit("cset x0, mi");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_LESS_EQUALS:
                emit("fcmp s0, s1");
                emit("cset x0, le");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER:
                emit("fcmp s0, s1");
                emit("cset x0, gt");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER_EQUALS:
                emit("fcmp s0, s1");
                emit("cset x0, ge");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_EQUALITY:
                emit("fcmp s0, s1");
                emit("cset x0, eq");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::EXCL_EQUAL:
                emit("fcmp s0, s1");
                emit("cset x0, ne");
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
                emit("cset x0, mi");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_LESS_EQUALS:
                emit("fcmp d0, d1");
                emit("cset x0, le");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER:
                emit("fcmp d0, d1");
                emit("cset x0, gt");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_GREATER_EQUALS:
                emit("fcmp d0, d1");
                emit("cset x0, ge");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::OPERATOR_EQUALITY:
                emit("fcmp d0, d1");
                emit("cset x0, eq");
                current_type = TypeSystem::Int64;
                break;
            case TokenType::EXCL_EQUAL:
                emit("fcmp d0, d1");
                emit("cset x0, ne");
                current_type = TypeSystem::Int64;
                break;
            default: break;
            }
        }
    } else {
        // Integer Math
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
        case TokenType::OPERATOR_EQUALITY:
            emit("cmp x0, x1");
            emit("cset x0, eq");
            break;
        case TokenType::EXCL_EQUAL:
            emit("cmp x0, x1");
            emit("cset x0, ne");
            break;

        case TokenType::OPERATOR_LESS:
            if (leftType.is_signed && rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, lt");
            } else if (!leftType.is_signed && !rightType.is_signed) {
                emit("cmp x0, x1");
                emit("cset x0, lo");
            } else { /* Mixed sign comparison logic omitted for brevity, use standard signed if unsure */
                emit("cmp x0, x1");
                emit("cset x0, lt");
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
                emit("cmp x0, x1");
                emit("cset x0, le");
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
                emit("cmp x0, x1");
                emit("cset x0, gt");
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
                emit("cmp x0, x1");
                emit("cset x0, ge");
            }
            break;
        default: break;
        }
    }
}

void CodeGen::visitAssignment(const BinaryExpr *expr) {
    // Case 1: Standard Variable Assignment (e.g., x = 5)
    if (auto *ident = dynamic_cast<const IdentifierExpr *>(expr->left.get())) {
        genExpr(expr->right.get());

        Type varType = ident->type;

        // Implicit Casting (RHS -> Variable Type)
        if (varType.is_float) {
            if (!current_type.is_float) {
                emit("scvtf d0, x0"); // Int -> Float
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
        } else {
            // Float -> Int
            if (current_type.is_float) {
                emit("fcvtzs x0, d0");
            }
        }

        // Store to Stack (Frame Pointer - Offset)
        if (varType.is_float) {
            if (varType.size_bytes == 4) {
                emit("stur s0, [x29, #-" + std::to_string(ident->offset) + "]");
            } else {
                emit("stur d0, [x29, #-" + std::to_string(ident->offset) + "]");
            }
        } else {
            if (varType.size_bytes == 1) {
                emit("sturb w0, [x29, #-" + std::to_string(ident->offset) + "]");
            } else if (varType.size_bytes == 2) {
                emit("sturh w0, [x29, #-" + std::to_string(ident->offset) + "]");
            } else if (varType.size_bytes == 4) {
                emit("stur w0, [x29, #-" + std::to_string(ident->offset) + "]");
            } else {
                emit("stur x0, [x29, #-" + std::to_string(ident->offset) + "]");
            }
        }

        // Update current_type to allow chained assignments
        current_type = varType;
    }

    //  Pointer Dereference Assignment (e.g., *ptr = 5)
    else if (auto *unary = dynamic_cast<const UnaryExpr *>(expr->left.get())) {
        if (unary->op.type != TokenType::OPERATOR_ASTERISK) {
            throw std::runtime_error("Invalid assignment target. Expected variable or pointer dereference.");
        }

        // Evaluate the Pointer (LHS) to get the target memory address
        genExpr(unary->right.get());

        if (current_type.kind != TypeKind::POINTER) {
            throw std::runtime_error("Semantic Error: Assigning to a non-pointer dereference.");
        }

        Type targetType = *current_type.baseType; // The type the pointer points to

        // Push the Address to the stack to preserve it while we evaluate the RHS
        emit("str x0, [sp, #-16]!");

        // Evaluate the Value (RHS)
        genExpr(expr->right.get());
        // x0 (or d0) now holds the value to assign

        // Perform Implicit Casting (RHS -> TargetType)
        if (targetType.is_float) {
            if (!current_type.is_float) {
                emit("scvtf d0, x0"); // Int -> Float
                current_type = (current_type.size_bytes == 4) ? TypeSystem::Float32 : TypeSystem::Float64;
            }

            if (targetType.size_bytes == 4 && current_type.size_bytes == 8) {
                emit("fcvt s0, d0");
            } else if (targetType.size_bytes == 8 && current_type.size_bytes == 4) {
                emit("fcvt d0, s0");
            }
        } else {
            if (current_type.is_float) {
                emit("fcvtzs x0, d0"); // Float -> Int
            }
        }

        // Pop the Memory Address into x1
        emit("ldr x1, [sp], #16");

        // Store the Value (x0/d0) into the Address (x1)
        if (targetType.is_float) {
            if (targetType.size_bytes == 4) {
                emit("stur s0, [x1]");
            } else {
                emit("stur d0, [x1]");
            }
        } else {
            if (targetType.size_bytes == 1) {
                emit("sturb w0, [x1]");
            } else if (targetType.size_bytes == 2) {
                emit("sturh w0, [x1]");
            } else if (targetType.size_bytes == 4) {
                emit("stur w0, [x1]");
            } else {
                emit("stur x0, [x1]");
            }
        }

        current_type = targetType;

    } else {
        throw std::runtime_error("Invalid assignment target.");
    }
}

void CodeGen::visitFunctionCallExpr(const FunctionCallExpr *expr) {
    // Evaluate arguments and push them (preserving type info)
    std::vector<Type> argTypes;
    for (int i = 0; i < expr->args.size(); i++) {
        const auto &arg = expr->args[i];
        genExpr(arg.get());

        if (i < expr->param_types.size()) {
            Type expected = expr->param_types[i];

            if (expected.is_float) {
                if (!current_type.is_float) {
                    emit("scvtf d0, x0");
                    current_type = TypeSystem::Float64;
                }

                if (expected.size_bytes == 8 && current_type.size_bytes == 4) {
                    emit("fcvt d0, s0");
                    current_type = TypeSystem::Float64; // Treat as double on stack
                } else if (expected.size_bytes == 4 && current_type.size_bytes == 8) {
                    emit("fcvt s0, d0");
                    current_type = TypeSystem::Float32;
                }
            } else if (!expected.is_float && current_type.is_float) {
                emit("fcvtzs x0, d0");
                current_type = TypeSystem::Int64;
            }
        }

        argTypes.push_back(current_type);

        if (current_type.is_float) {
            emit("str d0, [sp, #-16]!");
        } else {
            emit("str x0, [sp, #-16]!");
        }
    }

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
                emit("sxtb x0, w0");
            else if (current_type.size_bytes == 2)
                emit("sxth x0, w0");
            else if (current_type.size_bytes == 4)
                emit("sxtw x0, w0");
        } else {
            if (current_type.size_bytes == 1)
                emit("uxtb x0, w0");
            else if (current_type.size_bytes == 2)
                emit("uxth x0, w0");
            else if (current_type.size_bytes == 4)
                emit("uxtw x0, w0");
        }
    }
}
