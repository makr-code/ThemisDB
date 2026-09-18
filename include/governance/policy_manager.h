/**
 * @file policy_manager.h
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
#include <unordered_map>
#include <optional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "policy_version_history.h"
#include "governance_diagnostics.h"

namespace themis {
namespace governance {

enum class PolicyState {
    DRAFT       = 0,  ///< Policy created but not yet activated
    ACTIVE      = 1,  ///< Policy is actively enforced
    DEPRECATED  = 2,  ///< Policy retained for audit but not enforced
    RETIRED     = 3,  ///< Policy archived, no longer used
};

struct PolicyLifecycle {
    PolicyState current_state = PolicyState::DRAFT;
    int64_t created_at = 0;           ///< Unix timestamp (ms) of creation
    int64_t activated_at = 0;         ///< Unix timestamp (ms) of activation
    int64_t deprecated_at = 0;        ///< Unix timestamp (ms) of deprecation
    int64_t retired_at = 0;           ///< Unix timestamp (ms) of retirement
    std::string created_by;           ///< User who created the rule
    std::string last_modified_by;     ///< User who last modified the rule
    
    /**
     * @brief Can Transition To.
     * @param[in] target_state Input parameter.
     * @return True when the operation succeeds.
     */
    bool canTransitionTo(PolicyState target_state) const;
    
    /**
     * @brief Get State Description.
     * @return Return value.
     */
    std::string getStateDescription() const;
};

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
    
    // Versioning (GAP-004 Phase 5)
    std::string version = "1.0.0";                     // Semantic version
    std::string last_modified_by;                      // User who last modified the rule
    std::string change_description;                    // Description of last change
    
    // Lifecycle management (Phase 2-3)
    PolicyLifecycle lifecycle;                         // State machine and audit trail

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static PolicyRule fromJson(const nlohmann::json& j);
    
    /**
     * @brief Applies To.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @return True when the operation succeeds.
     */
    bool appliesTo(const std::string& resource, const std::string& action) const;
};

struct PolicySet {
    std::unordered_map<std::string, PolicyRule> rules;
    std::string version_hash;
    std::int64_t loaded_at = 0; ///< Unix epoch milliseconds of last load
};

class PolicyManager {
public:
    // ========== Policy Error Handling (Phase 2-3) ==========
    
    enum class PolicyError {
        kSuccess                = 0,  // Operation succeeded
        kRuleNotFound           = 1,  // Rule with given ID not found
        kInvalidStateTransition = 2,  // State transition not allowed
        kConflictDetected       = 3,  // Policy conflicts detected
        kAuditFailed            = 4,  // Audit logging failed
    };
    
    struct PolicyResult {
        PolicyError error = PolicyError::kSuccess;
        std::string error_message;
        std::string rule_version;
    };
    
    PolicyManager();
    
    /**
     * @brief Load Rules.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadRules(const std::string& path);
    
    /**
     * @brief Save Rules.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveRules(const std::string& path);
    
    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(const PolicyRule& rule);
    
    /**
     * @brief Remove Rule.
     * @param[in] rule_id Identifier of the rule.
     */
    void removeRule(const std::string& rule_id);
    
    /**
     * @brief Get Rule.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<PolicyRule> getRule(const std::string& rule_id) const;
    
    /**
     * @brief List Rules.
     * @return Return value.
     */
    std::vector<PolicyRule> listRules() const;
    
    /**
     * @brief Find Applicable Rules.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return Return value.
     */
    std::vector<PolicyRule> findApplicableRules(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
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
    
    /**
     * @brief Evaluate Policy.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return Return value.
     */
    PolicyDecision evaluatePolicy(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    /**
     * @brief Validate Rules.
     * @return Return value.
     */
    ValidationResult validateRules() const;
    
    struct PolicyStats {
        int total_rules = 0;
        int enabled_rules = 0;
        int disabled_rules = 0;
        std::unordered_map<std::string, int> rules_by_classification;
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    PolicyStats getStats() const;
    
    /**
     * @brief Export Rules.
     * @return Return value.
     */
    nlohmann::json exportRules() const;
    
    /**
     * @brief Import Rules.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importRules(const nlohmann::json& j);
    
    /**
     * @brief ========== Phase 5: Versioning & History ==========
     * @param[in] rule_id Identifier of the rule.
     * @param[in] updated_rule Input parameter.
     * @param[in] modified_by Input parameter.
     * @param[in] change_description Input parameter.
     * @return True when the operation succeeds.
     */
    
    bool updateRule(const std::string& rule_id, const PolicyRule& updated_rule, 
                    const std::string& modified_by, const std::string& change_description);
    
    /**
     * @brief Get Rule Versions.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<PolicyRuleVersion> getRuleVersions(const std::string& rule_id) const;
    
    /**
     * @brief Get Rule Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<PolicyRuleVersion> getRuleVersion(
        const std::string& rule_id, const std::string& version) const;
    
    /**
     * @brief Rollback To Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version Input parameter.
     * @param[in] modified_by Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackToVersion(const std::string& rule_id, const std::string& version, 
                           const std::string& modified_by);
    
    /**
     * @brief Rollback To Previous Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] modified_by Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackToPreviousVersion(const std::string& rule_id, const std::string& modified_by);
    
    /**
     * @brief Preview Rollback.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    std::vector<VersionDiff> previewRollback(
        const std::string& rule_id, const std::string& target_version) const;
    
    /**
     * @brief Compare Rule Versions.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version1 Input parameter.
     * @param[in] version2 Input parameter.
     * @return Return value.
     */
    std::vector<VersionDiff> compareRuleVersions(
        const std::string& rule_id, const std::string& version1, const std::string& version2) const;
    
    std::vector<PolicyRuleVersion> getAuditTrail(
        const std::string& rule_id, int64_t start_time = 0, int64_t end_time = INT64_MAX) const;
    
    std::vector<PolicyRuleVersion> getAuditTrailByUser(
        const std::string& user, int64_t start_time = 0, int64_t end_time = INT64_MAX) const;

    /**
     * @brief ========== Lifecycle State Management (Phase 2-3) ==========
     * @param[in] rule_id Identifier of the rule.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    
    PolicyResult activateRuleWithValidation(
        const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Deprecate Rule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::string deprecateRule(const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Retire Rule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::string retireRule(const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Can Transition Rule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_state Input parameter.
     * @return True when the operation succeeds.
     */
    bool canTransitionRule(const std::string& rule_id, PolicyState target_state);

    // ========== Hot-Reload API (double-buffer) ==========

    bool reloadPolicies(const std::string& path, std::string* err = nullptr);

    /**
     * @brief Active Policy Version.
     * @return Return value.
     */
    std::string activePolicyVersion() const;
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyRule> rules_;
    PolicyVersionHistory version_history_;             // Version history manager
    DiagnosticAggregator diagnostics_;                 // Phase 2-3: diagnostic recorder

    // Double-buffer for hot-reload: reloadPolicies() promotes a new PolicySet
    // with a release-store; findApplicableRules()/evaluatePolicy() acquire a
    // shared_ptr snapshot so that in-flight reads never block on reload.
    mutable std::shared_mutex policy_set_mutex_;
    std::shared_ptr<const PolicySet> active_policy_set_;  // null until first reloadPolicies()
    
    /**
     * @brief Match Pattern.
     * @param[in] pattern Input parameter.
     * @param[in] value Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchPattern(const std::string& pattern, const std::string& value) const;
    
    /**
     * @brief Aggregate Rules.
     * @param[in] rules Input parameter.
     * @return Return value.
     */
    PolicyDecision aggregateRules(const std::vector<PolicyRule>& rules) const;
    
    std::string incrementVersion(const std::string& current_version, int level = 2) const; // 0=major, 1=minor, 2=patch
    
    /**
     * @brief Check Conflicts For Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<std::string> checkConflictsForRule(const PolicyRule& rule) const;
};

} // namespace governance
} // namespace themis
