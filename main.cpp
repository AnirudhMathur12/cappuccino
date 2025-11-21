#include <iostream>

#include "AbstractSyntaxTree.h"
#include "AssemblyEmitterARM.h"
#include "Parser.h"
#include "Token.h"
#include "utils.h"

bool show_tokens = false, show_ast = false;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file> <args>" << std::endl;
        return 1;
    }

    std::vector<std::string> files;

    for (int i = 1; i < argc; i++) {
        std::string arg = static_cast<std::string>(argv[i]);
        if (arg.starts_with("--")) {
            if (arg == "--tokens") {
                show_tokens = true;
            } else if (arg == "--ast") {
                show_ast = true;
            }
        } else {

            std::optional<std::string> file_content = read_file(arg);

            if (!file_content.has_value())
                return 1;
            files.push_back(file_content.value());
        }
    }

    for (auto s : files) {

        Tokenizer t(s);
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

        for (const auto &[k, v] : prog.var_offset_lookup) {
            std::cout << k << ":" << v << std::endl;
        }

        AssemblyEmitterARM asmarm("main.capp", prog.var_offset_lookup,
                                  prog.stack_size);
        asmarm.emitProgram(prog);
        asmarm.output_file.close();

        system("as -o main.o main.s");
        system("ld -o main main.o -e _main -arch arm64 -lSystem -syslibroot "
               "`xcrun -sdk "
               "macosx --show-sdk-path`");
    }

    return 0;
}
