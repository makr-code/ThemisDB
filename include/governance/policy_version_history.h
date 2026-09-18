/**
 * @file policy_version_history.h
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
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

// Forward declaration to avoid circular include
struct PolicyRule;

struct PolicyRuleVersion {
    std::string version;                               // Semantic version (major.minor.patch)
    std::string rule_id;                               // ID of the rule
    std::string author;                                // Who made this version
    std::int64_t timestamp;                            // When this version was created
    std::string change_description;                    // Description of changes
    nlohmann::json rule_snapshot;                      // Full rule state at this version (JSON)
    
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
    static PolicyRuleVersion fromJson(const nlohmann::json& j);
};

struct AuditLogEntry {
    std::string rule_id;                               // Rule being operated on
    std::string operation;                             // "create", "update", "delete", "rollback"
    std::string user;                                  // User performing the operation
    std::int64_t timestamp;                            // When the operation occurred
    std::string old_version;                           // Previous version (for update/rollback)
    std::string new_version;                           // New version (for update/create)
    nlohmann::json details;                            // Additional details
    
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
    static AuditLogEntry fromJson(const nlohmann::json& j);
};

struct VersionDiff {
    std::string rule_id;
    std::string version1;
    std::string version2;
    std::vector<std::string> changes;                  // List of changed fields
    nlohmann::json details;                            // Detailed differences
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class PolicyVersionHistory {
public:
    PolicyVersionHistory();
    
    /**
     * @brief Record Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] rule Input parameter.
     * @param[in] author Input parameter.
     * @param[in] change_description Input parameter.
     * @return Return value.
     */
    std::string recordVersion(
        const std::string& rule_id,
        const PolicyRule& rule,
        const std::string& author,
        const std::string& change_description
    );
    
    /**
     * @brief Get Versions.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::vector<PolicyRuleVersion> getVersions(const std::string& rule_id) const;
    
    /**
     * @brief Get Version.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<PolicyRuleVersion> getVersion(
        const std::string& rule_id,
        const std::string& version
    ) const;
    
    /**
     * @brief Get Latest Version.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::string getLatestVersion(const std::string& rule_id) const;
    
    /**
     * @brief Get Previous Version.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::optional<std::string> getPreviousVersion(const std::string& rule_id) const;

    /**
     * @brief Get Last Recorded Version.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    std::string getLastRecordedVersion(const std::string& rule_id) const;
    
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

    /**
     * @brief Compare Rules.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return Return value.
     */
    VersionDiff compareRules(const PolicyRule& rule1, const PolicyRule& rule2) const;
    
    /**
     * @brief Record Audit.
     * @param[in] entry Input parameter.
     */
    void recordAudit(const AuditLogEntry& entry);
    
    std::vector<AuditLogEntry> queryAudit(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<std::int64_t>& start_time = std::nullopt,
        const std::optional<std::int64_t>& end_time = std::nullopt
    ) const;
    
    /**
     * @brief Delete Version History.
     * @param[in] rule_id Identifier of the rule.
     */
    void deleteVersionHistory(const std::string& rule_id);
    
    /**
     * @brief Export History.
     * @return Return value.
     */
    nlohmann::json exportHistory() const;
    
    /**
     * @brief Import History.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importHistory(const nlohmann::json& j);
    
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
    
private:
    mutable std::mutex mutex_;
    
    // Map: rule_id -> versions (ordered by timestamp)
    std::unordered_map<std::string, std::vector<PolicyRuleVersion>> versions_;
    
    // Audit log (ordered by timestamp)
    std::vector<AuditLogEntry> audit_log_;
    
    /**
     * @brief Increment Version.
     * @param[in] current_version Input parameter.
     * @return Return value.
     */
    std::string incrementVersion(const std::string& current_version) const;
    
    /**
     * @brief Identify Changes.
     * @param[in] rule1 Input parameter.
     * @param[in] rule2 Input parameter.
     * @return Return value.
     */
    std::vector<std::string> identifyChanges(
        const PolicyRule& rule1,
        const PolicyRule& rule2
    ) const;
};

} // namespace governance
} // namespace themis
