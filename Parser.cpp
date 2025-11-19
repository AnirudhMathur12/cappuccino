#include "Parser.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <memory>
#include <vector>

Parser::Parser(const std::vector<Token> &p_tokens) : tokens(p_tokens) {}

bool Parser::isAtEnd() const { return peek().type == TOKEN_EOF; }

const Token &Parser::peek() const { return tokens[pos]; }

const Token &Parser::previous() const { return tokens[pos - 1]; }

const Token &Parser::advance() {
    if (!isAtEnd())
        pos++;
    return previous();
}

bool Parser::check(TokenType t) const {
    if (isAtEnd())
        return false;
    return peek().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) {
        advance();
        return true;
    }

    return false;
}

void Parser::consume(TokenType t, const char *msg) {
    if (check(t)) {
        advance();
        return;
    }

    throw ParseError(msg);
}

ExprPtr Parser::parsePrimary() {
    if (match(LITERAL_FLOAT) || match(LITERAL_INTEGER) ||
        match(LITERAL_STRING)) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match(IDENTIFIER)) {
        return std::make_unique<IdentifierExpr>(previous());
    }

    if (match(LEFT_PAREN)) {
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
        expr =
            std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expr = parseFactor();

    while (match(OPERATOR_PLUS) || match(OPERATOR_MINUS)) {
        Token op = previous();
        ExprPtr right = parseFactor();
        expr =
            std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseTerm();

    while (match(OPERATOR_LESS) || match(OPERATOR_GREATER) ||
           match(OPERATOR_LESS_EQUALS) || match(OPERATOR_GREATER_EQUALS)) {
        Token op = previous();
        ExprPtr right = parseTerm();
        expr =
            std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();

    while (match(EXCL_EQUAL) || match(OPERATOR_EQUALITY)) {
        Token op = previous();
        ExprPtr right = parseComparison();
        expr =
            std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseAssignment() {
    ExprPtr left = parseEquality();

    if (match(OPERATOR_ASSIGNMENT)) {
        Token equals = previous();
        ExprPtr right = parseAssignment();

        if (auto *ident = dynamic_cast<IdentifierExpr *>(left.get())) {
            return std::make_unique<BinaryExpr>(equals, std::move(left),
                                                std::move(right));
        }

        throw ParseError("Invalid assignment target.");
    }

    return left;
}

StmtPtr Parser::parseStatement() {
    if (match(LEFT_CURLY)) {
        return parseBlock();
    }

    if (match(KEYWORD_IF)) {
        return parseIf();
    }

    if (match(KEYWORD_WHILE)) {
        return parseWhile();
    }

    if (match(KEYWORD_FOR)) {
        return parseFor();
    }

    if (check(KEYWORD_TYPE_FLOAT) || check(KEYWORD_TYPE_STRING) ||
        check(KEYWORD_TYPE_INT)) {
        return parseVarDecl();
    }

    return parseExpressionStatement();
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    consume(SEMICOLON, "Expected ';' after expression statement.");

    return std::make_unique<ExprStmt>(std::move(expr));
}

std::stack<std::string> var_lookup;
std::unordered_map<std::string, int> data_type_size_lookup = {{"int", 4},
                                                              {"float", 8}};

StmtPtr Parser::parseVarDecl() {
    Token varType = advance();

    consume(IDENTIFIER, "Expected variable name");
    Token varName = previous();

    std::optional<ExprPtr> init;

    if (match(OPERATOR_ASSIGNMENT)) {
        init = parseExpression();
    }

    consume(SEMICOLON, "Expected ';' after initialization.");

    var_offset_lookup[varName.lexeme] =
        (var_lookup.empty() ? 0 : var_offset_lookup[var_lookup.top()]) +
        data_type_size_lookup[varType.lexeme];
    total_size_bytes += data_type_size_lookup[varType.lexeme];
    var_lookup.push(varName.lexeme);
    return std::make_unique<VariableDeclStmt>(varType, varName.lexeme,
                                              std::move(init));
}

StmtPtr Parser::parseBlock() {
    std::vector<StmtPtr> stmts;

    while (!check(RIGHT_CURLY) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    consume(RIGHT_CURLY, "Expected '}' after block");

    return std::make_unique<BlockStmt>(std::move(stmts));
}

StmtPtr Parser::parseIf() {
    consume(LEFT_PAREN, "Expected '(' after If. ");
    ExprPtr condition = parseExpression();
    consume(RIGHT_PAREN, "Exptected ')' after condition");

    StmtPtr thenBranch = parseStatement();
    std::optional<StmtPtr> elseBranch;

    if (match(KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                    std::move(elseBranch));
}

StmtPtr Parser::parseWhile() {
    consume(LEFT_PAREN, "Expected '(' after while");
    ExprPtr condition = parseExpression();
    consume(RIGHT_PAREN, "Expected ')' after condition");

    StmtPtr body = parseStatement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::parseFor() {
    consume(LEFT_PAREN, "Expected '(' after for");

    std::optional<StmtPtr> init;

    if (!check(SEMICOLON)) {
        if (check(KEYWORD_TYPE_INT) || check(KEYWORD_TYPE_FLOAT) ||
            check(KEYWORD_TYPE_STRING)) {
            init = parseVarDecl();
        } else {
            init = parseExpressionStatement();
        }
    } else {
        consume(SEMICOLON, "Expected ';' after for initializer");
    }

    std::optional<ExprPtr> cond;
    if (!check(SEMICOLON))
        cond = parseExpression();
    consume(SEMICOLON, "Expected ';' after for condition.");

    std::optional<ExprPtr> post;
    if (!check(RIGHT_PAREN))
        post = parseExpression();
    consume(RIGHT_PAREN, "Expected ')' after for clause.");

    StmtPtr body = parseStatement();

    return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                     std::move(post), std::move(body));
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

Program Parser::parse() {
    Program prog;

    while (!isAtEnd()) {
        prog.statements.push_back(parseStatement());
    }

    prog.var_offset_lookup = var_offset_lookup;

    return prog;
}
