#include <iostream>

#include "Token.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }
    std::cout << "Hi!\n";

    std::optional<std::string> file_content = read_file(argv[1]);
    if (!file_content.has_value()) {
        return 1;
    }

    Tokenizer t(file_content.value());
    std::vector<Token> tokens = t.tokenize();


    for (Token& token : tokens) {
        std::cout << token << std::endl;
    }

    return 0;
}
