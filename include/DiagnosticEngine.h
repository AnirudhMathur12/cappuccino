#ifndef DIAGNOSTICENGINE_H_
#define DIAGNOSTICENGINE_H_

#include <string>
#include <vector>

enum class DiagnosticLevel { ERROR, WARNING, NOTE };

struct DiagnosticMessage {
    DiagnosticLevel dl;
    std::string error_message;
    int col, row;

  public:
    DiagnosticMessage(DiagnosticLevel p_dl, const std::string& p_error, int p_col, int p_row);
};

class DiagnosticEngine {
    std::vector<DiagnosticMessage> diagnostics;

  public:
    void report(DiagnosticLevel p_dl, const std::string& p_error, int p_col, int p_row);
    bool hasErrors();
    void printDiagnostics();
};
#endif
