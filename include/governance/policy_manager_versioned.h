/**
 * @file policy_manager_versioned.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include "governance/policy_version_history.h"

#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/**
 * @brief Conflict information from real-time policy conflict detection.
 *
 * Populated by PolicyManagerWithVersioning::checkConflictsForRule() and
 * PolicyManagerWithVersioning::getActiveConflicts().
 */
struct ConflictInfo {
    std::string conflict_type;                          ///< "contradictory" or "overlapping"
    std::string severity;                               ///< "critical", "high", "medium", "low"
    std::string new_rule_id;                            ///< The rule that triggered detection
    std::vector<std::string> conflicting_rule_ids;      ///< Existing rules that conflict
    std::string description;                            ///< Human-readable description
    std::vector<std::string> resolution_suggestions;    ///< Concrete steps to resolve
    std::int64_t detected_at = 0;                       ///< Unix timestamp of detection

    /**
     * @brief TBD: Describe toJson.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Extended PolicyManager with versioning and audit capabilities
 * 
 * This class extends PolicyManager functionality with:
 * - Automatic version tracking for all rule changes
 * - Complete audit trail
 * - Rollback functionality
 * - Version comparison
 */
class PolicyManagerWithVersioning {
public:
    PolicyManagerWithVersioning();
    
    /**
     * @brief TBD: Describe PolicyManagerWithVersioning.
     * @param[in] policy_manager Input parameter.
     * @return Return value.
     */
    explicit PolicyManagerWithVersioning(
        std::shared_ptr<PolicyManager> policy_manager
    );
    
    /// Get the underlying PolicyManager
    std::shared_ptr<PolicyManager> getPolicyManager() const;
    
    /// Get the version history manager
    std::shared_ptr<PolicyVersionHistory> getVersionHistory() const;
    
    /**
     * @brief Add a rule with version tracking @param rule Rule to add @param user User creating the rule @param change_description Description of changes @return Version number assigned
     * @param[in] rule Input parameter.
     * @param[in] user Input parameter.
     * @param[in] change_description Input parameter.
     * @return Return value.
     */
    std::string addRuleVersioned(
        const PolicyRule& rule,
        const std::string& user,
        const std::string& change_description
    );
    
    /**
     * @brief Update a rule with version tracking @param rule_id Rule identifier @param rule Updated rule data @param user User making the update @param change_description Description of changes @return New version number
     * @param[in] rule_id Input parameter.
     * @param[in] rule Input parameter.
     * @param[in] user Input parameter.
     * @param[in] change_description Input parameter.
     * @return Return value.
     */
    std::string updateRuleVersioned(
        const std::string& rule_id,
        const PolicyRule& rule,
        const std::string& user,
        const std::string& change_description
    );
    
    /**
     * @brief Delete a rule with audit tracking @param rule_id Rule identifier @param user User deleting the rule
     * @param[in] rule_id Input parameter.
     * @param[in] user Input parameter.
     */
    void deleteRuleVersioned(
        const std::string& rule_id,
        const std::string& user
    );
    
    /**
     * @brief Rollback rule to a specific version @param rule_id Rule identifier @param target_version Version to rollback to @param user User performing rollback @return True if rollback succeeded
     * @param[in] rule_id Input parameter.
     * @param[in] target_version Input parameter.
     * @param[in] user Input parameter.
     * @return True on success.
     */
    bool rollbackToVersion(
        const std::string& rule_id,
        const std::string& target_version,
        const std::string& user
    );
    
    /**
     * @brief Rollback rule to previous version @param rule_id Rule identifier @param user User performing rollback @return True if rollback succeeded
     * @param[in] rule_id Input parameter.
     * @param[in] user Input parameter.
     * @return True on success.
     */
    bool rollbackToPreviousVersion(
        const std::string& rule_id,
        const std::string& user
    );
    
    /**
     * @brief Preview rollback changes without applying them @param rule_id Rule identifier @param target_version Version to preview @return Diff showing what would change
     * @param[in] rule_id Input parameter.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    VersionDiff previewRollback(
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /**
     * @brief Get all versions of a rule @param rule_id Rule identifier @return Vector of versions (newest first)
     * @param[in] rule_id Input parameter.
     * @return Return value.
     */
    std::vector<PolicyRuleVersion> getRuleVersions(const std::string& rule_id) const;
    
    /**
     * @brief Get a specific version of a rule @param rule_id Rule identifier @param version Version number @return Rule version if found
     * @param[in] rule_id Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<PolicyRuleVersion> getRuleVersion(
        const std::string& rule_id,
        const std::string& version
    ) const;
    
    /**
     * @brief Compare two versions of a rule @param rule_id Rule identifier @param version1 First version @param version2 Second version @return Differences between versions
     * @param[in] rule_id Input parameter.
     * @param[in] version1 Input parameter.
     * @param[in] version2 Input parameter.
     * @return Return value.
     */
    VersionDiff compareVersions(
        const std::string& rule_id,
        const std::string& version1,
        const std::string& version2
    ) const;
    
    /// Query audit trail
    /// @param rule_id Optional rule ID filter
    /// @param user Optional user filter
    /// @param start_time Optional start time filter
    /// @param end_time Optional end time filter
    /// @return Filtered audit log entries
    std::vector<AuditLogEntry> queryAudit(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<std::int64_t>& start_time = std::nullopt,
        const std::optional<std::int64_t>& end_time = std::nullopt
    ) const;
    
    /// Load versioning data from file
    bool loadVersionHistory(const std::string& path);
    
    /// Save versioning data to file
    bool saveVersionHistory(const std::string& path) const;
    
    /**
     * @brief Check a candidate rule for conflicts with all currently-active rules.
     * @param[in] rule Input parameter.
     * @return Return value.
     * @details Call this before or after adding/updating a rule to detect overlapping or contradictory access-control settings in real time. @param rule Rule to evaluate (may or may not already be in the rule set) @return List of detected ConflictInfo entries (empty when conflict-free)
     */
    std::vector<ConflictInfo> checkConflictsForRule(const PolicyRule& rule) const;

    /**
     * @brief Return the current, live conflict state across the entire rule set.
     * @return Return value.
     * @details All enabled rules are compared pairwise; the result reflects the state at the moment of the call and can be used to feed a real-time report. @return All detected ConflictInfo entries across the active rule set
     */
    std::vector<ConflictInfo> getActiveConflicts() const;

private:
    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<PolicyVersionHistory> version_history_;
    
    /// Helper: Record audit entry
    void recordAudit(
        const std::string& rule_id,
        const std::string& operation,
        const std::string& user,
        const std::string& old_version = "",
        const std::string& new_version = ""
    );
};

} // namespace governance
} // namespace themis
