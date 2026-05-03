#include "AbstractSyntaxTree.h"
#include "Errors.h"
#include "Parser.h"
#include "Token.h"
#include "Type.h"
#include "utils.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

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

    error(peek(), msg);
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

            consume(TokenType::RIGHT_PAREN, "Expected a ')'");

            Type returnType = TypeSystem::Int64;
            std::vector<Type> paramTypes;

            auto sym = symbolTable.lookup(identifierName.lexeme);
            if (sym && sym->is_function) {
                returnType = sym->type;
                paramTypes = sym->param_types;
            } else {
                error(identifierName, "Implicit declaration of '" + identifierName.lexeme + "' is not allowed.");
            }

            return std::make_unique<FunctionCallExpr>(std::move(identifierName), std::move(args), returnType, paramTypes);
        }

        if (match(TokenType::LEFT_SQUARE)) {
            Token bracket = previous();
            ExprPtr index = parseExpression();
            consume(TokenType::RIGHT_SQUARE, "Expected ']' after array index.");

            auto sym = symbolTable.lookup(identifierName.lexeme);
            if (!sym) {
                error(identifierName, "Undefined variable '" + identifierName.lexeme + "'.");
            }

            auto arrayIdent = std::make_unique<IdentifierExpr>(identifierName, sym->offset, sym->type);
            return std::make_unique<ArrayAccessExpr>(std::move(arrayIdent), std::move(index), bracket);
        }

        if (match(TokenType::PUNCTUATION_DOT)) {
            auto sym = symbolTable.lookup(identifierName.lexeme);
            if (!sym) {
                error(identifierName, "Undefined variable '" + identifierName.lexeme + "'.");
            }
            if (sym->type.kind != TypeKind::CLASS) {
                error(identifierName, "Member access requires a class type.");
            }

            consume(TokenType::IDENTIFIER, "Expected member name after '.'.");
            Token memberName = previous();

            auto classIt = class_registry.find(sym->type.name);
            if (classIt == class_registry.end()) {
                error(identifierName, "Unknown class '" + sym->type.name + "'.");
            }

            const ClassTypeInfo &classInfo = classIt->second;

            if (match(TokenType::LEFT_PAREN)) {
                std::vector<ExprPtr> args;

                Token ampToken("&", identifierName.row, identifierName.column, TokenType::OPERATOR_AMPERSAND);
                auto thisIdent = std::make_unique<IdentifierExpr>(identifierName, sym->offset, sym->type);
                args.push_back(std::make_unique<UnaryExpr>(ampToken, std::move(thisIdent)));

                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        args.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }

                consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");

                std::string mangledName = mangle_method(classInfo.name, memberName.lexeme);
                auto funcSym = symbolTable.lookup(mangledName);
                if (!funcSym || !funcSym->is_function) {
                    error(memberName, "Unknown method '" + memberName.lexeme + "'.");
                }

                Token mangledToken = memberName;
                mangledToken.lexeme = mangledName;

                return std::make_unique<FunctionCallExpr>(std::move(mangledToken), std::move(args), funcSym->type, funcSym->param_types);
            }

            auto fieldIt = classInfo.fields.find(memberName.lexeme);
            if (fieldIt == classInfo.fields.end()) {
                error(memberName, "Unknown field '" + memberName.lexeme + "'.");
            }

            auto objIdent = std::make_unique<IdentifierExpr>(identifierName, sym->offset, sym->type);
            return std::make_unique<PropertyAccessExpr>(std::move(objIdent), memberName, fieldIt->second.type, fieldIt->second.offset);
        }

        auto sym = symbolTable.lookup(identifierName.lexeme);
        if (!sym) {
            error(identifierName, "Undefined variable '" + identifierName.lexeme + "'.");
        }
        return std::make_unique<IdentifierExpr>(identifierName, sym->offset, sym->type);
    }

    if (match(TokenType::LEFT_PAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression. ");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    if (match(TokenType::LEFT_CURLY)) {
        std::vector<ExprPtr> elements;

        if (!match(TokenType::RIGHT_CURLY)) {
            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }

        consume(TokenType::RIGHT_CURLY, "Expected '}' after array initializer.");
        return std::make_unique<ArrayLiteralExpr>(std::move(elements));
    }

    // throw ParseError(previous(), "Expected expression");
    error(previous(), "Expected expression");
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::OPERATOR_MINUS) || match(TokenType::EXCLAMATION) || match(TokenType::OPERATOR_AMPERSAND) ||
        match(TokenType::OPERATOR_ASTERISK)) {
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

        if (auto *unary = dynamic_cast<UnaryExpr *>(left.get())) {
            if (unary->op.type == TokenType::OPERATOR_ASTERISK) {
                return std::make_unique<BinaryExpr>(equals, std::move(left), std::move(right));
            }
        }

        if (auto *arrAccess = dynamic_cast<ArrayAccessExpr *>(left.get())) {
            return std::make_unique<BinaryExpr>(equals, std::move(left), std::move(right));
        }

        if (auto *propAccess = dynamic_cast<PropertyAccessExpr *>(left.get())) {
            return std::make_unique<BinaryExpr>(equals, std::move(left), std::move(right));
        }

        // throw ParseError(previous(), "Invalid assignment target. Only variables or pointer dereferences are allowed.");
        error(previous(), "Invalid assignment target. Only variables or pointer dereferences are allowed.");
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

    if (check(TokenType::IDENTIFIER)) {
        auto it = class_registry.find(peek().lexeme);
        if (it != class_registry.end()) {
            advance();
            return parseVarOrFunctionDecl();
        }
    }

    if (match(TokenType::KEYWORD_RETURN)) {
        return parseReturnStmt();
    }

    if (match(TokenType::KEYWORD_CLASS)) {
        return parseClassDecl();
    }

    return parseExpressionStatement();
}

StmtPtr Parser::parseExpressionStatement() {
    ExprPtr expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression statement.");

    return std::make_unique<ExprStmt>(std::move(expr));
}

StmtPtr Parser::parseVarOrFunctionDecl() {
    bool isPtr = false;
    Token identifierTypeToken = previous();

    Type type = TypeSystem::Int64;
    try {
        type = TypeSystem::from_string(identifierTypeToken.lexeme);
    } catch (const TypeError &e) {
        ErrorReporter::report(e);
    }

    if (match(TokenType::LEFT_SQUARE)) {
        if (type.kind == TypeKind::VOID) {
            error(identifierTypeToken, "Arrays of type 'void' are not allowed.");
        }
        consume(TokenType::LITERAL_INTEGER, "Expected array length. ");
        int length = std::get<uint64_t>(previous().fd);
        consume(TokenType::RIGHT_SQUARE, "Expected ']' after array length.");

        type = TypeSystem::createArray(type, length);
    }

    while (check(TokenType::OPERATOR_ASTERISK)) {
        type = TypeSystem::createPointer(type);
        advance();
    }

    consume(TokenType::IDENTIFIER, "Expected identifier name");
    Token identifierName = previous();

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

                Type argType = TypeSystem::Int64;
                try {
                    argType = TypeSystem::from_string(arg_type_tok.lexeme);
                } catch (const TypeError &e) {
                    ErrorReporter::report(e);
                }

                if (argType.kind == TypeKind::CLASS) {
                    error(arg_type_tok, "Class parameters by value are not supported. Use pointers.");
                }

                if (!symbolTable.declare(arg_name_tok.lexeme, argType)) {
                    // throw ParseError(arg_name_tok, "Duplicate parameter name.");
                    error(arg_name_tok, "Duplicate parameter name.");
                }

                auto sym = symbolTable.lookup(arg_name_tok.lexeme);

                std::cout << "  Param: " << arg_name_tok.lexeme << " at offset " << sym->offset << std::endl;

                args.push_back(
                    std::make_unique<FunctionParameterStmt>(std::move(arg_type_tok), std::move(arg_name_tok.lexeme), sym->offset));
                paramTypes.push_back(argType);
            } while (match(TokenType::COMMA));
        }

        if (type.kind == TypeKind::CLASS) {
            error(identifierName, "Returning class by value is not supported. Use pointers.");
        }

        if (!symbolTable.declareFunction(identifierName.lexeme, type, paramTypes)) {
            error(identifierName, "Function '" + identifierName.lexeme + "' already declared.");
        }

        std::cout << "--- Inside Function " << identifierName.lexeme << " (Params parsed) ---" << std::endl;

        consume(TokenType::RIGHT_PAREN, "Expected a ')' after function definition");
        consume(TokenType::LEFT_CURLY, "Expected function block after definition");

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
        if (type.kind == TypeKind::CLASS) {
            error(identifierTypeToken, "Class copy initialization is not supported. Use field assignments.");
        }
        init = parseExpression();
    }

    if (type.kind == TypeKind::VOID) {
        error(identifierTypeToken, "Variables of type void are not allowed.");
    }

    consume(TokenType::SEMICOLON, "Expected ';' after initialization.");

    if (!symbolTable.declare(identifierName.lexeme, type)) {
        // throw ParseError(identifierName, "Variable '" + identifierName.lexeme + "' already declared in this scope.");
        error(identifierName, "Variable '" + identifierName.lexeme + "' already declared in this scope.");
    }

    auto sym = symbolTable.lookup(identifierName.lexeme);

    std::cout << "  Var: " << identifierName.lexeme << " at offset " << sym->offset << std::endl;

    return std::make_unique<VariableDeclStmt>(identifierTypeToken, identifierName.lexeme, std::move(init), sym->offset, type);
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
    consume(TokenType::RIGHT_PAREN, "Expected ')' after condition");

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
        } else if (check(TokenType::IDENTIFIER) && class_registry.count(peek().lexeme)) {
            advance();
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

StmtPtr Parser::parseClassDecl() {
    Token className = advance();
    consume(TokenType::LEFT_CURLY, "Expected '{' before class body.");

    ClassTypeInfo classInfo;
    classInfo.name = className.lexeme;
    size_t current_offset = 0;

    size_t scan_pos = pos;
    while (scan_pos < tokens.size() && tokens[scan_pos].type != TokenType::RIGHT_CURLY) {
        Token typeTok = tokens[scan_pos++];

        Type memberType = TypeSystem::Int64;
        try {
            memberType = TypeSystem::from_string(typeTok.lexeme);
        } catch (const TypeError &e) {
            ErrorReporter::report(e);
        }

        if (scan_pos >= tokens.size() || tokens[scan_pos].type != TokenType::IDENTIFIER) {
            error(tokens[scan_pos - 1], "Expected member name.");
        }

        Token memberName = tokens[scan_pos++];

        if (scan_pos < tokens.size() && tokens[scan_pos].type == TokenType::LEFT_PAREN) {
            scan_pos++;

            if (memberType.kind == TypeKind::CLASS) {
                error(memberName, "Returning class by value is not supported. Use pointers.");
            }

            std::vector<Type> paramTypes;
            Type thisType = TypeSystem::createPointer(Type{.name = classInfo.name, .kind = TypeKind::CLASS, .size_bytes = 0});
            paramTypes.push_back(thisType);

            if (scan_pos < tokens.size() && tokens[scan_pos].type != TokenType::RIGHT_PAREN) {
                while (true) {
                    if (scan_pos + 1 >= tokens.size()) {
                        error(tokens[scan_pos], "Expected parameter type and name.");
                    }

                    Token argTypeTok = tokens[scan_pos++];
                    Token argNameTok = tokens[scan_pos++];

                    Type argType = TypeSystem::Int64;
                    try {
                        argType = TypeSystem::from_string(argTypeTok.lexeme);
                    } catch (const TypeError &e) {
                        ErrorReporter::report(e);
                    }

                    if (argType.kind == TypeKind::CLASS) {
                        error(argTypeTok, "Class parameters by value are not supported. Use pointers.");
                    }

                    paramTypes.push_back(argType);

                    if (scan_pos < tokens.size() && tokens[scan_pos].type == TokenType::COMMA) {
                        scan_pos++;
                        continue;
                    }
                    break;
                }
            }

            if (scan_pos >= tokens.size() || tokens[scan_pos].type != TokenType::RIGHT_PAREN) {
                error(tokens[scan_pos - 1], "Expected ')' after parameters.");
            }
            scan_pos++;

            std::string mangledName = mangle_method(classInfo.name, memberName.lexeme);
            if (!symbolTable.declareFunction(mangledName, memberType, paramTypes)) {
                error(memberName, "Duplicate method '" + memberName.lexeme + "'.");
            }

            if (scan_pos >= tokens.size() || tokens[scan_pos].type != TokenType::LEFT_CURLY) {
                error(tokens[scan_pos - 1], "Expected '{' before method body.");
            }

            int brace_depth = 1;
            scan_pos++;
            while (scan_pos < tokens.size() && brace_depth > 0) {
                if (tokens[scan_pos].type == TokenType::LEFT_CURLY) brace_depth++;
                if (tokens[scan_pos].type == TokenType::RIGHT_CURLY) brace_depth--;
                scan_pos++;
            }
        } else {
            if (scan_pos >= tokens.size() || tokens[scan_pos].type != TokenType::SEMICOLON) {
                error(tokens[scan_pos - 1], "Expected ';' after field declaration.");
            }
            scan_pos++;
        }
    }

    std::vector<StmtPtr> methods;

    while (!check(TokenType::RIGHT_CURLY) && !isAtEnd()) {
        Type memberType = TypeSystem::Int64;
        try {
            memberType = TypeSystem::from_string(peek().lexeme);
        } catch (const TypeError &e) {
            ErrorReporter::report(e);
        }
        advance();

        Token memberName = previous();
        consume(TokenType::IDENTIFIER, "Expected member name.");
        memberName = previous();

        if (match(TokenType::LEFT_PAREN)) {
            symbolTable.reset_local_offset();

            if (memberType.kind == TypeKind::CLASS) {
                error(memberName, "Returning class by value is not supported. Use pointers.");
            }

            std::vector<StmtPtr> args;
            std::vector<Type> paramTypes;

            symbolTable.enter_scope();

            Type thisType = TypeSystem::createPointer(Type{.name = classInfo.name, .kind = TypeKind::CLASS, .size_bytes = 0});

            if (!symbolTable.declare("this", thisType)) {
                error(memberName, "Duplicate parameter name 'this'.");
            }

            auto thisSym = symbolTable.lookup("this");

            Token thisTypeToken("uint64", memberName.row, memberName.column, TokenType::KEYWORD_TYPE_UINT64);
            args.push_back(std::make_unique<FunctionParameterStmt>(thisTypeToken, "this", thisSym->offset));
            paramTypes.push_back(thisType);

            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    Token arg_type_tok = advance();
                    Token arg_name_tok = advance();

                    Type argType = TypeSystem::Int64;
                    try {
                        argType = TypeSystem::from_string(arg_type_tok.lexeme);
                    } catch (const TypeError &e) {
                        ErrorReporter::report(e);
                    }

                    if (argType.kind == TypeKind::CLASS) { // CHANGE
                        error(arg_type_tok, "Class parameters by value are not supported. Use pointers.");
                    }

                    if (!symbolTable.declare(arg_name_tok.lexeme, argType)) {
                        error(arg_name_tok, "Duplicate parameter name.");
                    }

                    auto sym = symbolTable.lookup(arg_name_tok.lexeme);

                    args.push_back(std::make_unique<FunctionParameterStmt>(arg_type_tok, arg_name_tok.lexeme, sym->offset));
                    paramTypes.push_back(argType);
                } while (match(TokenType::COMMA));
            }

            consume(TokenType::RIGHT_PAREN, "Expected ')' after method parameters.");
            consume(TokenType::LEFT_CURLY, "Expected '{' before method body.");

            StmtPtr block_ptr = parseBlock();

            int function_stack_size = symbolTable.getMaxStackSize();

            symbolTable.exit_scope();

            std::string mangled_name = mangle_method(classInfo.name, memberName.lexeme);
            Token mangledToken = memberName;
            mangledToken.lexeme = mangled_name;

            methods.push_back(std::make_unique<FunctionDeclStmt>(std::move(memberType), std::move(mangledToken), std::move(args),
                                                                 std::move(block_ptr), function_stack_size));
        } else {
            if (memberType.kind == TypeKind::VOID) {
                error(memberName, "Fields of type void are not allowed.");
            }

            consume(TokenType::SEMICOLON, "Expected ';' after field declaration.");

            int alignment = memberType.size_bytes;
            while (current_offset % alignment != 0)
                current_offset++;

            classInfo.fields[memberName.lexeme] = {memberType, current_offset};
            current_offset += memberType.size_bytes;
        }
    }

    classInfo.total_size_bytes = (current_offset == 0) ? 1 : current_offset;
    class_registry[classInfo.name] = classInfo;

    consume(TokenType::RIGHT_CURLY, "Expected '}' after class body.");
    consume(TokenType::SEMICOLON, "Expected ';' after '}'.");
    return std::make_unique<ClassDeclStmt>(className, std::move(methods));
}

ExprPtr Parser::parseExpression() { return parseAssignment(); }

Program Parser::parse() {
    Program prog;
    symbolTable.reset();

    register_intrinsics(symbolTable);

    while (!isAtEnd()) {
        try {
            prog.statements.push_back(parseStatement());
        } catch (const ParserPanic &) {
            synchronize();
        }
    }

    prog.stack_size = symbolTable.getMaxStackSize();

    return prog;
}

[[noreturn]] void Parser::error(const Token &tok, const std::string &msg) {
    ErrorReporter::report(ParseError(tok, msg));
    throw ParserPanic(); // Throwing internal panic to unwind the stack safely
}

void Parser::synchronize() {
    advance(); // Consume the token that caused the error

    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) {
        case TokenType::KEYWORD_TYPE_INT64:
        case TokenType::KEYWORD_TYPE_INT32:
        case TokenType::KEYWORD_TYPE_INT16:
        case TokenType::KEYWORD_TYPE_INT8:
        case TokenType::KEYWORD_TYPE_FLOAT64:
        case TokenType::KEYWORD_TYPE_FLOAT32:
        case TokenType::KEYWORD_TYPE_VOID:
        case TokenType::KEYWORD_IF:
        case TokenType::KEYWORD_WHILE:
        case TokenType::KEYWORD_FOR:
        case TokenType::KEYWORD_RETURN: return; // We found a valid boundary to resume parsing!
        default: break;
        }
        advance();
    }
}
