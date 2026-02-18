//
//
// Created by Anirudh Mathur on 13/11/25.
//

#include "AbstractSyntaxTree.h"
#include "Type.h"
#include "Visitor.h"
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

// Constructors

IdentifierExpr::IdentifierExpr(Token t, int off, Type type) : token(t), name(t.lexeme), offset(off), type(type) {} // Initialize type

UnaryExpr::UnaryExpr(Token t, ExprPtr r) : op(t), right(std::move(r)) {}

BinaryExpr::BinaryExpr(Token t, ExprPtr l, ExprPtr r) : op(t), left(std::move(l)), right(std::move(r)) {}

GroupingExpr::GroupingExpr(ExprPtr e) : expr(std::move(e)) {}

ExprStmt::ExprStmt(ExprPtr e) : expr(std::move(e)) {}

VariableDeclStmt::VariableDeclStmt(const Token &t, std::string n, std::optional<ExprPtr> i, int off, Type type)
    : type_token(t), name(std::move(n)), initializer(std::move(i)), offset(off), type(type) {}

BlockStmt::BlockStmt(std::vector<StmtPtr> s) : statements(std::move(s)) {}

IfStmt::IfStmt(ExprPtr c, StmtPtr then_branch, std::optional<StmtPtr> else_branch)
    : condition(std::move(c)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

WhileStmt::WhileStmt(ExprPtr c, StmtPtr b) : condition(std::move(c)), body(std::move(b)) {}

ForStmt::ForStmt(std::optional<StmtPtr> i, std::optional<ExprPtr> c, std::optional<ExprPtr> inc, StmtPtr b)
    : initializer(std::move(i)), condition(std::move(c)), increment(std::move(inc)), body(std::move(b)) {}

ReturnStmt::ReturnStmt(const Token &t, std::optional<ExprPtr> v) : ret_token(t), value(std::move(v)) {}

FunctionParameterStmt::FunctionParameterStmt(const Token &p_type_token, const std::string &n, int off)
    : type_token(p_type_token), name(n), offset(off) {}

FunctionDeclStmt::FunctionDeclStmt(Type rt, Token name, std::vector<StmtPtr> p, StmtPtr b, int stack)
    : return_type(std::move(rt)), name_token(std::move(name)), params(std::move(p)), body(std::move(b)), stack_size(stack) {}

// Visitor

FunctionCallExpr::FunctionCallExpr(const Token &n, std::vector<ExprPtr> p_args, Type return_type, std::vector<Type> param_types)
    : name_token(n), args(std::move(p_args)), return_type(return_type), param_types(std::move(param_types)) {}

void LiteralExpr::accept(Visitor &visitor) const { visitor.visitLiteralExpr(this); }

void IdentifierExpr::accept(Visitor &visitor) const { visitor.visitIdentifierExpr(this); }

void UnaryExpr::accept(Visitor &visitor) const { visitor.visitUnaryExpr(this); }

void BinaryExpr::accept(Visitor &visitor) const { visitor.visitBinaryExpr(this); }

void GroupingExpr::accept(Visitor &visitor) const { visitor.visitGroupingExpr(this); }

void FunctionCallExpr::accept(Visitor &visitor) const { visitor.visitFunctionCallExpr(this); }

void ExprStmt::accept(Visitor &visitor) const { visitor.visitExprStmt(this); }

void VariableDeclStmt::accept(Visitor &visitor) const { visitor.visitVariableDeclStmt(this); }

void BlockStmt::accept(Visitor &visitor) const { visitor.visitBlockStmt(this); }

void IfStmt::accept(Visitor &visitor) const { visitor.visitIfStmt(this); }

void WhileStmt::accept(Visitor &visitor) const { visitor.visitWhileStmt(this); }

void ForStmt::accept(Visitor &visitor) const { visitor.visitForStmt(this); }

void ReturnStmt::accept(Visitor &visitor) const { visitor.visitReturnStmt(this); }

void FunctionParameterStmt::accept(Visitor &visitor) const { visitor.visitFunctionParameterStmt(this); }

void FunctionDeclStmt::accept(Visitor &visitor) const { visitor.visitFunctionDeclStmt(this); }
