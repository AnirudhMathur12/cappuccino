//
// Created by Anirudh Mathur on 13/11/25.
//

#ifndef CAPPUCCINO_ABSTRACTSYNTAXTREE_H
#define CAPPUCCINO_ABSTRACTSYNTAXTREE_H

#include "Token.h"
#include <memory>

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
    std::variant<int64_t, float, std::string> value;

    LiteralExpr(const Token &t);
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

struct VariableDeclStmt : Stmt {
    Token type_token;
    std::string name;
    std::optional<ExprPtr> initializer;

    VariableDeclStmt(const Token &t, std::string n, std::optional<ExprPtr> i);
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
    std::optional<StmtPtr> initializer;
    std::optional<ExprPtr> condition;
    std::optional<ExprPtr> increment;

    StmtPtr body;

    ForStmt(std::optional<StmtPtr> i, std::optional<ExprPtr> c,
            std::optional<ExprPtr> inc, StmtPtr b);
    void dump(int indent = 0) const override;
};

struct ReturnStmt : Stmt {
    Token ret_token;
    std::optional<ExprPtr> value;

    ReturnStmt(const Token &t, std::optional<ExprPtr> v);
    void dump(int indent = 0) const override;
};

struct FunctionParameterStmt : Stmt {
    Token type_token;
    std::string name;

    FunctionParameterStmt(const Token &p_type_token, const std::string &n);
    void dump(int indent = 0) const override;
};

struct FunctionDeclStmt : Stmt {
    Token return_type;
    Token name_token;
    std::vector<StmtPtr> params;
    StmtPtr body;
    FunctionDeclStmt(Token rt, Token name, std::vector<StmtPtr> p, StmtPtr b);

    void dump(int indent = 0) const override;
};

struct FunctionCallExpr : Expr {
    Token name_token;
    std::vector<ExprPtr> args;

    FunctionCallExpr(const Token &n, std::vector<ExprPtr> p_args);
    void dump(int indent = 0) const override;
};

struct Program {
    std::vector<StmtPtr> statements;

    void dump() const;
    std::unordered_map<std::string, int> var_offset_lookup;
    int stack_size = 0;
};

#endif // CAPPUCCINO_ABSTRACTSYNTAXTREE_H
