#include "Parser.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <memory>
#include <vector>

Parser::Parser(const std::vector<Token>& p_tokens) : tokens(p_tokens) {}

bool Parser::isAtEnd() const {
    return peek().type == TOKEN_EOF;
}

const Token& Parser::peek() const{
    return tokens[pos];
}

const Token& Parser::previous() const {
    return tokens[pos-1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) pos++;
    return previous();
}

bool Parser::check(TokenType t) const{
    if(isAtEnd()) return false;
    return peek().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) {
        advance();
        return true;
    }

    return false;
}

void Parser::consume(TokenType t, const char* msg) {
    if(check(t)) {
        advance();
        return;
    }

    throw ParseError(msg);
}

ExprPtr Parser::parsePrimary() {
    if (match(LITERAL_FLOAT) ||
        match(LITERAL_INTEGER) ||
        match(LITERAL_STRING))
    {
        return std::make_unique<LiteralExpr>(previous());
    }

    if(match(IDENTIFIER)) {
        return std::make_unique<IdentifierExpr>(previous());
    }

    if(match(LEFT_PAREN)) {
        ExprPtr expr = parseExpression();
        consume(RIGHT_PAREN, "Expected ')' after expression. ");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }



    throw ParseError("Expected expression");
}

ExprPtr Parser::parseUnary() {
    if (match(OPERATOR_MINUS) || match(EXCLAMATION)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return parsePrimary();
}

ExprPtr Parser::parseFactor() {
    ExprPtr expr = parseUnary();

    while (match(OPERATOR_ASTERISK) || match(OPERATOR_FORWARD_SLASH)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        expr =  std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expr = parseFactor();

    while (match(OPERATOR_PLUS) || match(OPERATOR_MINUS)) {
        Token op = previous();
        ExprPtr right = parseFactor();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseTerm();

    while (
        match(OPERATOR_LESS) ||
        match(OPERATOR_GREATER) ||
        match(OPERATOR_LESS_EQUALS) ||
        match(OPERATOR_GREATER_EQUALS)){
        Token op = previous();
        ExprPtr right = parseTerm();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();

    while (match(EXCL_EQUAL) || match(OPERATOR_EQUALITY)) {
        Token op = previous();
        ExprPtr right = parseComparison();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseAssignment() {
    ExprPtr left = parseEquality();

    if (match(OPERATOR_ASSIGNMENT)) {
        Token equals = previous();
        ExprPtr right = parseAssignment();

        if (auto *ident = dynamic_cast<IdentifierExpr*>(left.get())) {
            return std::make_unique<BinaryExpr>(equals, std::move(left), std::move(right));
        }

        throw ParseError("Invalid assignment target.");
    }

    return left;
}

ExprPtr Parser::parseExpression() {
    return parseAssignment();
}

Program Parser::parse() {
    Program prog;

    while(!isAtEnd()) {
        ExprPtr ptr = parseExpression();
        prog.statements.push_back(
            std::make_unique<ExprStmt>(std::move(ptr))
        );
    }

    return prog;
}
