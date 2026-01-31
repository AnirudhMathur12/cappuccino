#ifndef CAPPUCCINO_PARSER_H
#define CAPPUCCINO_PARSER_H

#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <stdexcept>

class ParseError : public std::runtime_error {
  public:
    const Token token;
    ParseError(const Token &tok, const std::string &msg);

  private:
    static std::string format_message(const Token &tok, const std::string msg);
};

class Parser {
  public:
    Parser(const std::vector<Token> &tokens);

    Program parse();

    int stack_size_bytes = 0;
    std::unordered_map<std::string, int> var_offset_lookup;
    std::unordered_map<std::string, std::string> var_type_lookup;

  private:
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
};

#endif
