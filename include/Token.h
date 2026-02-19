//
// Created by Anirudh Mathur on 12/11/25.
//

#ifndef CAPPUCCINO_TOKEN_H
#define CAPPUCCINO_TOKEN_H
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using FormattedData = std::variant<std::monostate, uint64_t, double, std::string>;

enum class TokenType {
    // Literals
    LITERAL_STRING,
    LITERAL_FLOAT,
    LITERAL_INTEGER,
    IDENTIFIER,

    // Punctuation
    SEMICOLON,
    COMMA,
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_CURLY,
    RIGHT_CURLY,
    LEFT_SQUARE,
    RIGHT_SQUARE,

    // Control Flow
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_RETURN,
    KEYWORD_WHILE,

    // Data Types
    KEYWORD_TYPE_INT64,
    KEYWORD_TYPE_INT32,
    KEYWORD_TYPE_INT16,
    KEYWORD_TYPE_INT8,
    KEYWORD_TYPE_UINT64,
    KEYWORD_TYPE_UINT32,
    KEYWORD_TYPE_UINT16,
    KEYWORD_TYPE_UINT8,
    KEYWORD_TYPE_FLOAT64,
    KEYWORD_TYPE_FLOAT32,
    KEYWORD_TYPE_VOID,

    // Operators
    OPERATOR_EQUALITY,
    OPERATOR_MINUS,
    OPERATOR_PLUS,
    OPERATOR_ASTERISK,
    OPERATOR_ASSIGNMENT,
    OPERATOR_LESS,
    OPERATOR_LESS_EQUALS,
    OPERATOR_GREATER,
    OPERATOR_GREATER_EQUALS,
    OPERATOR_FORWARD_SLASH,
    OPERATOR_BACKSLASH,
    OPERATOR_AMPERSAND,

    // Logical
    EXCL_EQUAL,
    EXCLAMATION,

    // Meta
    TOKEN_EOF,
    UNDEFINED
};

inline std::string token_type_to_string(TokenType type);
inline std::string fd_to_string(const FormattedData &fd);

class Token {
  public:
    Token() = default;
    Token(const std::string &p_lexeme, int p_row, int p_col, TokenType p_type);

    friend std::ostream &operator<<(std::ostream &os, const Token &tok);

    std::string lexeme;
    int row, column;
    FormattedData fd;
    TokenType type;
};

std::ostream &operator<<(std::ostream &os, const Token &tok);

class Tokenizer {
  public:
    Tokenizer(std::string &p_src) : src(p_src) {};

    std::vector<Token> tokenize();

  private:
    std::string src;
    size_t start = 0;
    size_t current = 0;

    int line = 1;
    int column = 1;

    bool isAtEnd() const { return current >= src.length(); }
    char advance();
    char peek() const;
    char peekNext() const;

    void skipWhiteSpaceAndComments();
    Token makeToken(TokenType type);
    Token string();
    Token number();
    Token identifier();
};

#endif // CAPPUCCINO_TOKEN_H
//
