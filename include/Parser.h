#ifndef CAPPUCCINO_PARSER_H
#define CAPPUCCINO_PARSER_H

#include "AbstractSyntaxTree.h"
#include "SymbolTable.h"
#include "Token.h"
#include <stdexcept>

class ParserPanic : public std::runtime_error {
  public:
    ParserPanic() : std::runtime_error("panic") {}
};

class Parser {
  public:
    Parser(const std::vector<Token> &tokens);

    Program parse();

    SymbolTable symbolTable;

  private:
    void error(const Token &tok, const std::string &msg);
    void synchronize();

    const std::vector<Token> &tokens;
    size_t pos = 0;

    const Token &peek() const;
    const Token &peekNext() const;
    const Token &previous() const;

    const Token &advance();

    bool isAtEnd() const;
    bool check(TokenType t) const;
    bool match(TokenType t);
    void consume(TokenType t, const char *msg);

    ExprPtr parseExpression();
    ExprPtr parsePrimary();
    ExprPtr parseUnary();
    ExprPtr parseFactor();
    ExprPtr parseComparison();
    ExprPtr parseEquality();
    ExprPtr parseAssignment();
    ExprPtr parseTerm();

    StmtPtr parseStatement();
    StmtPtr parseExpressionStatement();
    StmtPtr parseVarOrFunctionDecl();
    StmtPtr parseBlock();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseFunction();
    StmtPtr parseReturnStmt();
    StmtPtr parseClassDecl();
};

#endif
