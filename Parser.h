#ifndef CAPPUCCINO_PARSER_H
#define CAPPUCCINO_PARSER_H

#include "Token.h"
#include <stdexcept>
#include "AbstractSyntaxTree.h"

class ParseError : public std::runtime_error {
    public:
    using std::runtime_error::runtime_error;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);

    Program parse();
private:
    const std::vector<Token>& tokens;
    size_t pos = 0;

    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();

    bool isAtEnd() const;
    bool check(TokenType t) const;
    bool match(TokenType t) ;
    void consume(TokenType t, const char* msg);

    ExprPtr parseExpression();
    ExprPtr parsePrimary();
    ExprPtr parseUnary();
    ExprPtr parseFactor();
    ExprPtr parseComparison();
    ExprPtr parseEquality();

    ExprPtr parseAssignment();

    ExprPtr parseTerm();
};

#endif
