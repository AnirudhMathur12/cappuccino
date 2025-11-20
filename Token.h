//
// Created by Anirudh Mathur on 12/11/25.
//

#ifndef CAPPUCCINO_TOKEN_H
#define CAPPUCCINO_TOKEN_H
#include <iostream>
#include <string>
#include <vector>

enum TokenType {
    LITERAL_STRING,
    LITERAL_FLOAT,
    LITERAL_INTEGER,
    IDENTIFIER,
    SEMICOLON,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_RETURN,
    KEYWORD_WHILE,
    KEYWORD_TYPE_INT,
    KEYWORD_TYPE_FLOAT,
    KEYWORD_TYPE_STRING,
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
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_CURLY,
    RIGHT_CURLY,
    EXCL_EQUAL,
    EXCLAMATION,
    TOKEN_EOF,
    COMMA,
    UNDEFINED
};

inline std::string token_type_to_string(TokenType type);
inline std::string
fd_to_string(const std::variant<std::monostate, int, float, std::string> &fd);

class Token {
  public:
    Token() = default;
    Token(const std::string &p_lexeme, int p_row, int p_col, TokenType p_type);

    friend std::ostream &operator<<(std::ostream &os, const Token &tok);

    std::string lexeme;
    int row, column;
    std::variant<std::monostate, int, float, std::string> fd;
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
