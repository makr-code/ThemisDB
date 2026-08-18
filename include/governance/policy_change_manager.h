/**
 * @file policy_change_manager.h
 * @brief Advanced policy change management with rollback, dependency tracking, and safety verification.
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Implements:
 * - Atomic rollback operations (single and multi-policy)
 * - Policy dependency tracking and resolution
 * - Rollback safety verification
 * - Coordinated multi-policy rollback
 * - Pre-rollback impact analysis
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// Forward declarations
struct PolicyRule;
struct PolicyRuleVersion;
class PolicyVersionHistory;
class PolicyManager;

/**
 * @brief Rollback safety check result
 */
enum class RollbackSafetyLevel {
    SAFE              = 0,  ///< Rollback is safe with no conflicts
    WARNING           = 1,  ///< Rollback has minor concerns
    BLOCKED           = 2,  ///< Rollback is unsafe and cannot proceed
};

/**
 * @brief Policy dependency information
 */
struct PolicyDependency {
    std::string dependent_rule_id;      ///< Rule depending on another
    std::string dependency_rule_id;     ///< Rule being depended on
    std::string dependency_type;        ///< "enforcement", "inheritance", "composition"
    std::string reason;                 ///< Description of dependency
    
    nlohmann::json toJson() const;
    static PolicyDependency fromJson(const nlohmann::json& j);
};

/**
 * @brief Rollback safety check result
 */
struct RollbackSafetyReport {
    std::string rule_id;                ///< Rule being rolled back
    std::string target_version;         ///< Target rollback version
    RollbackSafetyLevel safety_level;   ///< Safety assessment
    
    std::vector<std::string> conflicts; ///< Conflicting rule IDs
    std::vector<std::string> warnings;  ///< Warning messages
    std::vector<PolicyDependency> affected_dependencies;  ///< Dependencies that would be affected
    
    int estimated_duration_ms = 0;      ///< Estimated rollback duration
    bool is_reversible = true;          ///< Whether rollback can be reversed
    
    nlohmann::json toJson() const;
    static RollbackSafetyReport fromJson(const nlohmann::json& j);
};

/**
 * @brief Rollback operation record
 */
struct RollbackOperation {
    std::string operation_id;           ///< Unique rollback operation ID
    std::string rule_id;                ///< Primary rule being rolled back
    std::vector<std::string> multi_rule_ids;  ///< Additional rules in coordinated rollback
    std::string from_version;           ///< Version being rolled back from
    std::string to_version;             ///< Target rollback version
    std::string operator_user;          ///< User performing rollback
    int64_t started_at = 0;             ///< When rollback started
    int64_t completed_at = 0;           ///< When rollback completed
    bool success = false;               ///< Whether operation succeeded
    std::string error_message;          ///< Error message if failed
    std::string reason;                 ///< Why rollback was performed
    
    nlohmann::json toJson() const;
    static RollbackOperation fromJson(const nlohmann::json& j);
};

/**
 * @brief Manages policy changes with advanced rollback and dependency tracking
 * 
 * Provides:
 * - Atomic rollback with automatic versioning
 * - Multi-policy coordinated rollback
 * - Pre-rollback safety verification
 * - Dependency tracking and impact analysis
 * - Complete rollback audit trail
 * - Reversible rollback operations
 */
class PolicyChangeManager {
public:
    PolicyChangeManager();
    
    explicit PolicyChangeManager(
        std::shared_ptr<PolicyManager> policy_manager,
        std::shared_ptr<PolicyVersionHistory> version_history
    );
    
    /// Register a dependency between two policies
    /// @param dependent_rule_id Rule that depends on another
    /// @param dependency_rule_id Rule being depended on
    /// @param dependency_type Type of dependency
    /// @param reason Description of dependency
    void registerDependency(
        const std::string& dependent_rule_id,
        const std::string& dependency_rule_id,
        const std::string& dependency_type,
        const std::string& reason
    );
    
    /// Get all dependencies for a rule
    /// @param rule_id Rule identifier
    /// @return Vector of dependencies (rules this rule depends on)
    std::vector<PolicyDependency> getDependencies(const std::string& rule_id) const;
    
    /// Get all reverse dependencies for a rule
    /// @param rule_id Rule identifier
    /// @return Vector of rules that depend on this rule
    std::vector<PolicyDependency> getReverseDependencies(const std::string& rule_id) const;
    
    /// Check rollback safety before performing operation
    /// @param rule_id Rule to rollback
    /// @param target_version Target version
    /// @return Safety report with findings
    RollbackSafetyReport checkRollbackSafety(
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /// Preview rollback changes without applying
    /// @param rule_id Rule to rollback
    /// @param target_version Target version
    /// @return Safety report
    RollbackSafetyReport previewRollback(
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /// Perform atomic rollback to specific version
    /// @param rule_id Rule to rollback
    /// @param target_version Target version to rollback to
    /// @param operator_user User performing rollback
    /// @param reason Reason for rollback
    /// @return Operation record (success/failure)
    RollbackOperation performRollback(
        const std::string& rule_id,
        const std::string& target_version,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /// Perform coordinated multi-policy rollback
    /// @param rule_ids Multiple rules to rollback together
    /// @param target_version Target version for all rules
    /// @param operator_user User performing rollback
    /// @param reason Reason for rollback
    /// @return Operation record for coordinated rollback
    RollbackOperation performCoordinatedRollback(
        const std::vector<std::string>& rule_ids,
        const std::string& target_version,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /// Rollback to previous version
    /// @param rule_id Rule to rollback
    /// @param operator_user User performing rollback
    /// @param reason Reason for rollback
    /// @return Operation record
    RollbackOperation rollbackToPrevious(
        const std::string& rule_id,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /// Reverse a rollback operation (undo the rollback)
    /// @param operation_id Rollback operation to reverse
    /// @param operator_user User authorizing reversal
    /// @return New operation record for reversal
    std::optional<RollbackOperation> reverseRollback(
        const std::string& operation_id,
        const std::string& operator_user
    );
    
    /// Get rollback operation history
    /// @param rule_id Optional rule ID filter
    /// @param start_time Optional start time filter
    /// @param end_time Optional end time filter
    /// @return Vector of rollback operations
    std::vector<RollbackOperation> getRollbackHistory(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<int64_t>& start_time = std::nullopt,
        const std::optional<int64_t>& end_time = std::nullopt
    ) const;
    
    /// Get a specific rollback operation
    /// @param operation_id Operation identifier
    /// @return Operation details if found
    std::optional<RollbackOperation> getRollbackOperation(
        const std::string& operation_id
    ) const;
    
    /// Check if rollback is currently in progress
    /// @param rule_id Rule identifier
    /// @return True if rollback is in progress
    bool isRollbackInProgress(const std::string& rule_id) const;
    
    /// Export change management data as JSON
    nlohmann::json exportChangeData() const;
    
    /// Import change management data from JSON
    bool importChangeData(const nlohmann::json& j);
    
    /// Save change data to file
    bool saveToFile(const std::string& path) const;
    
    /// Load change data from file
    bool loadFromFile(const std::string& path);
    
    /// Clear all change records (for testing/cleanup)
    void clear();
    
private:
    mutable std::mutex mutex_;
    
    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<PolicyVersionHistory> version_history_;
    
    // Dependency graph
    std::unordered_map<std::string, std::vector<PolicyDependency>> dependencies_;
    std::unordered_map<std::string, std::vector<PolicyDependency>> reverse_dependencies_;
    
    // Rollback operation history
    std::vector<RollbackOperation> rollback_history_;
    
    // In-progress rollbacks (rule_id -> operation_id)
    std::unordered_map<std::string, std::string> in_progress_rollbacks_;
    
    /// Generate unique operation ID
    std::string generateOperationId();
    
    /// Check for circular dependencies
    bool hasCircularDependency(const std::string& rule_id) const;
    
    /// Find all rules that would be affected by rollback
    std::vector<std::string> findAffectedRules(const std::string& rule_id) const;
    
    /// Verify rollback can be applied atomically
    bool canApplyAtomically(const std::string& rule_id, const std::string& target_version);
    
    /// Execute rollback operation
    bool executeRollback(
        const std::string& rule_id,
        const std::string& target_version,
        RollbackOperation& operation
    );
};

} // namespace governance
} // namespace themis
