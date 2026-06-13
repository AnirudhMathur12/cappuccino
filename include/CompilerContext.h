#ifndef COMPILERCONTEXT_H_
#define COMPILERCONTEXT_H_

#include "ArenaAllocator.h"

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

struct CompilerOptions {
    // Input and Output
    std::vector<std::string> source_files;
    std::string output_name = "a.out";

    // Halting execution
    bool stop_at_tokens = false;
    bool stop_at_ast = false;
    bool emit_assembly_only = false;
    bool compile_only = false;

    // Debugging and Dumps
    bool show_tokens = false;
    bool show_ast = false;
    /* bool dump_ir = false; // tentative */

    // Diagnostics and Strictness
    bool warnings_as_errors = false;
    bool quiet_mode = false;

    /*
    // Target Architecture and Linking (tentative)
    std::string target_arch = "arm64-apple-darwin";
    std::vector<std::string> library_paths;
    */

    /*
    // Optimization (tentative)
    int optimization_level = 0; // 0, 1, 2, 3
    */
};

class CompilerContext {
  public:
    CompilerOptions options;
    DiagnosticEngine de;
    ArenaAllocator aa;
};

#endif
