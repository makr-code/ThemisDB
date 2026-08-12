/**
 * @file cli_parser_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>

/// @file cli_parser_utils.h
/// @brief Shared CLI argument-parsing utilities for all ThemisDB command-line tools.
///
/// Provides:
///   - is_help_flag    — recognises --help / -h / /?
///   - is_version_flag — recognises --version / -v
///   - consume_next_value    — pulls the next token from argc/argv
///   - consume_next_argument — pulls the next token from a std::vector<std::string>
///
/// All functions are header-only (constexpr / inline) and have no external dependencies.

namespace themis::cli {

/// Returns true for --help, -h, or /? (Windows-style help alias).
[[nodiscard]] constexpr bool is_help_flag(const std::string_view arg) noexcept {
    return arg == "--help" || arg == "-h" || arg == "/?";
}

/// Returns true for --version or -v.
[[nodiscard]] constexpr bool is_version_flag(const std::string_view arg) noexcept {
    return arg == "--version" || arg == "-v";
}

/// Advances @p index by one and stores the next argv token in @p value.
/// If no next token exists, sets @p error_message and returns false.
/// Use with argc/argv-style parsers (main.cpp, main_server.cpp, themis_model_cli.cpp).
[[nodiscard]] inline bool consume_next_value(int argc,
                                             char* const* argv,
                                             int& index,
                                             const std::string_view option,
                                             std::string& value,
                                             std::string& error_message) {
    if (index + 1 >= argc) {
        error_message = "Missing value for option " + std::string(option);
        return false;
    }
    value = argv[++index];
    return true;
}

/// Advances @p index by one and stores the next element of @p args in @p value.
/// If no next element exists, sets @p error_message and returns false.
/// Use with std::vector<std::string>-style parsers (themisctl.cpp).
[[nodiscard]] inline bool consume_next_argument(const std::vector<std::string>& args,
                                                size_t& index,
                                                const std::string_view option,
                                                std::string& value,
                                                std::string& error_message) {
    if (index + 1 >= args.size()) {
        error_message = "Missing value for option " + std::string(option);
        return false;
    }
    value = args[++index];
    return true;
}

} // namespace themis::cli
