/**
 * @file policy_version_history.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Represents a versioned snapshot of a PolicyRule
struct PolicyRuleVersion {
    std::string version;                               // Semantic version (major.minor.patch)
    std::string rule_id;                               // ID of the rule
    std::string author;                                // Who made this version
    std::int64_t timestamp;                            // When this version was created
    std::string change_description;                    // Description of changes
    nlohmann::json rule_snapshot;                      // Full rule state at this version (JSON)
    
    nlohmann::json toJson() const;
    static PolicyRuleVersion fromJson(const nlohmann::json& j);
};

/// Audit log entry for rule operations
struct AuditLogEntry {
    std::string rule_id;                               // Rule being operated on
    std::string operation;                             // "create", "update", "delete", "rollback"
    std::string user;                                  // User performing the operation
    std::int64_t timestamp;                            // When the operation occurred
    std::string old_version;                           // Previous version (for update/rollback)
    std::string new_version;                           // New version (for update/create)
    nlohmann::json details;                            // Additional details
    
    nlohmann::json toJson() const;
    static AuditLogEntry fromJson(const nlohmann::json& j);
};

/// Version difference between two rule versions
struct VersionDiff {
    std::string rule_id;
    std::string version1;
    std::string version2;
    std::vector<std::string> changes;                  // List of changed fields
    nlohmann::json details;                            // Detailed differences
    
    nlohmann::json toJson() const;
};

/// Manages version history for policy rules
class PolicyVersionHistory {
public:
    PolicyVersionHistory();
    
    /// Record a new version of a rule
    /// @param rule_id Rule identifier
    /// @param rule Current rule state
    /// @param author User creating this version
    /// @param change_description Description of changes
    /// @return New version number
    std::string recordVersion(
        const std::string& rule_id,
        const PolicyRule& rule,
        const std::string& author,
        const std::string& change_description
    );
    
    /// Get all versions of a rule
    /// @param rule_id Rule identifier
    /// @return Vector of all versions, ordered newest first
    std::vector<PolicyRuleVersion> getVersions(const std::string& rule_id) const;
    
    /// Get a specific version of a rule
    /// @param rule_id Rule identifier
    /// @param version Version number
    /// @return Rule version if found
    std::optional<PolicyRuleVersion> getVersion(
        const std::string& rule_id,
        const std::string& version
    ) const;
    
    /// Get the latest version number for a rule
    /// @param rule_id Rule identifier
    /// @return Latest version number or "0.0.0" if no versions exist
    std::string getLatestVersion(const std::string& rule_id) const;
    
    /// Get the previous version number for a rule
    /// @param rule_id Rule identifier
    /// @return Previous version number or empty if no previous version
    std::optional<std::string> getPreviousVersion(const std::string& rule_id) const;

    /// Get the last recorded version number (most recent entry in history)
    /// @param rule_id Rule identifier
    /// @return Last recorded version string, or empty string if no history
    std::string getLastRecordedVersion(const std::string& rule_id) const;
    
    /// Compare two versions of a rule
    /// @param rule_id Rule identifier
    /// @param version1 First version
    /// @param version2 Second version
    /// @return Differences between versions
    VersionDiff compareVersions(
        const std::string& rule_id,
        const std::string& version1,
        const std::string& version2
    ) const;

    /// Compare two PolicyRule objects directly and return field-level differences.
    /// @param rule1 First rule
    /// @param rule2 Second rule
    /// @return VersionDiff populated with rule_id, versions, and changed field names
    VersionDiff compareRules(const PolicyRule& rule1, const PolicyRule& rule2) const;
    
    /// Record an audit log entry
    /// @param entry Audit log entry
    void recordAudit(const AuditLogEntry& entry);
    
    /// Query audit log
    /// @param rule_id Optional rule ID filter
    /// @param user Optional user filter
    /// @param start_time Optional start time filter (unix timestamp)
    /// @param end_time Optional end time filter (unix timestamp)
    /// @return Filtered audit log entries
    std::vector<AuditLogEntry> queryAudit(
        const std::optional<std::string>& rule_id = std::nullopt,
        const std::optional<std::string>& user = std::nullopt,
        const std::optional<std::int64_t>& start_time = std::nullopt,
        const std::optional<std::int64_t>& end_time = std::nullopt
    ) const;
    
    /// Delete all versions of a rule (for cleanup)
    /// @param rule_id Rule identifier
    void deleteVersionHistory(const std::string& rule_id);
    
    /// Export version history as JSON
    nlohmann::json exportHistory() const;
    
    /// Import version history from JSON
    bool importHistory(const nlohmann::json& j);
    
    /// Save version history to file
    bool saveToFile(const std::string& path) const;
    
    /// Load version history from file
    bool loadFromFile(const std::string& path);
    
private:
    mutable std::mutex mutex_;
    
    // Map: rule_id -> versions (ordered by timestamp)
    std::unordered_map<std::string, std::vector<PolicyRuleVersion>> versions_;
    
    // Audit log (ordered by timestamp)
    std::vector<AuditLogEntry> audit_log_;
    
    /// Helper: increment version number
    std::string incrementVersion(const std::string& current_version) const;
    
    /// Helper: compare two PolicyRules and identify differences
    std::vector<std::string> identifyChanges(
        const PolicyRule& rule1,
        const PolicyRule& rule2
    ) const;
};

} // namespace governance
} // namespace themis
