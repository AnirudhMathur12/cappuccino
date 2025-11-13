//
// Created by Anirudh Mathur on 12/11/25.
//

#include "Token.h"

#include <unordered_map>


Token::Token(const std::string& p_lexeme, int p_row, int p_col, TokenType p_type) : lexeme(p_lexeme), row(p_row), column(p_col), type(p_type), fd(std::monostate{}) {
    if (type == TokenType::LITERAL_STRING) {
        fd = p_lexeme.substr(1, lexeme.length() - 2);
    } else if (type == TokenType::LITERAL_INTEGER) {
        fd = std::stoi(p_lexeme);
    } else if (type == TokenType::LITERAL_FLOAT) {
        fd = std::stof(p_lexeme);
    }
}

inline std::string token_type_to_string(TokenType type) {
    switch (type) {
        case LITERAL_STRING:        return "LITERAL_STRING";
        case LITERAL_FLOAT:         return "LITERAL_FLOAT";
        case LITERAL_INTEGER:           return "LITERAL_INTEGER";
        case IDENTIFIER:            return "IDENTIFIER";
        case KEYWORD_IF:            return "KEYWORD_IF";
        case KEYWORD_ELSE:          return "KEYWORD_ELSE";
        case KEYWORD_FOR:           return "KEYWORD_FOR";
        case KEYWORD_RETURN:        return "KEYWORD_RETURN";
        case KEYWORD_WHILE:         return "KEYWORD_WHILE";
        case OPERATOR_PLUS:         return "OPERATOR_PLUS";
        case OPERATOR_MINUS:        return "OPERATOR_MINUS";
        case OPERATOR_ASTERISK:     return "OPERATOR_ASTERISK";
        case OPERATOR_EQUALS:       return "OPERATOR_EQUALS";
        case OPERATOR_FORWARD_SLASH:return "OPERATOR_FORWARD_SLASH";
        case OPERATOR_BACKSLASH:    return "OPERATOR_BACKSLASH";
        case LEFT_PAREN:            return "LEFT_PAREN";
        case RIGHT_PAREN:           return "RIGHT_PAREN";
        case EXCL_EQUAL:            return "EXCL_EQUAL";
        case EXCLAMATION:           return "EXCLAMATION";
        case TOKEN_EOF:             return "TOKEN_EOF";
        case KEYWORD_TYPE_FLOAT:     return "KEYWORD_TYPE_FLOAT";
        case KEYWORD_TYPE_INT:       return "KEYWORD_TYPE_INT";
        case KEYWORD_TYPE_STRING:    return "KEYWORD_TYPE_STRING";
        case SEMICOLON:             return "SEMICOLON";
        case RIGHT_CURLY:           return "RIGHT_CURLY";
        case LEFT_CURLY:             return "LEFT_CURLY";
        default:                    return "UNKNOWN_TOKEN";
    }
}

inline std::string fd_to_string(const std::variant<std::monostate, int, float, std::string>& v) {
    return std::visit([](auto&& value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::monostate>)
            return "NoValue";
        else if constexpr (std::is_same_v<T, int>)
            return std::to_string(value);
        else if constexpr (std::is_same_v<T, float>)
            return std::to_string(value);
        else if constexpr (std::is_same_v<T, std::string>)
            return value;
        else
            return "<Invalid>";
    }, v);
}


std::ostream& operator<<(std::ostream& os, const Token& tok) {
    return os << "Token { "
              << "type = "   << token_type_to_string(tok.type)
              << ", lexeme = \"" << tok.lexeme << "\""
              << ", row = "  << tok.row
              << ", column = " << tok.column
              << ", fd = "   << fd_to_string(tok.fd)
              << " }";
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        start = current;
        skipWhiteSpaceAndComments();
        if (isAtEnd()) break;

        char c = advance();

        switch (c) {
            case '(': tokens.push_back(makeToken(TokenType::LEFT_PAREN)); break;
            case ')': tokens.push_back(makeToken(TokenType::RIGHT_PAREN)); break;
            case '{': tokens.push_back(makeToken(TokenType::LEFT_CURLY)); break;
            case '}': tokens.push_back(makeToken(TokenType::RIGHT_CURLY)); break;
            case '+':tokens.push_back(makeToken(TokenType::OPERATOR_PLUS)); break;
            case ';': tokens.push_back(makeToken(TokenType::SEMICOLON)); break;
            case '=': tokens.push_back(makeToken(TokenType::OPERATOR_EQUALS)); break;

            case '!':
                tokens.push_back(makeToken(
                    (peek() == '=') ? (advance(), TokenType::EXCL_EQUAL)
                    : TokenType::EXCLAMATION));
                break;

            case '"' : tokens.push_back(string()); break;
            default:
                if (std::isdigit(c)) {
                    tokens.push_back(number());
                } else if (std::isalpha(c) || c == '_') {
                    tokens.push_back(identifier());
                } else {
                    // What the fuck was in that file
                }
        }
    }

    tokens.push_back(Token("", line, column, TokenType::TOKEN_EOF));
    return tokens;
}

Token Tokenizer::string() {
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            line++;
            column = 0;
        }

        if (isAtEnd()) {
            // Unterminated string
        }


        advance();
    }

    advance();

    std::string value = src.substr(start, current - start);
    return Token(value, line, column, TokenType::LITERAL_STRING);
}

Token Tokenizer::number() {
    while (std::isdigit(peek()) && !isAtEnd()) advance();

    bool isFloat = false;

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance();
        while (std::isdigit(peek())) advance();
    }

    std::string lex = src.substr(start, current - start);
    start = current;

    return Token(lex, line, column, isFloat ? TokenType::LITERAL_FLOAT : TokenType::LITERAL_INTEGER);
}

std::unordered_map<std::string, TokenType> keywords = {
    {"if", TokenType::KEYWORD_IF},
    {"else", TokenType::KEYWORD_ELSE},
    {"for", TokenType::KEYWORD_FOR},
    {"return", TokenType::KEYWORD_RETURN},
    {"while", TokenType::KEYWORD_WHILE},
    {"int", TokenType::KEYWORD_TYPE_INT},
    {"float", TokenType::KEYWORD_TYPE_FLOAT},
    {"string", TokenType::KEYWORD_TYPE_STRING},
};

Token Tokenizer::identifier() {
    while ((isalnum(peek()) || peek() == '_') && !isAtEnd()) advance();

    std::string lex = src.substr(start, current - start);
    auto it = keywords.find(lex);

    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

    return Token(lex, line, column, type);
}

char Tokenizer::advance(){
    column++;
    return src[current++];
}

char Tokenizer::peek() const {
    return isAtEnd() ? '\0' : src[current];
}

char Tokenizer::peekNext() const {
    return (current + 1 >= src.size()) ? '\0' : src[current + 1];
}

Token Tokenizer::makeToken(TokenType type) {
    return Token(src.substr(start, current - start), line, column, type);
}

void Tokenizer::skipWhiteSpaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                advance();
                line++;
                column = 1;
                break;

            case '/':
                if (peekNext() == '/') {
                    while (!isAtEnd() && peek() != '\n') advance();
                } else {
                    return;
                }

                break;

            default:
                start = current;
                return;
        }
    }
}