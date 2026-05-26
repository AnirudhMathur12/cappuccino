#include "CappIR.h"

#include "AbstractSyntaxTree.h"
#include "Errors.h"
#include "Token.h"

#include <cassert>
#include <sys/resource.h>
#include <type_traits>
#include <variant>

VirtualReg IRFunction::allocate_vreg() {
    return VirtualReg(next_vreg_id++);
}

void IRGenerator::emit(OPCode opc, std::optional<VirtualReg> destination,
                       std::vector<Operand> sources) {
    current_block->instruction_list.push_back(Instruction(opc, destination, std::move(sources)));
}

void IRGenerator::visitLiteralExpr(const LiteralExpr* expr) {
    assert(!std::holds_alternative<std::monostate>(expr->token.fd) &&
           "Literal token holds monostate.");

    VirtualReg vr = current_function->allocate_vreg();

    std::visit(
        [this, vr](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (!std::is_same_v<T, std::monostate>) {
                this->emit(OPCode::LOAD_CONST, vr, {arg});
            }
        },
        expr->token.fd);

    current_result = vr;
}

void IRGenerator::visitIdentifierExpr(const IdentifierExpr* expr) {
    VirtualReg vr = current_function->allocate_vreg();
    emit(OPCode::LOAD_LOCAL, vr, {static_cast<uint64_t>(expr->offset)});
    current_result = vr;
}

void IRGenerator::visitBinaryExpr(const BinaryExpr* expr) {
    expr->left->accept(*this);
    VirtualReg left = current_result.value();

    expr->right->accept(*this);
    VirtualReg right = current_result.value();

    OPCode operand;

    switch (expr->op.type) {
    case TokenType::OPERATOR_PLUS:
        operand = OPCode::ADD;
        break;
    case TokenType::OPERATOR_MINUS:
        operand = OPCode::SUB;
        break;
    case TokenType::OPERATOR_ASTERISK:
        operand = OPCode::MUL;
        break;
    case TokenType::OPERATOR_FORWARD_SLASH:
        operand = OPCode::SDIV;
        break;
    default:
        throw SemanticError(expr->op, "Unsupported binary operation");
    }

    VirtualReg result = current_function->allocate_vreg();
    emit(operand, result, {left, right});

    current_result = result;
}
