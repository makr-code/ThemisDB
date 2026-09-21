/**
 * @file file_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "utils/file_utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace utils {

std::string readFileContents(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer = {};
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace utils
} // namespace themis
