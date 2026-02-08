#include "Parser.h"
#include "AbstractSyntaxTree.h"
#include "Token.h"
#include "Type.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

ParseError::ParseError(const Token &tok, const std::string &msg) : std::runtime_error(format_message(tok, msg)), token(tok) {}

std::string ParseError::format_message(const Token &tok, const std::string msg) {
    std::ostringstream oss;
    oss << "[Line:" << tok.row << ", Column:" << tok.column << "] " << msg << std::endl;

    return oss.str();
}

Parser::Parser(const std::vector<Token> &p_tokens) : tokens(p_tokens) {}

bool Parser::isAtEnd() const { return peek().type == TokenType::TOKEN_EOF; }

const Token &Parser::peek() const { return tokens[pos]; }

const Token &Parser::peekNext() const { return isAtEnd() ? tokens[pos] : tokens[pos + 1]; }

const Token &Parser::previous() const { return tokens[pos - 1]; }

const Token &Parser::advance() {
    if (!isAtEnd()) pos++;
    return previous();
}

void register_intrinsics(SymbolTable &sym) {
    // void print(int)
    sym.declareFunction("print", TypeSystem::Void, {TypeSystem::Int64});
    // void print_f(float)
    sym.declareFunction("print_f", TypeSystem::Void, {TypeSystem::Float64});
    // void print_s(string)
    sym.declareFunction("print_s", TypeSystem::Void, {TypeSystem::StringLiteral});

    // int input_i()
    sym.declareFunction("input_i", TypeSystem::Int64, {});
    // float input_f()
    sym.declareFunction("input_f", TypeSystem::Float64, {});

    // Math (float -> float)
    std::vector<std::string> math_funcs = {"sqrt_f", "sin_f", "cos_f", "tan_f", "abs_f"};
    for (const auto &name : math_funcs) {
        sym.declareFunction(name, TypeSystem::Float64, {TypeSystem::Float64});
    }
}

bool Parser::check(TokenType t) const {
    if (isAtEnd()) return false;
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

    if (match(TokenType::LITERAL_FLOAT) || match(TokenType::LITERAL_INTEGER) || match(TokenType::LITERAL_STRING)) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match(TokenType::IDENTIFIER)) {
        Token identifierName = previous();
        if (match(TokenType::LEFT_PAREN)) {

            std::vector<ExprPtr> args;
            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RIGHT_PAREN, "Exptected a ')'");

            Type returnType = TypeSystem::Int64;
            std::vector<Type> paramTypes;

            auto sym = symbolTable.lookup(identifierName.lexeme);
            if (sym && sym->is_function) {
                returnType = sym->type;
                paramTypes = sym->param_types;
            } else {
                std::cerr << "Warning: Implicit declaration of function '" << identifierName.lexeme << "'" << std::endl;
            }

            return std::make_unique<FunctionCallExpr>(std::move(identifierName), std::move(args), returnType, paramTypes);
        }
        auto sym = symbolTable.lookup(identifierName.lexeme);
        if (!sym) {
            throw ParseError(identifierName, "Undefined variable '" + identifierName.lexeme + "'.");
        }
        return std::make_unique<IdentifierExpr>(identifierName, sym->offset, sym->type);
    }

    if (match(TokenType::LEFT_PAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression. ");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw ParseError(previous(), "Expected expression");
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::OPERATOR_MINUS) || match(TokenType::EXCLAMATION)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return parsePrimary();
}

ExprPtr Parser::parseFactor() {
    ExprPtr expr = parseUnary();

    while (match(TokenType::OPERATOR_ASTERISK) || match(TokenType::OPERATOR_FORWARD_SLASH)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expr = parseFactor();

    while (match(TokenType::OPERATOR_PLUS) || match(TokenType::OPERATOR_MINUS)) {
        Token op = previous();
        ExprPtr right = parseFactor();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseTerm();

    while (match(TokenType::OPERATOR_LESS) || match(TokenType::OPERATOR_GREATER) || match(TokenType::OPERATOR_LESS_EQUALS) ||
           match(TokenType::OPERATOR_GREATER_EQUALS)) {
        Token op = previous();
        ExprPtr right = parseTerm();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();

    while (match(TokenType::EXCL_EQUAL) || match(TokenType::OPERATOR_EQUALITY)) {
        Token op = previous();
        ExprPtr right = parseComparison();
        expr = std::make_unique<BinaryExpr>(op, std::move(expr), std::move(right));
    }

    return expr;
}

ExprPtr Parser::parseAssignment() {
    ExprPtr left = parseEquality();

    if (match(TokenType::OPERATOR_ASSIGNMENT)) {
        Token equals = previous();
        ExprPtr right = parseAssignment();

        if (auto *ident = dynamic_cast<IdentifierExpr *>(left.get())) {
            return std::make_unique<BinaryExpr>(equals, std::move(left), std::move(right));
        }

        throw ParseError(previous(), "Invalid assignment target.");
    }

    return left;
}

StmtPtr Parser::parseReturnStmt() {
    Token t = previous();
    if (match(TokenType::SEMICOLON)) {
        return std::make_unique<ReturnStmt>(t, std::nullopt);
    }

    ExprPtr ex = parseExpression();
    consume(TokenType::SEMICOLON, "Expected';' after expression");
    return std::make_unique<ReturnStmt>(t, std::move(ex));
}

StmtPtr Parser::parseStatement() {
    if (match(TokenType::LEFT_CURLY)) {
        return parseBlock();
    }

    if (match(TokenType::KEYWORD_IF)) {
        return parseIf();
    }

    if (match(TokenType::KEYWORD_WHILE)) {
        return parseWhile();
    }

    if (match(TokenType::KEYWORD_FOR)) {
        return parseFor();
    }

    if (match(TokenType::KEYWORD_TYPE_FLOAT32) || match(TokenType::KEYWORD_TYPE_FLOAT64) || match(TokenType::KEYWORD_TYPE_INT8) ||
        match(TokenType::KEYWORD_TYPE_INT16) || match(TokenType::KEYWORD_TYPE_INT32) || match(TokenType::KEYWORD_TYPE_INT64) ||
        match(TokenType::KEYWORD_TYPE_UINT8) || match(TokenType::KEYWORD_TYPE_UINT16) || match(TokenType::KEYWORD_TYPE_UINT32) ||
        match(TokenType::KEYWORD_TYPE_UINT64) || match(TokenType::KEYWORD_TYPE_VOID)) {
        return parseVarOrFunctionDecl();
    }

    if (match(TokenType::KEYWORD_RETURN)) {
        return parseReturnStmt();
    }

    return parseExpressionStatement();
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement.");

    return std::make_unique<ExprStmt>(std::move(expr));
}

std::stack<std::string> var_lookup;
std::unordered_map<std::string, int> data_type_size_lookup = {{"int", 8}, {"float", 8}};

StmtPtr Parser::parseVarOrFunctionDecl() {
    Token identifierTypeToken = previous();
    consume(TokenType::IDENTIFIER, "Expected identifier name");
    Token identifierName = previous();

    Type type = TypeSystem::from_string(identifierTypeToken.lexeme);

    if (match(TokenType::LEFT_PAREN)) {
        // Save the current stack offset to restore after this function

        std::cout << "=== Parsing function: " << identifierName.lexeme << " ===" << std::endl;

        symbolTable.reset_local_offset();

        std::vector<StmtPtr> args;
        std::vector<Type> paramTypes;

        symbolTable.enter_scope();

        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                Token arg_type_tok = advance();
                Token arg_name_tok = advance();

                Type argType = TypeSystem::from_string(arg_type_tok.lexeme);
                paramTypes.push_back(argType);

                if (!symbolTable.declare(arg_name_tok.lexeme, argType)) {
                    throw ParseError(arg_name_tok, "Duplicate parameter name.");
                }

                auto sym = symbolTable.lookup(arg_name_tok.lexeme);

                std::cout << "  Param: " << arg_name_tok.lexeme << " at offset " << sym->offset << std::endl;

                args.push_back(
                    std::make_unique<FunctionParameterStmt>(std::move(arg_type_tok), std::move(arg_name_tok.lexeme), sym->offset));
            } while (match(TokenType::COMMA));
        }

        symbolTable.declareFunction(identifierName.lexeme, type, paramTypes);

        std::cout << "--- Inside Function " << identifierName.lexeme << " (Params parsed) ---" << std::endl;

        consume(TokenType::RIGHT_PAREN, "Exptected a '(' after function definition");
        consume(TokenType::LEFT_CURLY, "Extected function block after definition");

        StmtPtr block_ptr = parseBlock();

        // SAVE this function's stack size for CodeGen
        int function_stack_size = symbolTable.getMaxStackSize();
        std::cout << "  Function stack size: " << function_stack_size << std::endl;

        symbolTable.dump();
        symbolTable.exit_scope();

        return std::make_unique<FunctionDeclStmt>(std::move(type), std::move(identifierName), std::move(args), std::move(block_ptr),
                                                  function_stack_size);
    }

    std::optional<ExprPtr> init;
    if (match(TokenType::OPERATOR_ASSIGNMENT)) {
        init = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after initialization.");

    if (!symbolTable.declare(identifierName.lexeme, type)) {
        throw ParseError(identifierName, "Variable '" + identifierName.lexeme + "' already declared in this scope.");
    }

    auto sym = symbolTable.lookup(identifierName.lexeme);

    std::cout << "  Var: " << identifierName.lexeme << " at offset " << sym->offset << std::endl;

    return std::make_unique<VariableDeclStmt>(identifierTypeToken, identifierName.lexeme, std::move(init), sym->offset);
}

StmtPtr Parser::parseBlock() {
    symbolTable.enter_scope();

    std::vector<StmtPtr> stmts;

    while (!check(TokenType::RIGHT_CURLY) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    consume(TokenType::RIGHT_CURLY, "Expected '}' after block");

    std::cout << "--- Exiting Block ---" << std::endl;
    symbolTable.dump();

    symbolTable.exit_scope();

    return std::make_unique<BlockStmt>(std::move(stmts));
}

StmtPtr Parser::parseIf() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after If. ");
    ExprPtr condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Exptected ')' after condition");

    StmtPtr thenBranch = parseStatement();
    std::optional<StmtPtr> elseBranch;

    if (match(TokenType::KEYWORD_ELSE)) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::parseWhile() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after while");
    ExprPtr condition = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after condition");

    StmtPtr body = parseStatement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

StmtPtr Parser::parseFor() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after for");

    symbolTable.enter_scope();

    std::optional<StmtPtr> init;

    if (!check(TokenType::SEMICOLON)) {
        if (match(TokenType::KEYWORD_TYPE_FLOAT32) || match(TokenType::KEYWORD_TYPE_FLOAT64) || match(TokenType::KEYWORD_TYPE_INT8) ||
            match(TokenType::KEYWORD_TYPE_INT16) || match(TokenType::KEYWORD_TYPE_INT32) || match(TokenType::KEYWORD_TYPE_INT64) ||
            match(TokenType::KEYWORD_TYPE_UINT8) || match(TokenType::KEYWORD_TYPE_UINT16) || match(TokenType::KEYWORD_TYPE_UINT32) ||
            match(TokenType::KEYWORD_TYPE_UINT64)) {
            init = parseVarOrFunctionDecl();
        } else {
            init = parseExpressionStatement();
        }
    } else {
        consume(TokenType::SEMICOLON, "Expected ';' after for initializer");
    }

    std::optional<ExprPtr> cond;
    if (!check(TokenType::SEMICOLON)) cond = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after for condition.");

    std::optional<ExprPtr> post;
    if (!check(TokenType::RIGHT_PAREN)) post = parseExpression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clause.");

    StmtPtr body = parseStatement();

    symbolTable.exit_scope();

    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(post), std::move(body));
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

Program Parser::parse() {
    Program prog;
    symbolTable.reset();

    register_intrinsics(symbolTable);

    while (!isAtEnd()) {
        prog.statements.push_back(parseStatement());
    }

    prog.stack_size = symbolTable.getMaxStackSize();

    return prog;
}
