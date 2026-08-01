/**
 * @file policy_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: policy_manager.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 251
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4300 feat(governance): CSV expor... (2026-03-17) | #1075 Implement GAP-004 Phase 5: ... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Policy lifecycle state machine.
 *
 * Defines valid state transitions: DRAFT → ACTIVE → (DEPRECATED|RETIRED)
 * and DEPRECATED → RETIRED. Used to track policy maturity and enforcement status.
 */
enum class PolicyState {
    DRAFT       = 0,  ///< Policy created but not yet activated
    ACTIVE      = 1,  ///< Policy is actively enforced
    DEPRECATED  = 2,  ///< Policy retained for audit but not enforced
    RETIRED     = 3,  ///< Policy archived, no longer used
};

/**
 * @brief Lifecycle metadata for a policy rule.
 *
 * Tracks state transitions, timestamps, and user actions for audit
 * and compliance purposes.
 */
struct PolicyLifecycle {
    PolicyState current_state = PolicyState::DRAFT;
    int64_t created_at = 0;           ///< Unix timestamp (ms) of creation
    int64_t activated_at = 0;         ///< Unix timestamp (ms) of activation
    int64_t deprecated_at = 0;        ///< Unix timestamp (ms) of deprecation
    int64_t retired_at = 0;           ///< Unix timestamp (ms) of retirement
    std::string created_by;           ///< User who created the rule
    std::string last_modified_by;     ///< User who last modified the rule
    
    /**
     * @brief Validate if a state transition is allowed.
     * 
     * @param target_state Desired next state.
     * @return true if transition is valid, false otherwise.
     */
    bool canTransitionTo(PolicyState target_state) const;
    
    /**
     * @brief Get human-readable description of current state.
     * 
     * @return Description string.
     */
    std::string getStateDescription() const;
};

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
    
    // Versioning (GAP-004 Phase 5)
    std::string version = "1.0.0";                     // Semantic version
    std::string last_modified_by;                      // User who last modified the rule
    std::string change_description;                    // Description of last change
    
    // Lifecycle management (Phase 2-3)
    PolicyLifecycle lifecycle;                         // State machine and audit trail

    nlohmann::json toJson() const;
    static PolicyRule fromJson(const nlohmann::json& j);
    
    /// Check if rule applies to a resource/action combination
    bool appliesTo(const std::string& resource, const std::string& action) const;
};

/// Immutable snapshot of the complete policy rule set.
///
/// Used as the double-buffer read target for `PolicyManager::reloadPolicies()`.
/// Readers capture a `shared_ptr<const PolicySet>` without holding any lock;
/// the old set is kept alive by its ref-count until all in-flight readers finish,
/// satisfying the "zero requests dropped during reload window" requirement.
///
/// **Immutability contract**: instances must never be modified after being
/// published via the `active_policy_set_` field.  Always create a fresh
/// `PolicySet` and atomically swap the pointer instead of mutating the current one.
struct PolicySet {
    std::unordered_map<std::string, PolicyRule> rules;
    /// Stable deterministic identifier derived from sorted rule IDs.
    /// Used only for logging and audit entries, not for cryptographic integrity.
    std::string version_hash;
    std::int64_t loaded_at = 0; ///< Unix epoch milliseconds of last load
};

/// PolicyManager manages governance rules and RBAC policies
class PolicyManager {
public:
    // ========== Policy Error Handling (Phase 2-3) ==========
    
    /// Error codes for policy lifecycle operations.
    enum class PolicyError {
        kSuccess                = 0,  // Operation succeeded
        kRuleNotFound           = 1,  // Rule with given ID not found
        kInvalidStateTransition = 2,  // State transition not allowed
        kConflictDetected       = 3,  // Policy conflicts detected
        kAuditFailed            = 4,  // Audit logging failed
    };
    
    /// Result of a policy operation with error details.
    struct PolicyResult {
        PolicyError error = PolicyError::kSuccess;
        std::string error_message;
        std::string rule_version;
    };
    
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
    
    // ========== Phase 5: Versioning & History ==========
    
    /// Update a rule (creates a new version in history)
    bool updateRule(const std::string& rule_id, const PolicyRule& updated_rule, 
                    const std::string& modified_by, const std::string& change_description);
    
    /// Get version history for a rule
    std::vector<PolicyRuleVersion> getRuleVersions(const std::string& rule_id) const;
    
    /// Get a specific version of a rule
    std::optional<PolicyRuleVersion> getRuleVersion(
        const std::string& rule_id, const std::string& version) const;
    
    /// Rollback a rule to a specific version
    bool rollbackToVersion(const std::string& rule_id, const std::string& version, 
                           const std::string& modified_by);
    
    /// Rollback a rule to the previous version
    bool rollbackToPreviousVersion(const std::string& rule_id, const std::string& modified_by);
    
    /// Preview changes that would occur if rolling back to a version
    std::vector<VersionDiff> previewRollback(
        const std::string& rule_id, const std::string& target_version) const;
    
    /// Compare two versions of a rule
    std::vector<VersionDiff> compareRuleVersions(
        const std::string& rule_id, const std::string& version1, const std::string& version2) const;
    
    /// Get audit trail for a rule
    std::vector<PolicyRuleVersion> getAuditTrail(
        const std::string& rule_id, int64_t start_time = 0, int64_t end_time = INT64_MAX) const;
    
    /// Query audit trail by user
    std::vector<PolicyRuleVersion> getAuditTrailByUser(
        const std::string& user, int64_t start_time = 0, int64_t end_time = INT64_MAX) const;

    // ========== Lifecycle State Management (Phase 2-3) ==========
    
    /**
     * @brief Transition a policy rule to ACTIVE state.
     * 
     * Validates state transition, checks for conflicts, and logs audit event.
     * Returns detailed result with error codes.
     * 
     * @param rule_id Rule identifier.
     * @param user_id User performing the activation.
     * @return PolicyResult with success/error details.
     */
    PolicyResult activateRuleWithValidation(
        const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Transition a policy rule from ACTIVE to DEPRECATED.
     * 
     * Policy is retained in history but no longer enforced.
     * 
     * @param rule_id Rule identifier.
     * @param user_id User performing the deprecation.
     * @return Rule version on success, empty string on failure.
     */
    std::string deprecateRule(const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Transition a policy rule to RETIRED (terminal) state.
     * 
     * @param rule_id Rule identifier.
     * @param user_id User performing the retirement.
     * @return Rule version on success, empty string on failure.
     */
    std::string retireRule(const std::string& rule_id, const std::string& user_id);
    
    /**
     * @brief Check if a state transition is valid for a given rule.
     * 
     * @param rule_id Rule identifier.
     * @param target_state Desired next state.
     * @return true if transition is allowed, false otherwise.
     */
    bool canTransitionRule(const std::string& rule_id, PolicyState target_state);

    // ========== Hot-Reload API (double-buffer) ==========

    /// Reload policies from disk with an atomic double-buffer swap.
    ///
    /// Loads the new rule set from @p path, validates it via PolicyValidator,
    /// and – only if validation passes – atomically promotes it as the active
    /// PolicySet via a release-store. Readers that captured a snapshot of the
    /// old set before the swap will complete normally (the old PolicySet stays
    /// alive through its shared_ptr ref-count).
    ///
    /// On validation failure the current rule set is retained unchanged.
    /// Emits a `governance_policy_reload_total` Prometheus counter with
    /// `result=success` or `result=failure` in both cases.
    ///
    /// @param path   Path to a YAML or JSON policy file.
    /// @param err    Optional output: error description on failure.
    /// @return       true on success, false on load or validation failure.
    bool reloadPolicies(const std::string& path, std::string* err = nullptr);

    /// @return The version hash of the currently active PolicySet.
    ///         Empty string if no policy set has been promoted via reloadPolicies().
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
    
    /// Helper: match pattern with wildcards
    bool matchPattern(const std::string& pattern, const std::string& value) const;
    
    /// Helper: aggregate effects from multiple rules
    PolicyDecision aggregateRules(const std::vector<PolicyRule>& rules) const;
    
    /// Helper: increment semantic version
    std::string incrementVersion(const std::string& current_version, int level = 2) const; // 0=major, 1=minor, 2=patch
    
    /// Helper: detect conflicts between a rule and all active rules
    std::vector<std::string> checkConflictsForRule(const PolicyRule& rule) const;
};

} // namespace governance
} // namespace themis
