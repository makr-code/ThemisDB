/**
 * @file importer_common.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
 * @brief Stream Read Line.
 * @param[in,out] file Input/output parameter.
 * @param[in,out] line Input/output parameter.
 * @param[in] max_bytes Input parameter.
 * @param[in,out] truncated Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: clear(), std::getline(), get().
 */
inline bool streamReadLine(std::istream& file,
                           std::string& line,
                           size_t max_bytes,
                           bool& truncated) {
    truncated = false;
    line.clear();

    if (max_bytes == 0) {
        if (!std::getline(file, line)) {
          return false;
        }
        return true;
    }

    char c = '\0';
    size_t count = 0;
    bool got_any = false;

    while (file.get(c)) {
        got_any = true;
        if (c == '\n') {
          break;
        }

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
 * @brief To Lower.
 * @param[in] s Input parameter.
 * @return Return value.
 * @details Calls: std::tolower().
 */
inline std::string toLower(const std::string& s) {
    std::string result = s;
    for (auto& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

} // namespace themis::importers
