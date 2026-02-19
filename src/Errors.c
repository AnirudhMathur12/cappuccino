#include "Errors.h"
#include <iomanip>
#include <iostream>
#include <sstream>

CompilerError::CompilerError(const std::string &phase, int line, int col, const std::string &msg)
    : std::runtime_error(format(phase, line, col, msg)), line(line), col(col), phase(phase) {}

CompilerError::CompilerError(const std::string &phase, const Token &tok, const std::string &msg)
    : std::runtime_error(format(phase, tok.row, tok.column, msg)), line(tok.row), col(tok.column), phase(phase) {}

CompilerError::CompilerError(const std::string &phase, const std::string &msg)
    : std::runtime_error(format(phase, 0, 0, msg)), line(0), col(0), phase(phase) {}

std::string CompilerError::format(const std::string &phase, int line, int col, const std::string &msg) {
    std::ostringstream oss;
    // ANSI Red color for the error prefix
    oss << "\033[1;31m[" << phase << " Error]\033[0m ";
    if (line > 0) {
        oss << "at Line " << line << ", Col " << col << ": ";
    }
    oss << msg;
    return oss.str();
}

LexError::LexError(int line, int col, uint32_t codepoint)
    : CompilerError("Lexer", line, col, "Unexpected character U+" + to_unicode(codepoint)) {}

std::string LexError::to_unicode(uint32_t codepoint) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << codepoint;
    return oss.str();
}

ParseError::ParseError(const Token &tok, const std::string &msg) : CompilerError("Parser", tok, msg), token(tok) {}

TypeError::TypeError(const std::string &msg) : CompilerError("Type", msg) {}

SemanticError::SemanticError(const std::string &msg) : CompilerError("Semantic", msg) {}

SemanticError::SemanticError(const Token &tok, const std::string &msg) : CompilerError("Semantic", tok, msg) {}

// Define the static vector in exactly one translation unit
std::vector<std::string> ErrorReporter::errors;

void ErrorReporter::report(const std::exception &error) { errors.push_back(error.what()); }

bool ErrorReporter::hasErrors() { return !errors.empty(); }

void ErrorReporter::printErrors() {
    for (const auto &err : errors) {
        std::cerr << err << "\n";
    }
}

void ErrorReporter::clear() { errors.clear(); }
