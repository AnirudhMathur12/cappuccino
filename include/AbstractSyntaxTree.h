#ifndef CAPPUCCINO_ABSTRACTSYNTAXTREE_H
#define CAPPUCCINO_ABSTRACTSYNTAXTREE_H

#include "Token.h"
#include "Type.h"
#include <memory>
#include <optional>
#include <vector>

class Visitor;

struct Expr {
    virtual ~Expr() = default;
    virtual void accept(Visitor &visitor) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct Stmt {
    virtual ~Stmt() = default;
    virtual void accept(Visitor &visitor) const = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct LiteralExpr : Expr {
    Token token;
    FormattedData value;

    LiteralExpr(const Token &t);
    void accept(Visitor &visitor) const override;
};

struct IdentifierExpr : Expr {
    Token token;
    std::string name;
    int offset;
    Type type;

    IdentifierExpr(Token t, int off, Type type);
    void accept(Visitor &visitor) const override;
};

struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;

    UnaryExpr(Token t, ExprPtr r);
    void accept(Visitor &visitor) const override;
};

struct BinaryExpr : Expr {
    Token op;
    ExprPtr left;
    ExprPtr right;

    BinaryExpr(Token t, ExprPtr l, ExprPtr r);
    void accept(Visitor &visitor) const override;
};

struct GroupingExpr : Expr {
    ExprPtr expr;

    GroupingExpr(ExprPtr e);
    void accept(Visitor &visitor) const override;
};

struct FunctionCallExpr : Expr {
    Token name_token;
    std::vector<ExprPtr> args;
    Type return_type;
    std::vector<Type> param_types;

    FunctionCallExpr(const Token &n, std::vector<ExprPtr> p_args, Type return_type, std::vector<Type> param_types);
    void accept(Visitor &visitor) const override;
};

struct ArrayAccessExpr : Expr {
    ExprPtr array;
    ExprPtr idx;
    Token bracket_token;

    ArrayAccessExpr(ExprPtr array, ExprPtr idx, Token bracket);
    void accept(Visitor &visitor) const override;
};

struct ArrayLiteralExpr : Expr {
    std::vector<ExprPtr> elements;

    ArrayLiteralExpr(std::vector<ExprPtr> elements);
    void accept(Visitor &visitor) const override;
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    ExprStmt(ExprPtr e);
    void accept(Visitor &visitor) const override;
};

struct VariableDeclStmt : Stmt {
    Token type_token;
    std::string name;
    std::optional<ExprPtr> initializer;
    int offset;
    Type type;

    VariableDeclStmt(const Token &t, std::string n, std::optional<ExprPtr> i, int off, Type type);
    void accept(Visitor &visitor) const override;
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    BlockStmt(std::vector<StmtPtr> s);
    void accept(Visitor &visitor) const override;
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr then_branch;
    std::optional<StmtPtr> else_branch;

    IfStmt(ExprPtr c, StmtPtr then_branch, std::optional<StmtPtr> else_branch);
    void accept(Visitor &visitor) const override;
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr c, StmtPtr b);
    void accept(Visitor &visitor) const override;
};

struct ForStmt : Stmt {
    std::optional<StmtPtr> initializer;
    std::optional<ExprPtr> condition;
    std::optional<ExprPtr> increment;

    StmtPtr body;

    ForStmt(std::optional<StmtPtr> i, std::optional<ExprPtr> c, std::optional<ExprPtr> inc, StmtPtr b);
    void accept(Visitor &visitor) const override;
};

struct ReturnStmt : Stmt {
    Token ret_token;
    std::optional<ExprPtr> value;

    ReturnStmt(const Token &t, std::optional<ExprPtr> v);
    void accept(Visitor &visitor) const override;
};

struct FunctionParameterStmt : Stmt {
    Token type_token;
    std::string name;
    int offset;

    FunctionParameterStmt(const Token &p_type_token, const std::string &n, int off);
    void accept(Visitor &visitor) const override;
};

struct FunctionDeclStmt : Stmt {
    Type return_type;
    Token name_token;
    std::vector<StmtPtr> params;
    StmtPtr body;
    int stack_size;

    FunctionDeclStmt(Type rt, Token name, std::vector<StmtPtr> p, StmtPtr b, int stack);
    void accept(Visitor &visitor) const override;
};

struct Program {
    std::vector<StmtPtr> statements;
    std::unordered_map<std::string, int> var_offset_lookup;
    std::unordered_map<std::string, std::string> var_type_lookup;
    int stack_size = 0;
};

#endif
