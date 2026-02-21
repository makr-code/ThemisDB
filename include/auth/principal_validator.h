/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            principal_validator.h                              ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     245                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <regex>
#include <optional>
#include <memory>
#include <chrono>

namespace themis {
namespace auth {

/**
 * @brief Principal Validator with whitelist/blacklist and regex rules
 * 
 * Security Feature: Validates principal names (Kerberos, JWT subject, etc.)
 * against configurable rules to prevent unauthorized access.
 * 
 * Features:
 * - Whitelist: explicitly allowed principals (exact match or regex)
 * - Blacklist: explicitly denied principals (takes precedence)
 * - Regex validation rules for principal format
 * - Role mapping with priority rules
 * - Audit logging for all validation decisions
 * 
 * Use cases:
 * - Restrict access to specific realms/domains
 * - Block known compromised principals
 * - Enforce principal naming conventions
 * - Map principals to roles based on patterns
 * 
 * P1 (High Priority) security hardening feature.
 */
class PrincipalValidator {
public:
    /**
     * @brief Validation rule types
     */
    enum class RuleType {
        WHITELIST,      // Explicitly allow
        BLACKLIST,      // Explicitly deny
        REGEX_MATCH,    // Must match regex
        REGEX_DENY      // Must not match regex
    };
    
    /**
     * @brief Validation rule
     */
    struct Rule {
        RuleType type;
        std::string pattern;        // Exact match or regex pattern
        bool is_regex = false;      // True if pattern is regex
        int priority = 0;           // Higher priority rules evaluated first
        std::string description;    // Human-readable rule description
        
        // Compiled regex (if is_regex = true)
        mutable std::optional<std::regex> compiled_regex;
    };
    
    /**
     * @brief Principal mapping rule (principal pattern -> role)
     */
    struct MappingRule {
        std::string principal_pattern;  // Exact match or regex
        bool is_regex = false;
        std::vector<std::string> roles; // Roles to assign
        int priority = 0;               // Higher priority evaluated first
        
        mutable std::optional<std::regex> compiled_regex;
    };
    
    /**
     * @brief Validation result
     */
    struct ValidationResult {
        bool allowed = false;
        std::string principal;
        std::vector<std::string> roles;
        std::string denial_reason;      // If not allowed, why?
        std::string matched_rule;       // Which rule matched
    };
    
    struct Config {
        // Default action if no rules match
        bool default_allow = false;
        
        // Enable audit logging for all validation decisions
        bool enable_audit_logging = true;
        
        // Case-sensitive matching
        bool case_sensitive = true;
        
        // Validation rules (evaluated in priority order)
        std::vector<Rule> rules;
        
        // Principal-to-role mapping rules
        std::vector<MappingRule> mapping_rules;
    };
    
    explicit PrincipalValidator(const Config& config = Config());
    
    /**
     * @brief Validate a principal name
     * 
     * Checks against whitelist/blacklist and validation rules.
     * Maps to roles if validation succeeds.
     * Logs audit trail of decision.
     * 
     * @param principal Principal name to validate (e.g., "alice@REALM.COM")
     * @return ValidationResult with allow/deny decision and roles
     */
    ValidationResult validate(const std::string& principal);
    
    /**
     * @brief Add a validation rule
     * 
     * @param rule Rule to add
     */
    void addRule(const Rule& rule);
    
    /**
     * @brief Add a mapping rule
     * 
     * @param rule Mapping rule to add
     */
    void addMappingRule(const MappingRule& rule);
    
    /**
     * @brief Remove all rules of a specific type
     * 
     * @param type Rule type to remove
     */
    void clearRules(RuleType type);
    
    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Get validation statistics
     */
    struct Statistics {
        uint64_t total_validations = 0;
        uint64_t allowed = 0;
        uint64_t denied = 0;
        uint64_t blacklisted = 0;
        uint64_t whitelisted = 0;
        uint64_t default_allow = 0;
        uint64_t default_deny = 0;
    };
    
    Statistics getStatistics() const;

private:
    Config config_;
    mutable Statistics stats_;
    
    // Check if principal matches a rule
    bool matchesRule(const std::string& principal, const Rule& rule) const;
    
    // Check if principal matches a mapping rule
    bool matchesMappingRule(const std::string& principal, const MappingRule& rule) const;
    
    // Apply mapping rules to get roles
    std::vector<std::string> applyMappingRules(const std::string& principal) const;
    
    // Log audit trail
    void logAudit(const ValidationResult& result) const;
    
    // Compile regex for a rule
    void compileRegex(const Rule& rule) const;
    void compileRegex(const MappingRule& rule) const;
};

/**
 * @brief Pre-configured principal validators for common scenarios
 */
class PrincipalValidatorPresets {
public:
    /**
     * @brief Create validator that only allows specific realm
     * 
     * @param realm Kerberos realm (e.g., "EXAMPLE.COM")
     * @return Configured validator
     */
    static PrincipalValidator realmRestricted(const std::string& realm);
    
    /**
     * @brief Create validator that blocks specific principals
     * 
     * @param blocked_principals List of principals to block
     * @return Configured validator
     */
    static PrincipalValidator withBlacklist(const std::vector<std::string>& blocked_principals);
    
    /**
     * @brief Create validator that only allows specific principals
     * 
     * @param allowed_principals List of allowed principals
     * @return Configured validator
     */
    static PrincipalValidator withWhitelist(const std::vector<std::string>& allowed_principals);
    
    /**
     * @brief Create validator with standard enterprise rules
     * 
     * - Requires realm suffix
     * - Blocks service accounts from interactive login
     * - Enforces naming conventions
     * 
     * @param realm Primary realm
     * @return Configured validator
     */
    static PrincipalValidator enterpriseStandard(const std::string& realm);
};

} // namespace auth
} // namespace themis
