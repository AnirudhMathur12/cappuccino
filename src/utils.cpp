//
// Created by Anirudh Mathur on 12/11/25.
//

#include "utils.h"

#include <iostream>
#include <sstream>
#include <optional>

std::optional<std::string> read_file(const std::string& file_name) {
    std::ifstream file_stream;
    file_stream.open(file_name);

    if (!file_stream.is_open()) {
        std::cerr << "Can't open file: " << file_name << std::endl;
        return {};
    }

    std::stringstream ss;
    ss << file_stream.rdbuf();
    std::string content = ss.str();
    file_stream.close();
    return content;
}