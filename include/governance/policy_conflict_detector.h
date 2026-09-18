/**
 * @file policy_conflict_detector.h
 * @brief Policy Conflict Detection and Resolution Engine
 * 
 * Detects and analyzes conflicts between policy rules with comprehensive
 * conflict categorization, severity assessment, and resolution recommendations.
 * 
 * **Conflict Types:**
 * - PERMIT-DENY: Same resource/action, conflicting effects
 * - Overlapping: Partially overlapping policies with no explicit precedence
 * - Circular Dependency: Policy chains forming cycles
 * - Type Mismatch: Incompatible rule types
 * 
 * **Precedence Algorithm (Deny-Overrides-Permit):**
 * 1. Evaluate all applicable rules for a request
 * 2. If ANY rule denies access -> DENY (unless overridden by explicit precedence)
 * 3. If explicit precedence exists -> follow priority order
 * 4. Otherwise -> PERMIT (default allow)
 * 5. Ties resolved by creation timestamp (earliest wins)
 * 
 * **Atomic Updates:**
 * Policy updates validate for conflicts before committing. Failed updates
 * roll back to previous state with full consistency guarantees.
 * 
 * @version 0.1.0
 * @since 2026-08-18
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "governance/policy_manager.h"

namespace themis {
namespace governance {

enum class ConflictSeverity {
    LOW,       ///< Informational, no impact on functionality
    MEDIUM,    ///< May cause unexpected behavior, review recommended
    HIGH,      ///< Will cause security issues or access control failures
    CRITICAL   ///< Immediate risk to data security or compliance
};

enum class ConflictType {
    PERMIT_DENY,           ///< Contradictory access decisions (allow vs deny)
    OVERLAPPING,           ///< Partially overlapping with no precedence
    CIRCULAR_DEPENDENCY,   ///< Policy chains forming cycles
    TYPE_MISMATCH,         ///< Incompatible rule types
    ENCRYPTION_CONFLICT,   ///< Encryption requirement conflicts
    EXPORT_CONFLICT,       ///< Export permission conflicts
    RETENTION_CONFLICT,    ///< Data retention conflicts
    COMPLIANCE_CONFLICT    ///< Cross-framework compliance conflicts
};

struct PolicyConflict {
    std::string conflict_id;                  ///< Unique conflict identifier
    ConflictType conflict_type;               ///< Category of conflict
    std::vector<std::string> conflicting_rule_ids;  ///< IDs of involved rules
    std::string description;                  ///< Human-readable conflict description
    ConflictSeverity severity;                ///< Severity classification
    std::string resolution_strategy;          ///< Recommended resolution
    int64_t detected_at;                      ///< Timestamp of detection
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct PrecedenceEvaluation {
    std::string rule_id;                      ///< Rule being evaluated
    int effective_priority;                   ///< Computed priority
    bool has_explicit_precedence;             ///< Whether explicit ordering exists
    std::vector<std::string> overrides;       ///< Rules this one overrides
    std::vector<std::string> overridden_by;   ///< Rules that override this one
    std::string rationale;                    ///< Explanation of priority decision
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct AtomicUpdateResult {
    bool success = 0;                             ///< Whether update succeeded
    std::string transaction_id;               ///< Transaction identifier
    std::vector<PolicyConflict> conflicts_detected; ///< Any conflicts found
    std::string error_message;                ///< Error details if failed
    std::vector<std::string> affected_rules;  ///< Rules affected by update
    int64_t operation_time_us;                ///< Microseconds to complete
};

class PolicyConflictDetector {
public:
    PolicyConflictDetector();
    ~PolicyConflictDetector() = default;
    
    // Disable copy; allow move
    PolicyConflictDetector(const PolicyConflictDetector&) = delete;
    PolicyConflictDetector& operator=(const PolicyConflictDetector&) = delete;
    PolicyConflictDetector(PolicyConflictDetector&&) noexcept = default;
    PolicyConflictDetector& operator=(PolicyConflictDetector&&) noexcept = default;

    /**
     * @brief Detect All Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectAllConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect Permit Deny Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectPermitDenyConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect Overlapping Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectOverlappingConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect Circular Dependencies.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<PolicyConflict> detectCircularDependencies(const PolicyManager& policy_mgr);

    /**
     * @brief Evaluate Rule Precedence.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    PrecedenceEvaluation evaluateRulePrecedence(
        const std::string& rule_id,
        const PolicyManager& policy_mgr
    );

    std::unordered_map<std::string, PrecedenceEvaluation> evaluateAllPrecedence(
        const PolicyManager& policy_mgr
    );

    /**
     * @brief Atomic Add Rule.
     * @param[in] rule Input parameter.
     * @param[in,out] policy_mgr Input/output parameter.
     * @return Return value.
     */
    AtomicUpdateResult atomicAddRule(
        const PolicyRule& rule,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Atomic Update Rule.
     * @param[in] rule Input parameter.
     * @param[in,out] policy_mgr Input/output parameter.
     * @return Return value.
     */
    AtomicUpdateResult atomicUpdateRule(
        const PolicyRule& rule,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Atomic Remove Rule.
     * @param[in] rule_id Identifier of the rule.
     * @param[in,out] policy_mgr Input/output parameter.
     * @return Return value.
     */
    AtomicUpdateResult atomicRemoveRule(
        const std::string& rule_id,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Check Rule Conflict.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return Return value.
     */
    std::optional<PolicyConflict> checkRuleConflict(
        const PolicyRule& rule1,
        const PolicyRule& rule2
    );

    /**
     * @brief Get Cached Conflicts.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<PolicyConflict> getCachedConflicts(const PolicyManager& policy_mgr) const;

    /**
     * @brief Clear Cache.
     */
    void clearCache();

    /**
     * @brief Set Caching Enabled.
     * @param[in] enabled Input parameter.
     * @details Implements setCachingEnabled without additional internal calls.
     */
    void setCachingEnabled(bool enabled) { caching_enabled_ = enabled; }

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    nlohmann::json getStatistics() const;

private:
    /**
     * @brief Rules Match.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool rulesMatch(const PolicyRule& rule1, const PolicyRule& rule2) const;

    /**
     * @brief Has Same Scope.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasSameScope(const PolicyRule& rule1, const PolicyRule& rule2) const;

    /**
     * @brief Compute Severity.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @param[in] conflict_type Input parameter.
     * @return Return value.
     */
    ConflictSeverity computeSeverity(
        const PolicyRule& rule1,
        const PolicyRule& rule2,
        ConflictType conflict_type
    ) const;

    /**
     * @brief Generate Conflict Id.
     * @param[in] rule_ids Input parameter.
     * @param[in] conflict_type Input parameter.
     * @return Return value.
     */
    std::string generateConflictId(
        const std::vector<std::string>& rule_ids,
        ConflictType conflict_type
    ) const;

    /**
     * @brief Has Circular Dependency.
     * @param[in] rule_id Identifier of the rule.
     * @param[in,out] visited Input/output parameter.
     * @param[in,out] rec_stack Input/output parameter.
     * @param[in] policy_mgr Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCircularDependency(
        const std::string& rule_id,
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& rec_stack,
        const PolicyManager& policy_mgr
    ) const;

    // State management
    mutable std::shared_mutex state_mutex_;
    std::unordered_map<std::string, std::vector<PolicyConflict>> conflict_cache_;
    int64_t cache_timestamp_ = 0;
    bool caching_enabled_ = true;
    uint64_t total_detections_ = 0;
    std::unordered_map<std::string, uint64_t> conflict_type_counts_;
};

} // namespace governance
} // namespace themis
