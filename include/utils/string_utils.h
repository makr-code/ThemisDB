/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            string_utils.h                                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <algorithm>
#include <cctype>

namespace themis {
namespace utils {

/**
 * @brief Case-insensitive string comparison utilities
 */

/**
 * @brief Check if a string contains a substring (case-insensitive)
 * @param str The string to search in
 * @param substr The substring to search for
 * @return true if substr is found in str (case-insensitive), false otherwise
 */
inline bool containsCaseInsensitive(const std::string& str, const std::string& substr) {
    auto it = std::search(
        str.begin(), str.end(),
        substr.begin(), substr.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return it != str.end();
}

/**
 * @brief Convert string to lowercase
 * @param str Input string
 * @return Lowercase version of the string
 */
inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

/**
 * @brief Compare two strings (case-insensitive)
 * @param str1 First string
 * @param str2 Second string
 * @return true if strings are equal (case-insensitive), false otherwise
 */
inline bool equalsCaseInsensitive(const std::string& str1, const std::string& str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    return std::equal(
        str1.begin(), str1.end(),
        str2.begin(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
}

} // namespace utils
} // namespace themis
