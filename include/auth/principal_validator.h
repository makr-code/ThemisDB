/**
 * @file principal_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <regex>
#include <optional>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace themis {
// Forward declarations
namespace utils { class AuditLogger; }
class PolicyEngine;
namespace auth {

class PrincipalValidator {
public:
    enum class RuleType {
        WHITELIST,      // Explicitly allow
        BLACKLIST,      // Explicitly deny
        REGEX_MATCH,    // Must match regex
        REGEX_DENY      // Must not match regex
    };
    
    struct Rule {
        RuleType type;
        std::string pattern;        // Exact match or regex pattern
        bool is_regex = false;      // True if pattern is regex
        int priority = 0;           // Higher priority rules evaluated first
        std::string description;    // Human-readable rule description
        
        // Compiled regex (if is_regex = true)
        mutable std::optional<std::regex> compiled_regex;
    };
    
    struct MappingRule {
        std::string principal_pattern;  // Exact match or regex
        bool is_regex = false;
        std::vector<std::string> roles; // Roles to assign
        int priority = 0;               // Higher priority evaluated first
        
        mutable std::optional<std::regex> compiled_regex;
    };
    
    struct ValidationContext {
        std::optional<std::string> ip_address;   ///< Client IP address
        std::optional<std::string> user_agent;   ///< HTTP User-Agent header
        std::optional<std::string> action;       ///< Action being performed (default: "authenticate")
        std::optional<std::string> resource;     ///< Resource being accessed
        std::unordered_map<std::string, std::string> attributes;
    };

    struct ValidationResult {
        bool allowed = false;
        std::string principal;
        std::vector<std::string> roles;
        std::string denial_reason;      // If not allowed, why?
        std::string matched_rule;       // Which rule matched
        std::string abac_policy_id;     // Matched ABAC policy id (if ABAC evaluated)
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

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit PrincipalValidator(const Config& config = Config::defaults());

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Set Abac Engine.
     * @param[in,out] engine Input/output parameter.
     * @details Implements setAbacEngine without additional internal calls.
     */
    void setAbacEngine(PolicyEngine* engine) { abac_engine_ = engine; }

    const PolicyEngine* getAbacEngine() const { return abac_engine_; }
    
    ValidationResult validate(const std::string& principal,
                              const ValidationContext& ctx = {});
    
    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(const Rule& rule);
    
    /**
     * @brief Add Mapping Rule.
     * @param[in] rule Input parameter.
     */
    void addMappingRule(const MappingRule& rule);
    
    /**
     * @brief Clear Rules.
     * @param[in] type Input parameter.
     */
    void clearRules(RuleType type);
    
    const Config& getConfig() const { return config_; }
    
    struct Statistics {
        uint64_t total_validations = 0;
        uint64_t allowed = 0;
        uint64_t denied = 0;
        uint64_t blacklisted = 0;
        uint64_t whitelisted = 0;
        uint64_t default_allow = 0;
        uint64_t default_deny = 0;
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

private:
    Config config_;
    mutable Statistics stats_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning, optional.
    PolicyEngine*       abac_engine_{nullptr};   ///< Non-owning, optional ABAC engine.
    
    /**
     * @brief Check if principal matches a rule
     * @param[in] principal Input parameter.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchesRule(const std::string& principal, const Rule& rule) const;
    
    /**
     * @brief Check if principal matches a mapping rule
     * @param[in] principal Input parameter.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchesMappingRule(const std::string& principal, const MappingRule& rule) const;
    
    /**
     * @brief Apply mapping rules to get roles
     * @param[in] principal Input parameter.
     * @return Return value.
     */
    std::vector<std::string> applyMappingRules(const std::string& principal) const;
    
    // Log audit trail
    /**
     * @brief Log Audit.
     * @param[in] result Input parameter.
     */
    void logAudit(const ValidationResult& result) const;
    
    /**
     * @brief Compile regex for a rule
     * @param[in] rule Input parameter.
     */
    void compileRegex(const Rule& rule) const;
    /**
     * @brief Compile Regex.
     * @param[in] rule Input parameter.
     */
    void compileRegex(const MappingRule& rule) const;
};

class PrincipalValidatorPresets {
public:
    /**
     * @brief Realm Restricted.
     * @param[in] realm Input parameter.
     * @return Return value.
     */
    static PrincipalValidator realmRestricted(const std::string& realm);
    
    /**
     * @brief With Blacklist.
     * @param[in] blocked_principals Input parameter.
     * @return Return value.
     */
    static PrincipalValidator withBlacklist(const std::vector<std::string>& blocked_principals);
    
    /**
     * @brief With Whitelist.
     * @param[in] allowed_principals Input parameter.
     * @return Return value.
     */
    static PrincipalValidator withWhitelist(const std::vector<std::string>& allowed_principals);
    
    /**
     * @brief Enterprise Standard.
     * @param[in] realm Input parameter.
     * @return Return value.
     */
    static PrincipalValidator enterpriseStandard(const std::string& realm);
};

} // namespace auth
} // namespace themis
