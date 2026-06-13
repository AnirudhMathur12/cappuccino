#include "AbstractSyntaxTree.h"
#include "CodeGen.h"
#include "CompilerContext.h"
#include "DebugVisitor.h"
#include "Parser.h"
#include "Token.h"
#include "utils.h"
#include "version.h"

#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {

    CompilerContext ctx;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.capp> [-o <output_binary>] [--tokens] [--ast]"
                  << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            ctx.options.show_tokens = true;
        } else if (arg == "--till_tokens") {
            ctx.options.stop_at_tokens = true;
        } else if (arg == "--ast") {
            ctx.options.show_ast = true;
        } else if (arg == "--till_ast") {
            ctx.options.stop_at_ast = true;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "Cappuccino Compiler v" << VERSION_STRING << std::endl;
            std::cout << PROJECT_DESCRIPTION << std::endl;
            std::cout << "Author:   " << PROJECT_AUTHOR << std::endl;
            std::cout << "Homepage: " << PROJECT_HOMEPAGE << std::endl;
            return 0;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                ctx.options.output_name = argv[++i];
            } else {
                std::cerr << "Error: -o requires a filename argument." << std::endl;
                return 1;
            }
        } else {
            ctx.options.source_files.push_back(arg);
        }
    }

    if (ctx.options.source_files.empty()) {
        std::cerr << "Error: No input files provided." << std::endl;
        return 1;
    }

    std::string source_path = ctx.options.source_files[0];

    if (source_path.length() < 5 || source_path.substr(source_path.length() - 5) != ".capp") {
        std::cerr << "Error: Input file must have a .capp extension." << std::endl;
        return 1;
    }

    std::optional<std::string> file_content = read_file(source_path);

    if (!file_content.has_value()) {
        return 1;
    }

    Tokenizer t(file_content.value(), ctx);
    std::vector<Token> tokens = t.tokenize();

    if (ctx.de.hasErrors()) {
        ctx.de.printDiagnostics();
        return 1;
    }

    if (ctx.options.show_tokens) {
        for (Token& token : tokens)
            std::cout << token << std::endl;
    }

    if (ctx.options.stop_at_tokens)
        return 0;

    Parser p(tokens, ctx);
    Program prog = p.parse();

    if (ctx.de.hasErrors()) {
        ctx.de.printDiagnostics();
        return 1;
    }

    if (ctx.options.show_ast) {
        DebugVisitor debugger;
        std::cout << "Program\n";
        for (const auto& stmt : prog.statements) {
            stmt->accept(debugger);
        }
        if (ctx.options.stop_at_ast)
            return 0;
    }

    try {
        std::ofstream asmFile("output.s");
        if (!asmFile.is_open()) {
            std::cerr << "Failed to write assembly file." << std::endl;
            return 1;
        }
        CodeGen generator(prog, asmFile, ctx);
        generator.generate();
        asmFile.close();
    } catch (const std::exception& e) {
        std::cerr << "\n\033[1;31m[Internal Compiler Bug]\033[0m: " << e.what() << "\n";
        return 1;
    }

    if (ctx.de.hasErrors()) {
        ctx.de.printDiagnostics();
        return 1;
    }

    std::cout << "Assembling..." << std::endl;
    std::string as_cmd = "as -o output.o output.s";
    int as_ret = std::system(as_cmd.c_str());
    if (as_ret != 0) {
        std::cerr << "Assembly failed. Check assembler errors above." << std::endl;
        return 1;
    }

    std::cout << "Linking..." << std::endl;
    std::string ld_cmd = "ld -o " + ctx.options.output_name +
                         " output.o -lSystem -syslibroot `xcrun -sdk macosx "
                         "--show-sdk-path` -e  _main -arch arm64";
    int ld_ret = std::system(ld_cmd.c_str());
    if (ld_ret != 0) {
        std::cerr << "Linker failed. Check linker errors above." << std::endl;
        return 1;
    } else {
        std::cout << "Compilation successful." << std::endl;
        // std::remove("output.s");
        std::remove("output.o");
    }

    return 0;
}
