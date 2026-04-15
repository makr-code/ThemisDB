/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            password_policy.cpp                                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:16:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     239                                            ║
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

#include "auth/password_policy.h"

#include <algorithm>
#include <cmath>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

PasswordPolicy::PasswordPolicy()
    : config_() {}

PasswordPolicy::PasswordPolicy(const Config& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------

PasswordPolicy::ValidationResult PasswordPolicy::validate(const std::string& password) const {
    ValidationResult result;

    // --- Length checks -----------------------------------------------------
    if (password.size() < config_.min_length) {
        std::ostringstream msg;
        msg << "Password must be at least " << config_.min_length << " characters long";
        result.violations.push_back(msg.str());
    }

    if (config_.max_length > 0 && password.size() > config_.max_length) {
        std::ostringstream msg;
        msg << "Password must not exceed " << config_.max_length << " characters";
        result.violations.push_back(msg.str());
    }

    // --- Character class checks --------------------------------------------
    if (config_.require_uppercase) {
        bool found = std::any_of(password.begin(), password.end(),
                                 [](unsigned char c) { return std::isupper(c); });
        if (!found) {
            result.violations.push_back(
                "Password must contain at least one uppercase letter (A-Z)");
        }
    }

    if (config_.require_lowercase) {
        bool found = std::any_of(password.begin(), password.end(),
                                 [](unsigned char c) { return std::islower(c); });
        if (!found) {
            result.violations.push_back(
                "Password must contain at least one lowercase letter (a-z)");
        }
    }

    if (config_.require_digit) {
        bool found = std::any_of(password.begin(), password.end(),
                                 [](unsigned char c) { return std::isdigit(c); });
        if (!found) {
            result.violations.push_back("Password must contain at least one digit (0-9)");
        }
    }

    if (config_.require_special && !config_.special_chars.empty()) {
        bool found = std::any_of(password.begin(), password.end(),
                                 [&](char c) {
                                     return config_.special_chars.find(c) != std::string::npos;
                                 });
        if (!found) {
            result.violations.push_back(
                "Password must contain at least one special character");
        }
    }

    // --- Unique character check -------------------------------------------
    if (config_.min_unique_chars > 0) {
        std::unordered_set<char> unique_chars(password.begin(), password.end());
        if (unique_chars.size() < config_.min_unique_chars) {
            std::ostringstream msg;
            msg << "Password must contain at least " << config_.min_unique_chars
                << " distinct characters";
            result.violations.push_back(msg.str());
        }
    }

    // --- Consecutive identical character check ----------------------------
    if (config_.max_consecutive_identical > 0 && !password.empty()) {
        size_t run = 1;
        for (size_t i = 1; i < password.size(); ++i) {
            if (password[i] == password[i - 1]) {
                ++run;
                if (run > config_.max_consecutive_identical) {
                    std::ostringstream msg;
                    msg << "Password must not contain more than "
                        << config_.max_consecutive_identical
                        << " consecutive identical characters";
                    result.violations.push_back(msg.str());
                    break;
                }
            } else {
                run = 1;
            }
        }
    }

    // --- Entropy check ----------------------------------------------------
    if (config_.min_entropy_bits > 0.0) {
        double entropy = computeEntropy(password);
        if (entropy < config_.min_entropy_bits) {
            std::ostringstream msg;
            msg << "Password entropy (" << static_cast<int>(entropy)
                << " bits) is below the required minimum of "
                << static_cast<int>(config_.min_entropy_bits) << " bits";
            result.violations.push_back(msg.str());
        }
    }

    // --- Forbidden pattern checks -----------------------------------------
    for (const auto& pattern : config_.forbidden_patterns) {
        try {
            std::regex re(pattern, std::regex_constants::icase |
                                   std::regex_constants::ECMAScript);
            if (std::regex_search(password, re)) {
                result.violations.push_back(
                    "Password contains a forbidden pattern");
            }
        } catch (const std::regex_error&) {
            // Malformed pattern — skip silently to avoid crashing callers
        }
    }

    result.valid = result.violations.empty();
    return result;
}

// ---------------------------------------------------------------------------
// isCompliant
// ---------------------------------------------------------------------------

bool PasswordPolicy::isCompliant(const std::string& password) const {
    return validate(password).valid;
}

// ---------------------------------------------------------------------------
// computeEntropy
// ---------------------------------------------------------------------------

double PasswordPolicy::computeEntropy(const std::string& password) {
    if (password.empty()) return 0.0;

    std::unordered_map<char, int> freq;
    for (char c : password) ++freq[c];

    const double len = static_cast<double>(password.size());
    double h = 0.0;
    for (const auto& kv : freq) {
        double p = kv.second / len;
        h -= p * std::log2(p);
    }
    return h * len; // total bits
}

// ---------------------------------------------------------------------------
// Preset factories
// ---------------------------------------------------------------------------

PasswordPolicy PasswordPolicy::nistGuidelines() {
    Config cfg;
    cfg.min_length = 8;
    cfg.max_length = 0;          // NIST does not impose a maximum
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_unique_chars = 0;
    cfg.max_consecutive_identical = 0;
    return PasswordPolicy(cfg);
}

PasswordPolicy PasswordPolicy::strict() {
    Config cfg;
    cfg.min_length = 16;
    cfg.max_length = 128;
    cfg.require_uppercase = true;
    cfg.require_lowercase = true;
    cfg.require_digit = true;
    cfg.require_special = true;
    cfg.min_unique_chars = 8;
    cfg.max_consecutive_identical = 2;
    cfg.min_entropy_bits = 60.0;
    return PasswordPolicy(cfg);
}

PasswordPolicy PasswordPolicy::basic() {
    Config cfg;
    cfg.min_length = 8;
    cfg.max_length = 64;
    cfg.require_uppercase = true;
    cfg.require_lowercase = true;
    cfg.require_digit = true;
    cfg.require_special = false;
    cfg.min_unique_chars = 0;
    cfg.max_consecutive_identical = 0;
    return PasswordPolicy(cfg);
}

} // namespace auth
} // namespace themis
