#include "Parser.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

// Global shadow stacks to track variable states for scoping/shadowing
static std::map<std::string, std::vector<int>> shadow_offset_stack;
static std::map<std::string, std::vector<std::string>> shadow_type_stack;

ParseError::ParseError(const Token &tok, const std::string &msg)
    : std::runtime_error(format_message(tok, msg)), token(tok) {}

std::string ParseError::format_message(const Token &tok,
                                       const std::string msg) {
    std::ostringstream oss;
    oss << "[Line:" << tok.row << ", Column:" << tok.column << "] " << msg
        << std::endl;

    return oss.str();
}

Parser::Parser(const std::vector<Token> &p_tokens) : tokens(p_tokens) {}

bool Parser::isAtEnd() const { return peek().type == TOKEN_EOF; }

const Token &Parser::peek() const { return tokens[pos]; }

const Token &Parser::peekNext() const {
    return isAtEnd() ? tokens[pos] : tokens[pos + 1];
}

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

    throw ParseError(previous(), msg);
}

ExprPtr Parser::parsePrimary() {

    if (match(LITERAL_FLOAT) || match(LITERAL_INTEGER) ||
        match(LITERAL_STRING)) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match(IDENTIFIER)) {
        Token identifierName = previous();
        if (match(LEFT_PAREN)) {

            std::vector<ExprPtr> args;
            if (!check(RIGHT_PAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(COMMA));
            }

            consume(RIGHT_PAREN, "Exptected a ')'");

            return std::make_unique<FunctionCallExpr>(std::move(identifierName),
                                                      std::move(args));
        }
        int off = var_offset_lookup[identifierName.lexeme];
        std::string type = var_type_lookup[identifierName.lexeme];
        return std::make_unique<IdentifierExpr>(identifierName, off, type);
    }

    if (match(LEFT_PAREN)) {
        ExprPtr expr = parseExpression();
        consume(RIGHT_PAREN, "Expected ')' after expression. ");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw ParseError(previous(), "Expected expression");
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

        throw ParseError(previous(), "Invalid assignment target.");
    }

    return left;
}

StmtPtr Parser::parseReturnStmt() {
    Token t = previous();
    if (match(SEMICOLON)) {
        return std::make_unique<ReturnStmt>(t, std::nullopt);
    }

    ExprPtr ex = parseExpression();
    consume(SEMICOLON, "Expected';' after expression");
    return std::make_unique<ReturnStmt>(t, std::move(ex));
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

    if (match(KEYWORD_TYPE_FLOAT) || match(KEYWORD_TYPE_STRING) ||
        match(KEYWORD_TYPE_INT) || match(KEYWORD_TYPE_VOID)) {
        return parseVarOrFunctionDecl();
    }

    if (match(KEYWORD_RETURN)) {
        return parseReturnStmt();
    }

    return parseExpressionStatement();
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    consume(SEMICOLON, "Expected ';' after expression statement.");

    return std::make_unique<ExprStmt>(std::move(expr));
}

std::stack<std::string> var_lookup;
std::unordered_map<std::string, int> data_type_size_lookup = {{"int", 8},
                                                              {"float", 8}};

StmtPtr Parser::parseVarOrFunctionDecl() {
    Token identifierType = previous();
    consume(IDENTIFIER, "Expected identifier name");
    Token identifierName = previous();

    if (match(LEFT_PAREN)) {
        // Save the current stack offset to restore after this function
        int saved_stack_size = stack_size_bytes;

        // Start fresh stack for this function
        stack_size_bytes = 0;

        std::cout << "=== Parsing function: " << identifierName.lexeme
                  << " ===" << std::endl;

        var_type_lookup[identifierName.lexeme] = identifierType.lexeme;

        // Track parameters so we can remove them after the function body is
        // parsed
        std::vector<std::string> params_declared;

        std::vector<StmtPtr> args;
        if (!check(RIGHT_PAREN)) {
            do {
                Token arg_type = advance();
                Token arg_name = advance();

                stack_size_bytes += 8;

                std::string scoped_name =
                    identifierName.lexeme + "::" + arg_name.lexeme;
                var_offset_lookup[scoped_name] = stack_size_bytes;
                var_offset_lookup[arg_name.lexeme] = stack_size_bytes;

                var_type_lookup[arg_name.lexeme] = arg_type.lexeme;
                var_lookup.push(arg_name.lexeme);

                params_declared.push_back(arg_name.lexeme); // Track param

                std::cout << "  Param: " << arg_name.lexeme << " at offset "
                          << stack_size_bytes << std::endl;

                args.push_back(std::make_unique<FunctionParameterStmt>(
                    std::move(arg_type), std::move(arg_name.lexeme),
                    stack_size_bytes));
            } while (match(COMMA));
        }

        consume(RIGHT_PAREN, "Exptected a '(' after function definition");
        consume(LEFT_CURLY, "Extected function block after definition");

        StmtPtr block_ptr = parseBlock();

        // SAVE this function's stack size for CodeGen
        int function_stack_size = stack_size_bytes;

        std::cout << "  Function stack size: " << function_stack_size
                  << std::endl;

        // --- CLEAN UP PARAMETERS ---
        // parseBlock only cleaned up variables inside the block.
        // We must manually remove the function parameters now to prevent them
        // from leaking into the global scope or next function.
        for (const std::string &param : params_declared) {
            // Remove from var_lookup stack if it's the top element
            if (!var_lookup.empty() && var_lookup.top() == param) {
                var_lookup.pop();
            }
            // Erase from maps
            var_offset_lookup.erase(param);
            var_type_lookup.erase(param);

            // Clean up scoped name alias
            std::string scoped = identifierName.lexeme + "::" + param;
            var_offset_lookup.erase(scoped);
        }

        // Restore stack size for next function (reset to 0 effectively)
        stack_size_bytes = saved_stack_size;

        return std::make_unique<FunctionDeclStmt>(
            std::move(identifierType), std::move(identifierName),
            std::move(args), std::move(block_ptr), function_stack_size);
    }

    std::optional<ExprPtr> init;
    if (match(OPERATOR_ASSIGNMENT)) {
        init = parseExpression();
    }
    consume(SEMICOLON, "Expected ';' after initialization.");

    // Check for shadowing
    if (var_offset_lookup.find(identifierName.lexeme) !=
        var_offset_lookup.end()) {
        // Save the OLD offset and type onto the shadow stack
        shadow_offset_stack[identifierName.lexeme].push_back(
            var_offset_lookup[identifierName.lexeme]);
        shadow_type_stack[identifierName.lexeme].push_back(
            var_type_lookup[identifierName.lexeme]);
    }

    // Calculate new offset
    stack_size_bytes += data_type_size_lookup[identifierType.lexeme];
    var_offset_lookup[identifierName.lexeme] = stack_size_bytes;

    var_type_lookup[identifierName.lexeme] = identifierType.lexeme;

    var_lookup.push(identifierName.lexeme);

    std::cout << "  Var: " << identifierName.lexeme << " at offset "
              << stack_size_bytes << std::endl;

    return std::make_unique<VariableDeclStmt>(
        identifierType, identifierName.lexeme, std::move(init),
        stack_size_bytes);
}

StmtPtr Parser::parseBlock() {

    size_t vars_before_block = var_lookup.size();

    std::vector<StmtPtr> stmts;

    while (!check(RIGHT_CURLY) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    consume(RIGHT_CURLY, "Expected '}' after block");

    // Unwind variables declared in this block
    while (var_lookup.size() > vars_before_block) {
        std::string name = var_lookup.top();
        var_lookup.pop();

        // Remove the current definition
        var_offset_lookup.erase(name);
        var_type_lookup.erase(name);

        // Restore shadowed definition if it exists in the shadow stack
        if (!shadow_offset_stack[name].empty()) {
            var_offset_lookup[name] = shadow_offset_stack[name].back();
            shadow_offset_stack[name].pop_back();
        }
        if (!shadow_type_stack[name].empty()) {
            var_type_lookup[name] = shadow_type_stack[name].back();
            shadow_type_stack[name].pop_back();
        }
    }

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

    // Track variables declared in the initializer (e.g., int i = 0)
    size_t vars_before_loop = var_lookup.size();

    std::optional<StmtPtr> init;

    if (!check(SEMICOLON)) {
        if (match(KEYWORD_TYPE_INT) || match(KEYWORD_TYPE_FLOAT) ||
            match(KEYWORD_TYPE_STRING)) {
            init = parseVarOrFunctionDecl();
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

    // Clean up loop variables (e.g., 'i') after the loop body
    while (var_lookup.size() > vars_before_loop) {
        std::string name = var_lookup.top();
        var_lookup.pop();
        var_offset_lookup.erase(name);
        var_type_lookup.erase(name);

        // Restore shadowed vars if any
        if (!shadow_offset_stack[name].empty()) {
            var_offset_lookup[name] = shadow_offset_stack[name].back();
            shadow_offset_stack[name].pop_back();
        }
        if (!shadow_type_stack[name].empty()) {
            var_type_lookup[name] = shadow_type_stack[name].back();
            shadow_type_stack[name].pop_back();
        }
    }

    return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                     std::move(post), std::move(body));
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

Program Parser::parse() {
    // Initialize standard library types
    var_type_lookup["input_f"] = "float";
    var_type_lookup["sqrt_f"] = "float";
    var_type_lookup["sin_f"] = "float";
    var_type_lookup["cos_f"] = "float";
    var_type_lookup["tan_f"] = "float";
    var_type_lookup["abs_f"] = "float";

    Program prog;

    while (!isAtEnd()) {
        prog.statements.push_back(parseStatement());
    }

    prog.var_offset_lookup = var_offset_lookup;
    prog.var_type_lookup = var_type_lookup;
    prog.stack_size = stack_size_bytes;

    return prog;
}
