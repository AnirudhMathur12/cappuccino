//
// Created by Anirudh Mathur on 12/11/25.
//

#ifndef CAPPUCCINO_UTILS_H
#define CAPPUCCINO_UTILS_H

#include <optional>

std::optional<std::string> read_file(const std::string &file_name);

std::pair<uint32_t, int> decode_utf8(const std::string &src, size_t pos);

#endif // CAPPUCCINO_UTILS_H
