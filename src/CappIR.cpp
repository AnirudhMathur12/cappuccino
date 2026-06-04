#include "CappIR.h"

#include "AbstractSyntaxTree.h"
#include "Errors.h"
#include "Token.h"

#include <cassert>
#include <memory>
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

Block* IRGenerator::createBlock(const std::string& prefix) {
    std::unique_ptr<Block> blck_ptr = std::make_unique<Block>();
    blck_ptr->identifier = prefix + "_" + std::to_string(block_count++);

    Block* raw = blck_ptr.get();

    current_function->blocks.push_back(std::move(blck_ptr));

    return raw;
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

void IRGenerator::visitUnaryExpr(const UnaryExpr* expr) {
    expr->right->accept(*this);
    VirtualReg right = current_result.value();

    OPCode op;

    switch (expr->op.type) {
    case TokenType::EXCLAMATION:
        op = OPCode::NOT;
        break;
    case TokenType::OPERATOR_MINUS:
        op = OPCode::NEG;
        break;
    case TokenType::OPERATOR_AMPERSAND:
    case TokenType::OPERATOR_ASTERISK:
    default:
        throw SemanticError(expr->op, "Unsupported unary operation");
    }

    VirtualReg result = current_function->allocate_vreg();
    emit(op, result, {right});

    current_result = result;
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

void IRGenerator::visitIfStmt(const IfStmt* stmt) {
    stmt->condition->accept(*this);

    assert(current_result.has_value() && "If statement condition result does hold value");

    VirtualReg cond = current_result.value();

    Block *then_block, *else_block, *merge_block;
    then_block = createBlock("if_then");
    else_block = createBlock("if_else");
    merge_block = createBlock("if_merge");

    current_block->successors.push_back(then_block);
    current_block->successors.push_back(else_block);

    then_block->predecessors.push_back(current_block);
    else_block->predecessors.push_back(current_block);

    emit(OPCode::JMP_IF_FALSE, std::nullopt, {cond, else_block->identifier});
    emit(OPCode::JMP, std::nullopt, {then_block->identifier});

    current_block = then_block;
    stmt->then_branch->accept(*this);
    emit(OPCode::JMP, std::nullopt, {merge_block->identifier});
    current_block->successors.push_back(merge_block);

    current_block = else_block;
    if (stmt->else_branch.has_value())
        stmt->else_branch->get()->accept(*this);
    emit(OPCode::JMP, std::nullopt, {merge_block->identifier});
    current_block->successors.push_back(merge_block);

    current_block = merge_block;
}
