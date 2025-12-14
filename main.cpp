#include <iostream>

#include "AbstractSyntaxTree.h"
#include "CodeGen.h"
#include "Parser.h"
#include "Token.h"
#include "utils.h"

bool show_tokens = false, show_ast = false;
std::string output_name = "a.out";

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <file.capp> [-o <output_binary>] [--tokens] [--ast]"
                  << std::endl;
        return 1;
    }

    std::vector<std::string> source_files;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            show_tokens = true;
        } else if (arg == "--ast") {
            show_ast = true;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_name = argv[++i];
            } else {
                std::cerr << "Error: -o requires a filename argument."
                          << std::endl;
                return 1;
            }
        } else {
            source_files.push_back(arg);
        }
    }

    if (source_files.empty()) {
        std::cerr << "Error: No input files provided." << std::endl;
        return 1;
    }

    std::string source_path = source_files[0];

    if (source_path.length() < 5 ||
        source_path.substr(source_path.length() - 5) != ".capp") {
        std::cerr << "Error: Input file must have a .capp extension."
                  << std::endl;
        return 1;
    }

    std::optional<std::string> file_content = read_file(source_path);

    if (!file_content.has_value()) {
        return 1;
    }

    Tokenizer t(file_content.value());
    std::vector<Token> tokens = t.tokenize();

    if (show_tokens) {
        for (Token &token : tokens) {
            std::cout << token << std::endl;
        }
    }

    Parser p(tokens);
    Program prog = p.parse();

    if (show_ast)
        prog.dump();

    std::ofstream asmFile("output.s");
    if (!asmFile.is_open()) {
        std::cerr << "Failed to write assembly file." << std::endl;
        return 1;
    }

    CodeGen generator(prog, asmFile);
    generator.generate();
    asmFile.close();

    std::string cmd = "clang -o " + output_name + " output.s";

    std::cout << "Compiling binary..." << std::endl;
    int ret = std::system(cmd.c_str());

    if (ret == 0) {
        std::cout << "Build successful: ./" << output_name << std::endl;
        std::remove("output.s");
    } else {
        std::cerr << "Build failed. Check clang errors above." << std::endl;
        return 1;
    }

    return 0;
}
