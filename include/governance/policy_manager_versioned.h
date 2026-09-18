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

struct ConflictInfo {
    std::string conflict_type;                          ///< "contradictory" or "overlapping"
    std::string severity;                               ///< "critical", "high", "medium", "low"
    std::string new_rule_id;                            ///< The rule that triggered detection
    std::vector<std::string> conflicting_rule_ids;      ///< Existing rules that conflict
    std::string description;                            ///< Human-readable description
    std::vector<std::string> resolution_suggestions;    ///< Concrete steps to resolve
    std::int64_t detected_at = 0;                       ///< Unix timestamp of detection

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class PolicyManagerWithVersioning {
public:
    PolicyManagerWithVersioning();
    
    /**
     * @brief Policy Manager With Versioning.
     * @param[in] policy_manager Input parameter.
     * @return Return value.
     */
    explicit PolicyManagerWithVersioning(
        std::shared_ptr<PolicyManager> policy_manager
    );
    
    /**
     * @brief Get Policy Manager.
     * @return Return value.
     */
    std::shared_ptr<PolicyManager> getPolicyManager() const;
    
    /**
     * @brief Get Version History.
     * @return Return value.
     */
    std::shared_ptr<PolicyVersionHistory> getVersionHistory() const;
    
    /**
     * @brief Add Rule Versioned.
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
     * @brief Update Rule Versioned.
     * @param[in] rule_id Identifier of the rule.
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
     * @brief Delete Rule Versioned.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] user Input parameter.
     */
    void deleteRuleVersioned(
        const std::string& rule_id,
        const std::string& user
    );
    
    /**
     * @brief Rollback To Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @param[in] user Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackToVersion(
        const std::string& rule_id,
        const std::string& target_version,
        const std::string& user
    );
    
    /**
     * @brief Rollback To Previous Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] user Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackToPreviousVersion(
        const std::string& rule_id,
        const std::string& user
    );
    
    /**
     * @brief Preview Rollback.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    VersionDiff previewRollback(
        const std::string& rule_id,
        const std::string& target_version
    );
    
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
        const std::string& rule_id,
        const std::string& version
    ) const;
    
    /**
     * @brief Compare Versions.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version1 Input parameter.
     * @param[in] version2 Input parameter.
     * @return Return value.
     */
    VersionDiff compareVersions(
        const std::string& rule_id,
        const std::string& version1,
        const std::string& version2
    ) const;
    
    std::vector<AuditLogEntry> queryAudit(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<std::int64_t>& start_time = std::nullopt,
        const std::optional<std::int64_t>& end_time = std::nullopt
    ) const;
    
    /**
     * @brief Load Version History.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadVersionHistory(const std::string& path);
    
    /**
     * @brief Save Version History.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool saveVersionHistory(const std::string& path) const;
    
    /**
     * @brief Check Conflicts For Rule.
     * @param[in] rule Input parameter.
     * @return Return value.
     */
    std::vector<ConflictInfo> checkConflictsForRule(const PolicyRule& rule) const;

    /**
     * @brief Get Active Conflicts.
     * @return Return value.
     */
    std::vector<ConflictInfo> getActiveConflicts() const;

private:
    std::shared_ptr<PolicyManager> policy_manager_;
    std::shared_ptr<PolicyVersionHistory> version_history_;
    
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
