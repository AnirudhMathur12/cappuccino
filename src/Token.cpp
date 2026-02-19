//
// Created by Anirudh Mathur on 12/11/25.
//

#include "Token.h"
#include "utils.h"

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <unordered_map>
#include <variant>

LexError::LexError(int line, int col, uint32_t codepoint)
    : std::runtime_error("Lexer error at " + std::to_string(line) + ":" + std::to_string(col) + ": unexpected character U+" +
                         to_unicode(codepoint)) {}

std::string LexError::to_unicode(uint32_t codepoint) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << codepoint;
    return oss.str();
}

Token::Token(const std::string &p_lexeme, int p_row, int p_col, TokenType p_type)
    : lexeme(p_lexeme), row(p_row), column(p_col), type(p_type) {
    if (p_type == TokenType::LITERAL_STRING) {
        fd = p_lexeme.substr(1, p_lexeme.length() - 2);
    } else if (p_type == TokenType::LITERAL_FLOAT) {
        fd = std::stod(p_lexeme);
    } else if (p_type == TokenType::LITERAL_INTEGER) {
        fd = static_cast<uint64_t>(std::stoull(p_lexeme));
    }
}

inline std::string token_type_to_string(TokenType type) {
    switch (type) {
        // Literals
    case TokenType::LITERAL_STRING: return "LITERAL_STRING";
    case TokenType::LITERAL_FLOAT: return "LITERAL_FLOAT";
    case TokenType::LITERAL_INTEGER: return "LITERAL_INTEGER";
    case TokenType::IDENTIFIER: return "IDENTIFIER";
    case TokenType::COMMA:
        return "COMMA";

        // Punctuations
    case TokenType::SEMICOLON: return "SEMICOLON";
    case TokenType::LEFT_PAREN: return "LEFT_PAREN";
    case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
    case TokenType::LEFT_CURLY: return "LEFT_CURLY";
    case TokenType::RIGHT_CURLY: return "RIGHT_CURLY";
    case TokenType::LEFT_SQUARE: return "LEFT_SQUARE";
    case TokenType::RIGHT_SQUARE:
        return "RIGHT_SQUARE";

        // Control Flow
    case TokenType::KEYWORD_IF: return "KEYWORD_IF";
    case TokenType::KEYWORD_ELSE: return "KEYWORD_ELSE";
    case TokenType::KEYWORD_FOR: return "KEYWORD_FOR";
    case TokenType::KEYWORD_RETURN: return "KEYWORD_RETURN";
    case TokenType::KEYWORD_WHILE:
        return "KEYWORD_WHILE";

        // Data Types
    case TokenType::KEYWORD_TYPE_INT64: return "KEYWORD_TYPE_INT64";
    case TokenType::KEYWORD_TYPE_INT32: return "KEYWORD_TYPE_INT32";
    case TokenType::KEYWORD_TYPE_INT16: return "KEYWORD_TYPE_INT16";
    case TokenType::KEYWORD_TYPE_INT8: return "KEYWORD_TYPE_INT8";

    case TokenType::KEYWORD_TYPE_UINT64: return "KEYWORD_TYPE_UINT64";
    case TokenType::KEYWORD_TYPE_UINT32: return "KEYWORD_TYPE_UINT32";
    case TokenType::KEYWORD_TYPE_UINT16: return "KEYWORD_TYPE_UINT16";
    case TokenType::KEYWORD_TYPE_UINT8: return "KEYWORD_TYPE_UINT8";

    case TokenType::KEYWORD_TYPE_FLOAT64: return "KEYWORD_TYPE_FLOAT64";
    case TokenType::KEYWORD_TYPE_FLOAT32: return "KEYWORD_TYPE_FLOAT32";

    case TokenType::KEYWORD_TYPE_VOID:
        return "KEYWORD_TYPE_VOID";

        // Operators
    case TokenType::OPERATOR_EQUALITY: return "OPERATOR_EQUALITY";
    case TokenType::OPERATOR_MINUS: return "OPERATOR_MINUS";
    case TokenType::OPERATOR_PLUS: return "OPERATOR_PLUS";
    case TokenType::OPERATOR_ASTERISK: return "OPERATOR_ASTERISK";
    case TokenType::OPERATOR_ASSIGNMENT: return "OPERATOR_ASSIGNEMENT";
    case TokenType::OPERATOR_LESS: return "OPERATOR_LESS";
    case TokenType::OPERATOR_LESS_EQUALS: return "OPERATOR_LESS_EQUALS";
    case TokenType::OPERATOR_GREATER: return "OPERATOR_GREATER";
    case TokenType::OPERATOR_GREATER_EQUALS: return "OPERATOR_GREATER_EQUALS";
    case TokenType::OPERATOR_FORWARD_SLASH: return "OPERATOR_FORWARD_SLASH";
    case TokenType::OPERATOR_BACKSLASH:
        return "OPERATOR_BACKSLASH";

        // Logical
    case TokenType::EXCL_EQUAL: return "EXCL_EQUAL";
    case TokenType::EXCLAMATION:
        return "EXCLAMATION";

        // Meta
    case TokenType::TOKEN_EOF: return "TOKEN_EOF";
    default: return "<UNKNOWN_TOKEN>";
    }
}

inline std::string fd_to_string(const FormattedData &fd) {
    return std::visit(
        [](auto &&value) -> std::string {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, std::monostate>)
                return "No Value";
            else if constexpr (std::is_same_v<T, float>)
                return std::to_string(value);
            else if constexpr (std::is_same_v<T, uint64_t>)
                return std::to_string(value);
            else if constexpr (std::is_same_v<T, std::string>)
                return value;
            else
                return "Invalid Value";
        },
        fd);
}

std::ostream &operator<<(std::ostream &os, const Token &tok) {
    return os << "Token { "
              << "type = " << token_type_to_string(tok.type) << ", lexeme = \"" << tok.lexeme << "\""
              << ", row = " << tok.row << ", column = " << tok.column << " }";
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
        case '[': tokens.push_back(makeToken(TokenType::LEFT_SQUARE)); break;
        case ']': tokens.push_back(makeToken(TokenType::RIGHT_SQUARE)); break;
        case '+': tokens.push_back(makeToken(TokenType::OPERATOR_PLUS)); break;
        case '-': tokens.push_back(makeToken(TokenType::OPERATOR_MINUS)); break;
        case ';': tokens.push_back(makeToken(TokenType::SEMICOLON)); break;
        case '*': tokens.push_back(makeToken(TokenType::OPERATOR_ASTERISK)); break;
        case '/': tokens.push_back(makeToken(TokenType::OPERATOR_FORWARD_SLASH)); break;
        case ',': tokens.push_back(makeToken(TokenType::COMMA)); break;
        case '&': tokens.push_back(makeToken(TokenType::OPERATOR_AMPERSAND)); break;

        case '=':
            tokens.push_back(makeToken((peek() == '=') ? (advance(), TokenType::OPERATOR_EQUALITY) : TokenType::OPERATOR_ASSIGNMENT));
            break;

        case '!': tokens.push_back(makeToken((peek() == '=') ? (advance(), TokenType::EXCL_EQUAL) : TokenType::EXCLAMATION)); break;

        case '<':
            tokens.push_back(makeToken((peek() == '=') ? (advance(), TokenType::OPERATOR_LESS_EQUALS) : TokenType::OPERATOR_LESS));
            break;

        case '>':
            tokens.push_back(makeToken((peek() == '=') ? (advance(), TokenType::OPERATOR_GREATER_EQUALS) : TokenType::OPERATOR_GREATER));
            break;

        case '"': tokens.push_back(string()); break;
        default:
            if (std::isdigit(c)) {
                tokens.push_back(number());
            } else if (std::isalpha(c) || c == '_') {
                tokens.push_back(identifier());
            } else {
                // What the fuck was in that file
                auto [codepoint, byte_count] = decode_utf8(src, current - 1);
                // Consume the remaining bytes of the sequence
                for (int i = 1; i < byte_count; i++)
                    advance();

                throw LexError(line, column, codepoint);
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
    while (std::isdigit(peek()) && !isAtEnd())
        advance();

    bool isFloat = false;

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance();
        while (std::isdigit(peek()))
            advance();
    }

    std::string lex = src.substr(start, current - start);
    start = current;

    return Token(lex, line, column, isFloat ? TokenType::LITERAL_FLOAT : TokenType::LITERAL_INTEGER);
}

std::unordered_map<std::string, TokenType> keywords = {{"if", TokenType::KEYWORD_IF},
                                                       {"else", TokenType::KEYWORD_ELSE},
                                                       {"for", TokenType::KEYWORD_FOR},
                                                       {"return", TokenType::KEYWORD_RETURN},
                                                       {"while", TokenType::KEYWORD_WHILE},
                                                       {"int64", TokenType::KEYWORD_TYPE_INT64},
                                                       {"int32", TokenType::KEYWORD_TYPE_INT32},
                                                       {"int16", TokenType::KEYWORD_TYPE_INT16},
                                                       {"int8", TokenType::KEYWORD_TYPE_INT8},

                                                       {"uint64", TokenType::KEYWORD_TYPE_UINT64},
                                                       {"uint32", TokenType::KEYWORD_TYPE_UINT32},
                                                       {"uint16", TokenType::KEYWORD_TYPE_UINT16},
                                                       {"uint8", TokenType::KEYWORD_TYPE_UINT8},

                                                       {"float64", TokenType::KEYWORD_TYPE_FLOAT64},
                                                       {"float32", TokenType::KEYWORD_TYPE_FLOAT32},
                                                       {"void", TokenType::KEYWORD_TYPE_VOID}};

Token Tokenizer::identifier() {
    while ((isalnum(peek()) || peek() == '_') && !isAtEnd())
        advance();

    std::string lex = src.substr(start, current - start);
    auto it = keywords.find(lex);

    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

    return Token(lex, line, column, type);
}

char Tokenizer::advance() {
    column++;
    return src[current++];
}

char Tokenizer::peek() const { return isAtEnd() ? '\0' : src[current]; }

char Tokenizer::peekNext() const { return (current + 1 >= src.size()) ? '\0' : src[current + 1]; }

Token Tokenizer::makeToken(TokenType type) { return Token(src.substr(start, current - start), line, column, type); }

void Tokenizer::skipWhiteSpaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        switch (c) {
        case ' ':
        case '\r':
        case '\t': advance(); break;

        case '\n':
            advance();
            line++;
            column = 1;
            break;

        case '/':
            if (peekNext() == '/') {
                while (!isAtEnd() && peek() != '\n')
                    advance();
            } else {
                return;
            }

            break;

        default: start = current; return;
        }
    }
}
