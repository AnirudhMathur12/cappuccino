#include "DiagnosticEngine.h"

#include <iostream>

DiagnosticMessage::DiagnosticMessage(DiagnosticLevel p_dl, const std::string& p_error, int p_col,
                                     int p_row)
    : dl(p_dl), error_message(std::move(p_error)), col(p_col), row(p_row) {}

void DiagnosticEngine::report(DiagnosticLevel p_dl, const std::string& p_error, int p_col,
                              int p_row) {
    diagnostics.emplace_back(p_dl, p_error, p_col, p_row);
}

bool DiagnosticEngine::hasErrors() {
    return !diagnostics.empty();
}

void DiagnosticEngine::printDiagnostics() {
    for (const auto& diag : diagnostics) {
        std::string level_str;
        std::string color_code;
        const std::string reset_code = "\033[0m";
        const std::string bold_white = "\033[1;37m";

        switch (diag.dl) {
        case DiagnosticLevel::ERROR:
            level_str = "error";
            color_code = "\033[1;31m"; // Bold Red
            break;
        case DiagnosticLevel::WARNING:
            level_str = "warning";
            color_code = "\033[1;35m"; // Bold Magenta
            break;
        case DiagnosticLevel::NOTE:
            level_str = "note";
            color_code = "\033[1;36m"; // Bold Cyan
            break;
        }

        // Format: row:col: error: message
        std::cerr << bold_white << diag.row << ":" << diag.col << ": " << reset_code << color_code
                  << level_str << ": " << reset_code << diag.error_message << "\n";
    }
}
