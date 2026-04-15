/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            password_policy.h                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:09:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 3487250b75  2026-02-28  Add min_entropy_bits to PasswordPolicy for configurable e... ║
    • 167d3943a9  2026-02-24  fix(auth): audit — remove regex from public header, fix d... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace auth {

/**
 * @brief Configurable password policy enforcement
 *
 * Validates passwords against a configurable set of rules, returning
 * a detailed list of violations. Can be used at registration,
 * password-change, and reset flows.
 *
 * Features:
 * - Minimum / maximum length
 * - Required character classes (uppercase, lowercase, digit, special)
 * - Configurable set of allowed special characters
 * - Minimum number of unique characters (entropy floor)
 * - Maximum consecutive identical characters
 * - Forbidden substring / regex patterns (e.g. username, common words)
 *
 * Preset factories following well-known security guidelines:
 * - PasswordPolicy::nistGuidelines() — NIST SP 800-63B
 * - PasswordPolicy::strict()         — High-assurance enterprise
 * - PasswordPolicy::basic()          — Minimal / legacy compatibility
 */
class PasswordPolicy {
public:
    /**
     * @brief Policy configuration
     *
     * All fields have safe defaults that meet general enterprise
     * password requirements.
     */
    struct Config {
        /// Minimum required password length (inclusive)
        size_t min_length = 12;

        /// Maximum allowed password length (inclusive, 0 = unlimited)
        size_t max_length = 128;

        /// Require at least one ASCII uppercase letter (A-Z)
        bool require_uppercase = true;

        /// Require at least one ASCII lowercase letter (a-z)
        bool require_lowercase = true;

        /// Require at least one ASCII digit (0-9)
        bool require_digit = true;

        /// Require at least one character from special_chars
        bool require_special = true;

        /// Characters considered "special". Only relevant when require_special = true
        std::string special_chars = "!@#$%^&*()_+-=[]{}|;':\",./<>?";

        /// Minimum number of distinct characters (0 = no check)
        size_t min_unique_chars = 0;

        /// Maximum run of identical consecutive characters (0 = no check)
        size_t max_consecutive_identical = 0;

        /**
         * @brief Minimum Shannon entropy of the password in bits (0 = no check).
         *
         * Computed as H(password) = -sum(p_i * log2(p_i)) * length, where
         * p_i is the relative frequency of each distinct character. This
         * provides an entropy floor independent of character-class rules,
         * catching repetitive patterns like "aaabbbccc" that pass complexity
         * checks but have low randomness.
         *
         * Typical values: 40 bits (moderate), 60 bits (high assurance).
         */
        double min_entropy_bits = 0.0;

        /**
         * @brief Regex patterns that must NOT appear in the password.
         *
         * Each entry is a case-insensitive ECMAScript regex. Useful to
         * block obvious strings such as the user's login name, "password",
         * month names, etc.
         */
        std::vector<std::string> forbidden_patterns;
    };

    /**
     * @brief Result of a policy validation check
     */
    struct ValidationResult {
        /// True when the password satisfies every active policy rule
        bool valid = false;

        /**
         * @brief Human-readable violation messages (one per failed rule).
         *
         * Empty when valid == true.
         */
        std::vector<std::string> violations;

        /// Convenience conversion so results can be used in boolean context
        explicit operator bool() const { return valid; }
    };

    /**
     * @brief Construct with default configuration
     */
    PasswordPolicy();

    /**
     * @brief Construct with explicit configuration
     * @param config Policy rules to enforce
     */
    explicit PasswordPolicy(const Config& config);

    /**
     * @brief Validate a plaintext password against the configured policy
     *
     * @param password The password candidate to validate
     * @return ValidationResult containing pass/fail and any violation messages
     */
    ValidationResult validate(const std::string& password) const;

    /**
     * @brief Quick compliance check (no details)
     * @param password The password candidate
     * @return true if the password satisfies every active rule
     */
    bool isCompliant(const std::string& password) const;

    /**
     * @brief Retrieve the current policy configuration
     */
    const Config& getConfig() const { return config_; }

    /**
     * @brief Replace the current policy configuration
     * @param config New policy rules
     */
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Compute the Shannon entropy of a password in bits
     *
     * H = -sum(p_i * log2(p_i)) * length, where p_i is the relative
     * frequency of each distinct character. Returns 0.0 for an empty
     * password.
     *
     * @param password The password candidate
     * @return Entropy in bits
     */
    static double computeEntropy(const std::string& password);

    // ---- Preset factories ------------------------------------------------

    /**
     * @brief NIST SP 800-63B guidelines
     *
     * Focuses on length (≥8 chars) and breach-corpus checks over
     * arbitrary complexity rules. Character-class requirements are
     * disabled; forbidden_patterns should be populated with known-bad
     * passwords by the application.
     */
    static PasswordPolicy nistGuidelines();

    /**
     * @brief Strict high-assurance enterprise policy
     *
     * ≥16 chars, all four character classes required, ≥8 unique chars,
     * max 2 consecutive identical chars.
     */
    static PasswordPolicy strict();

    /**
     * @brief Basic / legacy-compatible policy
     *
     * ≥8 chars, digit required, uppercase and lowercase required,
     * special character not required.
     */
    static PasswordPolicy basic();

private:
    Config config_;
};

} // namespace auth
} // namespace themis
