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
#include <string_view>
#include <algorithm>
#include <cctype>

namespace themis {
namespace utils {

/**
 * @brief Trim leading/trailing whitespace from a string.
 * @param s Input string.
 * @param ws Whitespace character set (default: " \t\r\n").
 * @return Trimmed string (returns empty if input is all-whitespace).
 */
inline std::string trim(const std::string& s, std::string_view ws = " \t\r\n") {
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

/**
 * @brief Trim std::string_view in-place without allocation.
 * @param sv Input string view.
 * @param ws Whitespace character set (default: " \t\r\n").
 * @return Trimmed view of original data (may be empty).
 */
inline std::string_view trim_view(std::string_view sv, std::string_view ws = " \t\r\n") {
    const auto b = sv.find_first_not_of(ws);
    if (b == std::string_view::npos) return {};
    const auto e = sv.find_last_not_of(ws);
    return sv.substr(b, e - b + 1);
}

/**
 * @brief Left-trim (remove leading whitespace).
 * @param s Input string.
 * @param ws Whitespace character set (default: " \t\r\n").
 * @return Left-trimmed string.
 */
inline std::string ltrim(const std::string& s, std::string_view ws = " \t\r\n") {
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    return s.substr(b);
}

/**
 * @brief Right-trim (remove trailing whitespace).
 * @param s Input string.
 * @param ws Whitespace character set (default: " \t\r\n").
 * @return Right-trimmed string.
 */
inline std::string rtrim(const std::string& s, std::string_view ws = " \t\r\n") {
    const auto e = s.find_last_not_of(ws);
    if (e == std::string::npos) return {};
    return s.substr(0, e + 1);
}

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
