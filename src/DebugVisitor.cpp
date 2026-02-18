#include "DebugVisitor.h"
#include "AbstractSyntaxTree.h"
#include "Type.h"
#include <iostream>
#include <variant>

std::string DebugVisitor::pad() const { return std::string(indent_level, ' '); }

// Expressions

void DebugVisitor::visitLiteralExpr(const LiteralExpr *expr) {
    std::cout << pad() << "Literal(";
    std::visit(
        [](auto &&val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<std::string, T>) {
                std::cout << "\"" << val << "\"";
            } else if constexpr (std::is_same_v<std::monostate, T>) {
                std::cout << "No Value";
            } else {
                std::cout << val;
            }
        },
        expr->value);
    std::cout << ")" << std::endl;
}

void DebugVisitor::visitIdentifierExpr(const IdentifierExpr *expr) {
    std::cout << pad() << "Identifier(" << expr->name << " [offset: " << expr->offset << ", type: " << expr->type.name
              << ", kind: " << kind_to_string(expr->type.kind) << "])\n";
}

void DebugVisitor::visitUnaryExpr(const UnaryExpr *expr) {
    std::cout << pad() << "Unary(" << expr->op.lexeme << ")\n";
    indent_level += 2;
    expr->right->accept(*this);
    indent_level -= 2;
}

void DebugVisitor::visitBinaryExpr(const BinaryExpr *expr) {
    std::cout << pad() << "Binary(" << expr->op.lexeme << ")\n";
    indent_level += 2;
    expr->left->accept(*this);
    expr->right->accept(*this);
    indent_level -= 2;
}

void DebugVisitor::visitGroupingExpr(const GroupingExpr *expr) {
    std::cout << pad() << "Grouping\n";
    indent_level += 2;
    expr->expr->accept(*this);
    indent_level -= 2;
}

void DebugVisitor::visitFunctionCallExpr(const FunctionCallExpr *expr) {
    std::cout << pad() << "Function Call:" << std::endl;
    std::cout << pad() << "\tFunction Name: " << expr->name_token.lexeme << std::endl;
    std::cout << pad() << "\tReturn Type:" << expr->return_type.name << std::endl;
    std::cout << pad() << "\tArguments" << std::endl;

    indent_level += 8;
    for (auto &e : expr->args) {
        e->accept(*this);
    }
    indent_level -= 8;
}

// Statements

void DebugVisitor::visitExprStmt(const ExprStmt *stmt) {
    std::cout << pad() << "Expression\n";
    indent_level += 2;
    stmt->expr->accept(*this);
    indent_level -= 2;
}

void DebugVisitor::visitVariableDeclStmt(const VariableDeclStmt *stmt) {
    std::cout << pad() << "VariableDecl(type=" << stmt->type_token.lexeme << ", name=" << stmt->name << ", offset=" << stmt->offset
              << ", kind=" << kind_to_string(stmt->type.kind) << ")\n";

    if (stmt->initializer) {
        std::cout << pad() << "  Initializer:\n";
        indent_level += 4;
        (*stmt->initializer)->accept(*this);
        indent_level -= 4;
    }
}

void DebugVisitor::visitBlockStmt(const BlockStmt *stmt) {
    std::cout << pad() << "Block\n";
    indent_level += 2;
    for (const auto &s : stmt->statements) {
        s->accept(*this);
    }
    indent_level -= 2;
}

void DebugVisitor::visitIfStmt(const IfStmt *stmt) {
    std::cout << pad() << "If\n";

    std::cout << pad() << "  Condition:\n";
    indent_level += 4;
    stmt->condition->accept(*this);
    indent_level -= 4;

    std::cout << pad() << "  Then:\n";
    indent_level += 4;
    stmt->then_branch->accept(*this);
    indent_level -= 4;

    if (stmt->else_branch) {
        std::cout << pad() << "  Else:\n";
        indent_level += 4;
        (*stmt->else_branch)->accept(*this);
        indent_level -= 4;
    }
}

void DebugVisitor::visitWhileStmt(const WhileStmt *stmt) {
    std::cout << pad() << "While\n";
    std::cout << pad() << "  Condition:\n";
    indent_level += 4;
    stmt->condition->accept(*this);
    indent_level -= 4;

    std::cout << pad() << "  Body:\n";
    indent_level += 4;
    stmt->body->accept(*this);
    indent_level -= 4;
}

void DebugVisitor::visitForStmt(const ForStmt *stmt) {
    std::cout << pad() << "For\n";

    std::cout << pad() << "  Initializer:\n";
    indent_level += 4;
    if (stmt->initializer) (*stmt->initializer)->accept(*this);
    indent_level -= 4;

    std::cout << pad() << "  Condition:\n";
    indent_level += 4;
    if (stmt->condition) (*stmt->condition)->accept(*this);
    indent_level -= 4;

    std::cout << pad() << "  Increment:\n";
    indent_level += 4;
    if (stmt->increment) (*stmt->increment)->accept(*this);
    indent_level -= 4;

    std::cout << pad() << "  Body:\n";
    indent_level += 4;
    stmt->body->accept(*this);
    indent_level -= 4;
}

void DebugVisitor::visitReturnStmt(const ReturnStmt *stmt) {
    std::cout << pad() << "Return\n";
    std::cout << pad() << "  Value:\n";
    if (stmt->value) {
        indent_level += 4;
        (*stmt->value)->accept(*this);
        indent_level -= 4;
    } else {
        std::cout << pad() << "    None\n";
    }
}

void DebugVisitor::visitFunctionParameterStmt(const FunctionParameterStmt *stmt) {
    std::cout << pad() << "Type: " << stmt->type_token.lexeme << " Name: " << stmt->name << std::endl;
}

void DebugVisitor::visitFunctionDeclStmt(const FunctionDeclStmt *stmt) {
    std::cout << pad() << "Function " << stmt->name_token.lexeme << " returns " << stmt->return_type.name << "\n";
    std::cout << pad() << "  Params:\n";

    indent_level += 4;
    for (auto &pr : stmt->params) {
        pr->accept(*this);
    }
    indent_level -= 4;

    std::cout << pad() << "  Body:\n";
    if (stmt->body) {
        indent_level += 4;
        stmt->body->accept(*this);
        indent_level -= 4;
    }
}
