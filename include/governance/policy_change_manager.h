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

enum class RollbackSafetyLevel {
    SAFE              = 0,  ///< Rollback is safe with no conflicts
    WARNING           = 1,  ///< Rollback has minor concerns
    BLOCKED           = 2,  ///< Rollback is unsafe and cannot proceed
};

struct PolicyDependency {
    std::string dependent_rule_id;      ///< Rule depending on another
    std::string dependency_rule_id;     ///< Rule being depended on
    std::string dependency_type;        ///< "enforcement", "inheritance", "composition"
    std::string reason;                 ///< Description of dependency
    
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
    static PolicyDependency fromJson(const nlohmann::json& j);
};

struct RollbackSafetyReport {
    std::string rule_id;                ///< Rule being rolled back
    std::string target_version;         ///< Target rollback version
    RollbackSafetyLevel safety_level;   ///< Safety assessment
    
    std::vector<std::string> conflicts; ///< Conflicting rule IDs
    std::vector<std::string> warnings;  ///< Warning messages
    std::vector<PolicyDependency> affected_dependencies;  ///< Dependencies that would be affected
    
    int estimated_duration_ms = 0;      ///< Estimated rollback duration
    bool is_reversible = true;          ///< Whether rollback can be reversed
    
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
    static RollbackSafetyReport fromJson(const nlohmann::json& j);
};

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
    static RollbackOperation fromJson(const nlohmann::json& j);
};

class PolicyChangeManager {
public:
    PolicyChangeManager();
    
    /**
     * @brief Policy Change Manager.
     * @param[in] policy_manager Input parameter.
     * @param[in] version_history Input parameter.
     * @return Return value.
     */
    explicit PolicyChangeManager(
        std::shared_ptr<PolicyManager> policy_manager,
        std::shared_ptr<PolicyVersionHistory> version_history
    );
    
    /**
     * @brief Register Dependency.
     * @param[in] dependent_rule_id Identifier of the dependent rule.
     * @param[in] dependency_rule_id Identifier of the dependency rule.
     * @param[in] dependency_type Input parameter.
     * @param[in] reason Input parameter.
     */
    void registerDependency(
        const std::string& dependent_rule_id,
        const std::string& dependency_rule_id,
        const std::string& dependency_type,
        const std::string& reason
    );
    
    /**
     * @brief Get Dependencies.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<PolicyDependency> getDependencies(const std::string& rule_id) const;
    
    /**
     * @brief Get Reverse Dependencies.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<PolicyDependency> getReverseDependencies(const std::string& rule_id) const;
    
    /**
     * @brief Check Rollback Safety.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    RollbackSafetyReport checkRollbackSafety(
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /**
     * @brief Preview Rollback.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    RollbackSafetyReport previewRollback(
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /**
     * @brief Perform Rollback.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @param[in] operator_user Input parameter.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    RollbackOperation performRollback(
        const std::string& rule_id,
        const std::string& target_version,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /**
     * @brief Perform Coordinated Rollback.
     * @param[in] rule_ids Input parameter.
     * @param[in] target_version Input parameter.
     * @param[in] operator_user Input parameter.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    RollbackOperation performCoordinatedRollback(
        const std::vector<std::string>& rule_ids,
        const std::string& target_version,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /**
     * @brief Rollback To Previous.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] operator_user Input parameter.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    RollbackOperation rollbackToPrevious(
        const std::string& rule_id,
        const std::string& operator_user,
        const std::string& reason
    );
    
    /**
     * @brief Reverse Rollback.
     * @param[in] operation_id Identifier of the operation.
     * @param[in] operator_user Input parameter.
     * @return Return value.
     */
    std::optional<RollbackOperation> reverseRollback(
        const std::string& operation_id,
        const std::string& operator_user
    );
    
    std::vector<RollbackOperation> getRollbackHistory(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<int64_t>& start_time = std::nullopt,
        const std::optional<int64_t>& end_time = std::nullopt
    ) const;
    
    /**
     * @brief Get Rollback Operation.
     * @param[in] operation_id Identifier of the operation.
     * @return Return value.
     */
    std::optional<RollbackOperation> getRollbackOperation(
        const std::string& operation_id
    ) const;
    
    /**
     * @brief Is Rollback In Progress.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool isRollbackInProgress(const std::string& rule_id) const;
    
    /**
     * @brief Export Change Data.
     * @return Return value.
     */
    nlohmann::json exportChangeData() const;
    
    /**
     * @brief Import Change Data.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importChangeData(const nlohmann::json& j);
    
    /**
     * @brief Save To File.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveToFile(const std::string& path) const;
    
    /**
     * @brief Load From File.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * @brief Clear.
     */
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
    
    /**
     * @brief Generate Operation Id.
     * @return Return value.
     */
    std::string generateOperationId();
    
    /**
     * @brief Has Circular Dependency.
     * @param[in] rule_id Identifier of the rule.
     * @return True when the operation succeeds.
     */
    bool hasCircularDependency(const std::string& rule_id) const;
    
    /**
     * @brief Find Affected Rules.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<std::string> findAffectedRules(const std::string& rule_id) const;
    
    /**
     * @brief Can Apply Atomically.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return True when the operation succeeds.
     */
    bool canApplyAtomically(const std::string& rule_id, const std::string& target_version);
    
    /**
     * @brief Execute Rollback.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @param[in,out] operation Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool executeRollback(
        const std::string& rule_id,
        const std::string& target_version,
        RollbackOperation& operation
    );
};

} // namespace governance
} // namespace themis
