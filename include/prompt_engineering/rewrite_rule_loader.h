/**
 * @file rewrite_rule_loader.h
 * @brief YAML-based rewrite rule loader interface (Phase 2 design).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 YAML loading (Q4 2026)
 *
 * Loads and validates rewrite rules from YAML configuration files.
 * Enforces constraint that only lexical rules (regex, dictionary) can be
 * loaded from YAML; semantic and policy rules must be registered programmatically.
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "prompt_engineering/rewrite_engine.h"

namespace themis {
namespace prompt_engineering {

/**
 * @class RewriteRuleLoader
 * @brief Loads and validates rewrite rules from YAML configuration.
 *
 * YAML rules are restricted to:
 * - RegexRewriteRule (type: regex)
 * - DictionaryRewriteRule (type: dictionary)
 *
 * Thread-safe for loading and rule creation, but not for concurrent modifications.
 */
class RewriteRuleLoader {
public:
    RewriteRuleLoader();
    ~RewriteRuleLoader();

    /**
     * @brief Load rules from YAML file.
     *
     * @param yaml_path Path to YAML configuration file
     * @param[out] rules Vector of loaded rules (appended to, not cleared)
     * @return true if load succeeded, false if parse/validation error
     *
     * On failure, no rules are added to the output vector (all-or-nothing semantics).
     * Errors are logged but not thrown.
     */
    bool load_rules_from_yaml(
        const std::string& yaml_path,
        std::vector<std::shared_ptr<IRewriteRule>>& rules
    );

    /**
     * @brief Get last error message from load attempt.
     *
     * @return Error message (empty if last load succeeded)
     */
    std::string last_error() const;

    /**
     * @brief Validate YAML rule definition before loading.
     *
     * Checks structure, required fields, regex compilation, etc.
     *
     * @param yaml_content YAML text to validate
     * @return true if structure is valid, false otherwise
     */
    bool validate_yaml_rules(const std::string& yaml_content);

private:
    std::string last_error_;

    // Internal helper to parse and validate a single rule definition
    std::shared_ptr<IRewriteRule> parse_rule_definition(
        const std::string& rule_id,
        const std::string& rule_type,
        const std::string& phase_str,
        uint8_t priority,
        const std::string& description,
        const nlohmann::json& rule_config
    );
};

} // namespace prompt_engineering
} // namespace themis
