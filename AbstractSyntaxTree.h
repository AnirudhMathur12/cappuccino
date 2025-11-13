//
// Created by Anirudh Mathur on 13/11/25.
//

#ifndef CAPPUCCINO_ABSTRACTSYNTAXTREE_H
#define CAPPUCCINO_ABSTRACTSYNTAXTREE_H

#include <memory>
#include "Token.h"

struct Expr {
    virtual ~Expr() = default;
    virtual void dump(int indent = 0) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct Stmt {
    virtual ~Stmt() = default;
    virtual void dump(int indent = 0) const = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct LiteralExpr : Expr {
    Token token;
    std::variant<int, float, std::string> value;

    LiteralExpr(const Token& t);
    void dump(int indent = 0) const override;
};

struct IdentifierExpr : Expr {
    Token token;
    std::string name;

    IdentifierExpr(Token t);
    void dump(int indent = 0) const override;
};

struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;

    UnaryExpr(Token t, ExprPtr r);
    void dump(int indent = 0) const override;
};

struct BinaryExpr : Expr {
    Token op;
    ExprPtr left;
    ExprPtr right;

    BinaryExpr(Token t, ExprPtr l, ExprPtr r);
    void dump(int indent = 0) const override;
};

struct GroupingExpr : Expr {
    ExprPtr expr;

    GroupingExpr(ExprPtr e);
    void dump(int indent = 0) const override;
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr e);
    void dump(int indent = 0) const override;
};

struct VariableDecl : Stmt {
    Token type_token;
    std::string name;
    std::optional<ExprPtr> initializer;

    VariableDecl(const Token& t, std::string n, std::optional<ExprPtr> i);
    void dump(int indent = 0) const override;
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    BlockStmt(std::vector<StmtPtr> s);
    void dump(int indent = 0) const override;
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr then_branch;
    std::optional<StmtPtr> else_branch;

    IfStmt(ExprPtr c, StmtPtr then_branch, std::optional<StmtPtr> else_branch);
    void dump(int indent = 0) const override;
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr c, StmtPtr b);
    void dump(int indent = 0) const override;
};

struct ForStmt : Stmt {
    ExprPtr initializer;
    ExprPtr condition;
    ExprPtr increment;

    StmtPtr body;

    ForStmt(ExprPtr i, ExprPtr c, ExprPtr inc, StmtPtr b);
    void dump(int indent = 0) const override;
};

struct ReturnStmt : Stmt {
    Token ret_token;
    std::optional<ExprPtr> value;

    ReturnStmt(const Token& t, std::optional<ExprPtr> v);
    void dump(int indent = 0) const override;
};

struct Program {
    std::vector<StmtPtr> statements;

    void dump() const;
};

#endif //CAPPUCCINO_ABSTRACTSYNTAXTREE_H
