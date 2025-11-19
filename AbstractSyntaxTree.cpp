//
//
// Created by Anirudh Mathur on 13/11/25.
//

#include "AbstractSyntaxTree.h"
#include <string>
#include <type_traits>
#include <variant>

LiteralExpr::LiteralExpr(const Token &t) : token(t) {
    std::visit(
        [&](auto &&src) {
            using T = std::decay_t<decltype(src)>;

            if constexpr (!std::is_same_v<T, std::monostate>) {
                value = src;
            } else {
                // Will never happen, made to suffice exhaustibility
            }
        },
        t.fd);
}

void LiteralExpr::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Literal(";
    std::visit(
        [](auto &&val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<std::string, T>) {
                std::cout << "\"" << val << "\"";
            } else {
                std::cout << val;
            }
        },
        value);
    std::cout << ")" << std::endl;
}

IdentifierExpr::IdentifierExpr(Token t) : token(t), name(t.lexeme) {}

void IdentifierExpr::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Identifier(" << name << ")\n";
}

UnaryExpr::UnaryExpr(Token t, ExprPtr r) : op(t), right(std::move(r)) {}

void UnaryExpr::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Unary(" << op.lexeme << ")\n";
    right->dump(indent + 2);
}

BinaryExpr::BinaryExpr(Token t, ExprPtr l, ExprPtr r)
    : op(t), left(std::move(l)), right(std::move(r)) {}

void BinaryExpr::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Binary(" << op.lexeme << ")\n";
    left->dump(indent + 2);
    right->dump(indent + 2);
}

GroupingExpr::GroupingExpr(ExprPtr e) : expr(std::move(e)) {}

void GroupingExpr::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Grouping\n";
    expr->dump(indent + 2);
}

ExprStmt::ExprStmt(ExprPtr e) : expr(std::move(e)) {}

void ExprStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Expression\n";
    expr->dump(indent + 2);
}

VariableDeclStmt::VariableDeclStmt(const Token &t, std::string n,
                                   std::optional<ExprPtr> i)
    : type_token(t), name(std::move(n)), initializer(std::move(i)) {}

void VariableDeclStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "VariableDecl(type=" << type_token.lexeme
              << ", name=" << name << ")\n";
    if (initializer) {
        std::cout << pad << "  Initializer:\n";
        (*initializer)->dump(indent + 4);
    }
}

BlockStmt::BlockStmt(std::vector<StmtPtr> s) : statements(std::move(s)) {}

void BlockStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Block\n";
    for (const auto &stmt : statements) {
        stmt->dump(indent + 2);
    }
}

IfStmt::IfStmt(ExprPtr c, StmtPtr then_branch,
               std::optional<StmtPtr> else_branch)
    : condition(std::move(c)), then_branch(std::move(then_branch)),
      else_branch(std::move(else_branch)) {}

void IfStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "If\n";
    std::cout << pad << "  Condition:\n";
    condition->dump(indent + 4);
    std::cout << pad << "  Then:\n";
    then_branch->dump(indent + 4);
    if (else_branch) {
        std::cout << pad << "  Else:\n";
        (*else_branch)->dump(indent + 4);
    }
}

WhileStmt::WhileStmt(ExprPtr c, StmtPtr b)
    : condition(std::move(c)), body(std::move(b)) {}

void WhileStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "While\n";
    std::cout << pad << "  Condition:\n";
    condition->dump(indent + 4);
    std::cout << pad << "  Body:\n";
    body->dump(indent + 4);
}

ForStmt::ForStmt(std::optional<StmtPtr> i, std::optional<ExprPtr> c,
                 std::optional<ExprPtr> inc, StmtPtr b)
    : initializer(std::move(i)), condition(std::move(c)),
      increment(std::move(inc)), body(std::move(b)) {}

void ForStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "For\n";
    std::cout << pad << "  Initializer:\n";
    initializer.value()->dump(indent + 4);
    std::cout << pad << "  Condition:\n";
    condition.value()->dump(indent + 4);
    std::cout << pad << "  Increment:\n";
    increment.value()->dump(indent + 4);
    std::cout << pad << "  Body:\n";
    body->dump(indent + 4);
}

ReturnStmt::ReturnStmt(const Token &t, std::optional<ExprPtr> v)
    : ret_token(t), value(std::move(v)) {}

void ReturnStmt::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Return\n";
    std::cout << pad << "  Value:\n";
    if (value) {
        (*value)->dump(indent + 4);
    } else {
        std::cout << pad << "    None\n";
    }
}

FunctionParameter::FunctionParameter(const Token &p_type_token,
                                     const std::string &n)
    : type_token(p_type_token), name(n) {}

FunctionDecl::FunctionDecl(const Token &rt, const Token &name,
                           std::vector<FunctionParameter> p,
                           std::unique_ptr<BlockStmt> b)
    : return_type(std::move(rt)), name_token(std::move(name)),
      body(std::move(b)) {}

void FunctionDecl::dump(int indent) const {
    std::string pad(indent, ' ');
    std::cout << pad << "Function " << name_token.lexeme << " returns "
              << return_type.lexeme << "\n";
    std::cout << pad << "  Params:\n";
    for (auto &pr : params) {
        std::cout << pad << "    " << pr.type_token.lexeme << " " << pr.name
                  << "\n";
    }
    std::cout << pad << "  Body:\n";
    if (body)
        body->dump(indent + 4);
}

void Program::dump() const {
    std::cout << "Program\n";
    for (auto const &stmt : statements) {
        stmt->dump(2);
    }
}
