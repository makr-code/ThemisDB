/*
 * ThemisDB | File: file_utils.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 32
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #785 Implement mTLS for secure s... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace utils
} // namespace themis
