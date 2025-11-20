#include <iostream>

#include "AbstractSyntaxTree.h"
#include "Parser.h"
#include "Token.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file> <args>" << std::endl;
        return 1;
    }

    std::optional<std::string> file_content = read_file(argv[1]);

    if (!file_content.has_value()) {
        return 1;
    }

    Tokenizer t(file_content.value());
    std::vector<Token> tokens = t.tokenize();

    for (Token &token : tokens) {
        std::cout << token << std::endl;
    }

    Parser p(tokens);
    Program prog = p.parse();

    prog.dump();

    // for (const auto &[k, v] : prog.var_offset_lookup) {
    //     std::cout << k << ":" << v << std::endl;
    // }

    return 0;
}
