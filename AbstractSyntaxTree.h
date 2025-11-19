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

struct VariableDeclStmt : Stmt {
    Token type_token;
    std::string name;
    std::optional<ExprPtr> initializer;

    VariableDeclStmt(const Token& t, std::string n, std::optional<ExprPtr> i);
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

    ForStmt(std::optional<StmtPtr> i, std::optional<ExprPtr> c, std::optional<ExprPtr> inc, StmtPtr b);
    void dump(int indent = 0) const override;
};

struct ReturnStmt : Stmt {
    Token ret_token;
    std::optional<ExprPtr> value;

    ReturnStmt(const Token& t, std::optional<ExprPtr> v);
    void dump(int indent = 0) const override;
};

struct FunctionParameter {
    Token type_token;
    std::string name;

    FunctionParameter(const Token& p_type_token, const std::string& n);
};

struct FunctionDecl{
    Token return_type;
    Token name_token;
    std::vector<FunctionParameter> params;
    std::unique_ptr<BlockStmt> body;
    FunctionDecl(const Token& rt, const Token& name, std::vector<FunctionParameter> p, std::unique_ptr<BlockStmt> b);

    void dump(int indent = 0) const;
};



struct Program {
    std::vector<StmtPtr> statements;

    void dump() const;
    std::unordered_map<std::string, int> var_offset_lookup;
};

#endif //CAPPUCCINO_ABSTRACTSYNTAXTREE_H
