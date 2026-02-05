#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <mutex>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// PolicyRule represents a single governance rule
struct PolicyRule {
    std::string id;                                    // Unique identifier
    std::string name;                                  // Human-readable name
    std::string description;                           // Description of the rule
    std::string classification_level;                  // e.g., "offen", "vs-nfd", "geheim", "streng-geheim"
    bool enabled = true;                               // Whether the rule is active
    
    // Conditions
    std::vector<std::string> resources;                // Resource patterns (e.g., "data/*", "keys/*")
    std::vector<std::string> actions;                  // Action patterns (e.g., "read", "write", "*")
    std::vector<std::string> required_roles;           // Required roles for access
    
    // Effects
    bool require_encryption = false;                   // Whether encryption is required
    bool require_signature = false;                    // Whether signature is required
    bool allow_export = true;                          // Whether export is allowed
    bool allow_cache = true;                           // Whether caching is allowed
    int retention_days = 365;                          // Data retention period
    std::string redaction_level = "standard";          // "none", "standard", "strict"
    
    // Audit
    bool audit_access = false;                         // Whether to audit access
    bool audit_changes = false;                        // Whether to audit changes
    
    // Metadata
    int priority = 0;                                  // Priority (higher = more important)
    std::string created_by;                            // User who created the rule
    int64_t created_at = 0;                            // Unix timestamp
    int64_t updated_at = 0;                            // Unix timestamp
    
    nlohmann::json toJson() const;
    static PolicyRule fromJson(const nlohmann::json& j);
    
    /// Check if rule applies to a resource/action combination
    bool appliesTo(const std::string& resource, const std::string& action) const;
};

/// PolicyManager manages governance rules and RBAC policies
class PolicyManager {
public:
    PolicyManager();
    
    /// Load policy rules from YAML/JSON file
    bool loadRules(const std::string& path);
    
    /// Save policy rules to YAML/JSON file
    bool saveRules(const std::string& path);
    
    /// Add a policy rule
    void addRule(const PolicyRule& rule);
    
    /// Remove a policy rule by ID
    void removeRule(const std::string& rule_id);
    
    /// Get a policy rule by ID
    std::optional<PolicyRule> getRule(const std::string& rule_id) const;
    
    /// List all policy rules
    std::vector<PolicyRule> listRules() const;
    
    /// Find applicable rules for a resource/action combination
    /// @param resource Resource identifier
    /// @param action Action identifier
    /// @param user_roles User's roles (for role-based filtering)
    /// @return Vector of applicable rules, sorted by priority (highest first)
    std::vector<PolicyRule> findApplicableRules(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /// Evaluate policy for a given request
    /// @param resource Resource being accessed
    /// @param action Action being performed
    /// @param user_roles User's roles
    /// @return Policy decision with aggregated effects
    struct PolicyDecision {
        bool allowed = true;                           // Whether access is allowed
        bool require_encryption = false;               // Whether encryption is required
        bool require_signature = false;                // Whether signature is required
        bool allow_export = true;                      // Whether export is allowed
        bool allow_cache = true;                       // Whether caching is allowed
        int retention_days = 365;                      // Data retention period
        std::string redaction_level = "standard";      // Redaction level
        bool audit_access = false;                     // Whether to audit access
        bool audit_changes = false;                    // Whether to audit changes
        std::string classification_level;              // Effective classification level
        std::vector<std::string> applied_rules;        // IDs of applied rules
    };
    
    PolicyDecision evaluatePolicy(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /// Validate policy rules (check for conflicts, cycles, etc.)
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    ValidationResult validateRules() const;
    
    /// Get policy statistics
    struct PolicyStats {
        int total_rules = 0;
        int enabled_rules = 0;
        int disabled_rules = 0;
        std::unordered_map<std::string, int> rules_by_classification;
    };
    PolicyStats getStats() const;
    
    /// Export rules as JSON
    nlohmann::json exportRules() const;
    
    /// Import rules from JSON
    bool importRules(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyRule> rules_;
    
    /// Helper: match pattern with wildcards
    bool matchPattern(const std::string& pattern, const std::string& value) const;
    
    /// Helper: aggregate effects from multiple rules
    PolicyDecision aggregateRules(const std::vector<PolicyRule>& rules) const;
};

} // namespace governance
} // namespace themis
