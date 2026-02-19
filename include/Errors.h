#ifndef CAPPUCCINO_ERRORS_H
#define CAPPUCCINO_ERRORS_H

#include "Token.h"
#include <stdexcept>
#include <string>
#include <vector>

class CompilerError : public std::runtime_error {
  public:
    int line;
    int col;
    std::string phase;

    CompilerError(const std::string &phase, int line, int col, const std::string &msg);
    CompilerError(const std::string &phase, const Token &tok, const std::string &msg);
    CompilerError(const std::string &phase, const std::string &msg);

  private:
    static std::string format(const std::string &phase, int line, int col, const std::string &msg);
};

class LexError : public CompilerError {
  public:
    LexError(int line, int col, uint32_t codepoint);

  private:
    static std::string to_unicode(uint32_t codepoint);
};

class ParseError : public CompilerError {
  public:
    const Token token;
    ParseError(const Token &tok, const std::string &msg);
};

class TypeError : public CompilerError {
  public:
    TypeError(const std::string &msg);
};

class SemanticError : public CompilerError {
  public:
    SemanticError(const std::string &msg);
    SemanticError(const Token &tok, const std::string &msg);
};

class ErrorReporter {
  public:
    static void report(const std::exception &error);
    static bool hasErrors();
    static void printErrors();
    static void clear();

  private:
    static std::vector<std::string> errors;
};

#endif // CAPPUCCINO_ERRORS_H
