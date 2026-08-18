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
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "governance/policy_manager.h"

namespace themis {
namespace governance {

/**
 * @brief Conflict severity levels
 */
enum class ConflictSeverity {
    LOW,       ///< Informational, no impact on functionality
    MEDIUM,    ///< May cause unexpected behavior, review recommended
    HIGH,      ///< Will cause security issues or access control failures
    CRITICAL   ///< Immediate risk to data security or compliance
};

/**
 * @brief Conflict types with root cause classifications
 */
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

/**
 * @brief Represents a single detected conflict
 */
struct PolicyConflict {
    std::string conflict_id;                  ///< Unique conflict identifier
    ConflictType conflict_type;               ///< Category of conflict
    std::vector<std::string> conflicting_rule_ids;  ///< IDs of involved rules
    std::string description;                  ///< Human-readable conflict description
    ConflictSeverity severity;                ///< Severity classification
    std::string resolution_strategy;          ///< Recommended resolution
    int64_t detected_at;                      ///< Timestamp of detection
    
    /**
     * Convert conflict to JSON for API responses and logging
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Precedence evaluation result for a rule
 */
struct PrecedenceEvaluation {
    std::string rule_id;                      ///< Rule being evaluated
    int effective_priority;                   ///< Computed priority
    bool has_explicit_precedence;             ///< Whether explicit ordering exists
    std::vector<std::string> overrides;       ///< Rules this one overrides
    std::vector<std::string> overridden_by;   ///< Rules that override this one
    std::string rationale;                    ///< Explanation of priority decision
    
    nlohmann::json toJson() const;
};

/**
 * @brief Result of atomic update validation
 */
struct AtomicUpdateResult {
    bool success;                             ///< Whether update succeeded
    std::string transaction_id;               ///< Transaction identifier
    std::vector<PolicyConflict> conflicts_detected; ///< Any conflicts found
    std::string error_message;                ///< Error details if failed
    std::vector<std::string> affected_rules;  ///< Rules affected by update
    int64_t operation_time_us;                ///< Microseconds to complete
};

/**
 * @brief Policy Conflict Detection and Resolution Engine
 * 
 * **Thread Safety:**
 * All public methods are thread-safe. Concurrent reads are allowed.
 * Writes are serialized with a shared_mutex.
 * 
 * **Performance:**
 * - Conflict detection: O(n²) in worst case (n = number of rules)
 * - Conflict caching: Results cached until policy changes
 * - Precedence evaluation: O(n log n) with incremental updates
 */
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
     * @brief Detect all conflicts in a policy set
     * 
     * Performs comprehensive conflict analysis across all rules in the
     * policy manager. Returns all detected conflicts with severity and
     * resolution recommendations.
     * 
     * **Complexity:** O(n²) where n = number of rules
     * 
     * @param policy_mgr PolicyManager containing rules to analyze
     * @return Vector of detected conflicts
     * @throws std::runtime_error if policy_mgr is invalid
     */
    std::vector<PolicyConflict> detectAllConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect PERMIT-DENY conflicts
     * 
     * Identifies rules with same resource/action patterns but contradictory
     * effects (one allows, one denies). These represent the most critical
     * conflict category requiring immediate resolution.
     * 
     * **Example:**
     * - Rule A: resource="data/*", action="read", allow_export=true
     * - Rule B: resource="data/*", action="read", allow_export=false
     * → Conflict detected (export permission conflict)
     * 
     * @param policy_mgr PolicyManager to analyze
     * @return Vector of PERMIT-DENY conflicts
     */
    std::vector<PolicyConflict> detectPermitDenyConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect overlapping but not contradictory rules
     * 
     * Identifies rules with overlapping resource/action patterns where
     * precedence is ambiguous (neither rule is strictly more permissive).
     * May require manual prioritization.
     * 
     * @param policy_mgr PolicyManager to analyze
     * @return Vector of overlapping conflicts
     */
    std::vector<PolicyConflict> detectOverlappingConflicts(const PolicyManager& policy_mgr);

    /**
     * @brief Detect circular dependencies in policy chain
     * 
     * Identifies circular dependency chains where policies reference
     * each other in a cycle, preventing deterministic evaluation.
     * 
     * **Example:**
     * - Policy A depends on Policy B
     * - Policy B depends on Policy C
     * - Policy C depends on Policy A
     * → Circular dependency detected
     * 
     * @param policy_mgr PolicyManager to analyze
     * @return Vector of circular dependency conflicts
     */
    std::vector<PolicyConflict> detectCircularDependencies(const PolicyManager& policy_mgr);

    /**
     * @brief Evaluate precedence for a specific rule
     * 
     * Determines the effective priority of a rule considering:
     * 1. Explicit priority values (lower number = higher priority)
     * 2. Deny-Overrides-Permit pattern (deny rules have higher implicit priority)
     * 3. Creation timestamp (earlier rules break ties)
     * 4. Scope specificity (more specific scopes have higher priority)
     * 
     * **Algorithm:**
     * ```
     * effective_priority = explicit_priority * 100 + precedence_bonus
     * precedence_bonus:
     *   - Deny rules: +50
     *   - Specific scope: +20
     *   - Creation order: +((now - created_at) / 1 second)
     * ```
     * 
     * @param rule_id Rule to evaluate
     * @param policy_mgr PolicyManager context
     * @return Precedence evaluation with rationale
     */
    PrecedenceEvaluation evaluateRulePrecedence(
        const std::string& rule_id,
        const PolicyManager& policy_mgr
    );

    /**
     * @brief Evaluate precedence for all rules
     * 
     * Computes precedence relationships across entire policy set,
     * identifying which rules override which.
     * 
     * **Time Complexity:** O(n² log n) with caching
     * 
     * @param policy_mgr PolicyManager to analyze
     * @return Map of rule_id -> PrecedenceEvaluation
     */
    std::unordered_map<std::string, PrecedenceEvaluation> evaluateAllPrecedence(
        const PolicyManager& policy_mgr
    );

    /**
     * @brief Atomically add a new rule with conflict validation
     * 
     * Validates the new rule for conflicts with existing rules before
     * committing. If conflicts are detected, the transaction is rolled back
     * and an error result is returned. No partial state is left behind.
     * 
     * **Transaction Semantics:**
     * 1. Acquire exclusive lock on policy manager
     * 2. Create snapshot of current state
     * 3. Attempt to add rule
     * 4. Run conflict detection on updated set
     * 5. If conflicts found:
     *    a. Restore snapshot
     *    b. Release lock
     *    c. Return failure with conflict details
     * 6. If no conflicts:
     *    a. Commit changes
     *    b. Release lock
     *    c. Return success
     * 
     * **Performance:** O(n²) for conflict detection
     * 
     * @param rule Rule to add
     * @param policy_mgr PolicyManager to update
     * @return AtomicUpdateResult with status and details
     */
    AtomicUpdateResult atomicAddRule(
        const PolicyRule& rule,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Atomically update an existing rule with conflict validation
     * 
     * Validates changes to rule for conflicts before committing.
     * Uses same atomic semantics as atomicAddRule().
     * 
     * @param rule Updated rule
     * @param policy_mgr PolicyManager to update
     * @return AtomicUpdateResult with status and details
     */
    AtomicUpdateResult atomicUpdateRule(
        const PolicyRule& rule,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Atomically remove a rule
     * 
     * Removes rule and validates remaining rules for orphaned dependencies
     * or other issues. Automatically resolved if simple removal is sufficient.
     * 
     * @param rule_id Rule to remove
     * @param policy_mgr PolicyManager to update
     * @return AtomicUpdateResult with status and details
     */
    AtomicUpdateResult atomicRemoveRule(
        const std::string& rule_id,
        PolicyManager& policy_mgr
    );

    /**
     * @brief Check if two rules have conflicting effects
     * 
     * Low-level check for direct conflicts between two specific rules.
     * 
     * @param rule1 First rule
     * @param rule2 Second rule
     * @return Optional conflict if detected
     */
    std::optional<PolicyConflict> checkRuleConflict(
        const PolicyRule& rule1,
        const PolicyRule& rule2
    );

    /**
     * @brief Get cached conflict report
     * 
     * Returns previously computed conflict report without re-analyzing.
     * Report is invalidated when policy_mgr changes.
     * 
     * @param policy_mgr PolicyManager to check cache for
     * @return Vector of cached conflicts, empty if not cached
     */
    std::vector<PolicyConflict> getCachedConflicts(const PolicyManager& policy_mgr) const;

    /**
     * @brief Clear conflict detection cache
     * 
     * Invalidates all cached results. Useful after bulk policy updates.
     */
    void clearCache();

    /**
     * @brief Enable/disable caching of conflict results
     * 
     * When caching is enabled, conflict detection results are cached
     * and reused for identical policy sets. Default: enabled.
     * 
     * @param enabled Whether to cache results
     */
    void setCachingEnabled(bool enabled) { caching_enabled_ = enabled; }

    /**
     * @brief Get statistics on conflict detection
     * 
     * Returns metrics on conflict types, severities, and detection performance.
     * 
     * @return JSON statistics object
     */
    nlohmann::json getStatistics() const;

private:
    /**
     * @brief Check if two rules match on resource and action patterns
     * 
     * @return true if rules apply to same resource/action combinations
     */
    bool rulesMatch(const PolicyRule& rule1, const PolicyRule& rule2) const;

    /**
     * @brief Check if rules have same scope specificity
     */
    bool hasSameScope(const PolicyRule& rule1, const PolicyRule& rule2) const;

    /**
     * @brief Compute conflict severity based on conflict type and effects
     */
    ConflictSeverity computeSeverity(
        const PolicyRule& rule1,
        const PolicyRule& rule2,
        ConflictType conflict_type
    ) const;

    /**
     * @brief Generate unique conflict ID
     */
    std::string generateConflictId(
        const std::vector<std::string>& rule_ids,
        ConflictType conflict_type
    ) const;

    /**
     * @brief Check for circular dependencies using depth-first search
     */
    bool hasCircularDependency(
        const std::string& rule_id,
        const std::unordered_set<std::string>& visited,
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
