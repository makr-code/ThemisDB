/**
 * @file importer_common.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Shared utilities for importer implementations
 */
#pragma once

#include <string>
#include <istream>
#include <cctype>
#include <algorithm>

namespace themis::importers {

/**
 * @brief Read next line from stream with optional hard cap on bytes per line.
 *
 * Reads the next newline-terminated line from @p file with a hard per-line
 * byte cap of @p max_bytes (0 = unlimited). When the cap is exceeded the
 * remaining bytes of the current line are discarded and @p truncated is set
 * to true. Returns false only when EOF is reached before any bytes are read.
 *
 * @param file       Input stream to read from
 * @param line       Output buffer for the line (cleared at entry)
 * @param max_bytes  Maximum bytes per line (0 = unlimited)
 * @param truncated  Output flag: set to true if line was truncated
 * @return false only at EOF with no bytes read
 */
inline bool streamReadLine(std::istream& file,
                           std::string& line,
                           size_t max_bytes,
                           bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        if (!std::getline(file, line)) return false;
        return true;
    }

    char c = '\0';
    size_t count = 0;
    bool got_any = false;

    while (file.get(c)) {
        got_any = true;
        if (c == '\n') break;

        if (count < max_bytes) {
            line += c;
            ++count;
        } else {
            truncated = true;
            while (file.get(c) && c != '\n') { /* discard */ }
            break;
        }
    }

    return got_any;
}

/**
 * @brief Convert a string to lower-case (ASCII only).
 *
 * @param s Input string
 * @return Lower-case copy of input
 */
inline std::string toLower(const std::string& s) {
    std::string result = s;
    for (auto& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

} // namespace themis::importers
