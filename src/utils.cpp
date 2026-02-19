//
// Created by Anirudh Mathur on 12/11/25.
//

#include "utils.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

std::optional<std::string> read_file(const std::string &file_name) {
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

std::pair<uint32_t, int> decode_utf8(const std::string &src, size_t pos) {
    unsigned char c = src[pos];

    if (c < 0x80) // Single byte (ASCII)
        return {c, 1};
    else if ((c & 0xE0) == 0xC0) // Two bytes
        return {((c & 0x1F) << 6) | (src[pos + 1] & 0x3F), 2};
    else if ((c & 0xF0) == 0xE0) // Three bytes
        return {((c & 0x0F) << 12) | ((src[pos + 1] & 0x3F) << 6) | (src[pos + 2] & 0x3F), 3};
    else if ((c & 0xF8) == 0xF0) // Four bytes
        return {((c & 0x07) << 18) | ((src[pos + 1] & 0x3F) << 12) | ((src[pos + 2] & 0x3F) << 6) | (src[pos + 3] & 0x3F), 4};

    return {c, 1}; // Invalid byte, treat as single
}
