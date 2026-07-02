#ifndef THEMIS_SECURITY_SAFE_REGEX_H
#define THEMIS_SECURITY_SAFE_REGEX_H

#include <regex>
#include <string>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <unordered_map>

namespace themis::security {

/**
 * @brief Safe regular expression matching wrapper to prevent ReDoS (Regular Expression Denial of Service).
 *
 * ReDoS vulnerabilities occur when regex patterns with catastrophic backtracking are applied to
 * untrusted input, causing exponential time complexity and denial of service.
 *
 * **Key Safety Features:**
 * - Enforces regex matching timeout (default 5 seconds)
 * - Validates regex complexity before compilation
 * - Caches compiled regex patterns for performance
 * - Pre-validates input length before matching
 * - Detects known dangerous regex patterns
 *
 * **Dangerous Pattern Examples to Avoid:**
 * - `(a+)+` — nested quantifiers
 * - `(a|a)*` — alternation with overlap
 * - `(a|ab)*` — overlapping alternation
 * - `(a*)*` — nested Kleene star
 *
 * **Usage Pattern:**
 * ```cpp
 * // UNSAFE - Don't do this:
 * std::string user_pattern = get_user_input();
 * std::regex re(user_pattern);  // Untrusted pattern - ReDoS risk!
 * std::regex_search(text, re);
 *
 * // SAFE - Use SafeRegex:
 * SafeRegex safe_regex;
 * bool matches = safe_regex.match("^[a-z]+@[a-z]+\\.[a-z]{2,}$", text, 
 *                                  std::chrono::seconds(5));
 * ```
 *
 * **CWE References:**
 * - CWE-1333: Inefficient Regular Expression Complexity
 * - OWASP: Regular Expression Denial of Service (ReDoS)
 */
class SafeRegex {
public:
    /**
     * @brief Construct SafeRegex with optional timeout.
     * @param timeout_seconds Maximum seconds allowed for regex matching (default: 5).
     */
    explicit SafeRegex(size_t timeout_seconds = 5);

    ~SafeRegex();

    /**
     * @brief Check if a string matches a regex pattern with timeout protection.
     * @param pattern The regex pattern (should be hardcoded or validated).
     * @param text The text to match against.
     * @param timeout Optional custom timeout for this operation.
     * @return true if the pattern matches, false otherwise.
     * @throws std::runtime_error if timeout exceeded or pattern is invalid.
     * 
     * **Recommended: Only use this with hardcoded patterns, not user input.**
     */
    bool match(const std::string& pattern, const std::string& text,
               std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Search for first occurrence of pattern in text with timeout.
     * @param pattern The regex pattern (should be hardcoded or validated).
     * @param text The text to search in.
     * @param timeout Optional custom timeout for this operation.
     * @return true if pattern found, false otherwise.
     * @throws std::runtime_error if timeout exceeded or pattern is invalid.
     */
    bool search(const std::string& pattern, const std::string& text,
                std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Replace all occurrences of pattern with replacement.
     * @param pattern The regex pattern (should be hardcoded or validated).
     * @param text The text to search and replace in.
     * @param replacement The replacement string.
     * @param timeout Optional custom timeout for this operation.
     * @return String with all matches replaced.
     * @throws std::runtime_error if timeout exceeded or pattern is invalid.
     */
    std::string replace(const std::string& pattern, const std::string& text,
                        const std::string& replacement,
                        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Split string by regex pattern with timeout protection.
     * @param pattern The regex pattern for splitting (should be hardcoded).
     * @param text The text to split.
     * @param timeout Optional custom timeout for this operation.
     * @return Vector of split strings.
     * @throws std::runtime_error if timeout exceeded or pattern is invalid.
     */
    std::vector<std::string> split(const std::string& pattern, const std::string& text,
                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    /**
     * @brief Validate regex pattern for known ReDoS vulnerabilities.
     * @param pattern The pattern to validate.
     * @return true if pattern is considered safe, false if potentially dangerous.
     * 
     * **Checks for:**
     * - Nested quantifiers: (a+)+, (a*)+, etc.
     * - Overlapping alternation: (a|ab)*, (a|a)*
     * - Excessive alternation chains: (a|b|c|d|e|f|g|...)
     */
    static bool is_pattern_safe(const std::string& pattern);

    /**
     * @brief Pre-validate user input before regex matching.
     * @param text The text to validate.
     * @param max_length Maximum allowed length (default: 10KB).
     * @return true if text passes validation, false otherwise.
     * 
     * **Prevents:**
     * - Excessively long inputs
     * - Inputs with pathological backtracking patterns
     */
    static bool validate_input(const std::string& text, size_t max_length = 10240);

    /**
     * @brief Clear the compiled regex cache.
     */
    void clear_cache();

    /**
     * @brief Get cache statistics.
     * @return String with cache hit/miss statistics.
     */
    std::string cache_stats() const;

private:
    size_t default_timeout_seconds_;
    std::unordered_map<std::string, std::shared_ptr<std::regex>> pattern_cache_;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;

    /**
     * @brief Get or compile a regex pattern with caching.
     * @param pattern The pattern to compile.
     * @return Shared pointer to compiled regex.
     * @throws std::runtime_error if pattern is invalid.
     */
    std::shared_ptr<std::regex> get_compiled_pattern(const std::string& pattern);

    /**
     * @brief Check for nested quantifiers pattern.
     * @param pattern The pattern to check.
     * @return true if nested quantifiers detected.
     */
    static bool has_nested_quantifiers(const std::string& pattern);

    /**
     * @brief Check for overlapping alternation pattern.
     * @param pattern The pattern to check.
     * @return true if overlapping alternation detected.
     */
    static bool has_overlapping_alternation(const std::string& pattern);
};

}  // namespace themis::security

#endif  // THEMIS_SECURITY_SAFE_REGEX_H
