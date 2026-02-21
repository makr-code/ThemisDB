/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_manager_versioned.cpp                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:38:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     299                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 235d2ca7f  2026-02-10  Refactor tests and update dependencies   ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • b5959447b  2026-02-06  Implement GAP-004 Phase 5: Enterprise Policy Features wit... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/policy_manager_versioned.h"
#include "utils/logger.h"

#include <chrono>

namespace themis {
namespace governance {

PolicyManagerWithVersioning::PolicyManagerWithVersioning()
    : policy_manager_(std::make_shared<PolicyManager>())
    , version_history_(std::make_shared<PolicyVersionHistory>())
{
}

PolicyManagerWithVersioning::PolicyManagerWithVersioning(
    std::shared_ptr<PolicyManager> policy_manager
)
    : policy_manager_(std::move(policy_manager))
    , version_history_(std::make_shared<PolicyVersionHistory>())
{
    if (!policy_manager_) {
        policy_manager_ = std::make_shared<PolicyManager>();
    }
}

std::shared_ptr<PolicyManager> PolicyManagerWithVersioning::getPolicyManager() const {
    return policy_manager_;
}

std::shared_ptr<PolicyVersionHistory> PolicyManagerWithVersioning::getVersionHistory() const {
    return version_history_;
}

std::string PolicyManagerWithVersioning::addRuleVersioned(
    const PolicyRule& rule,
    const std::string& user,
    const std::string& change_description
) {
    // Set timestamps
    PolicyRule new_rule = rule;
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    new_rule.created_at = now;
    new_rule.updated_at = now;
    new_rule.created_by = user;
    new_rule.last_modified_by = user;
    new_rule.change_description = change_description;
    
    // Record version first to get the correct version number
    std::string version = version_history_->recordVersion(
        new_rule.id,
        new_rule,
        user,
        change_description
    );
    
    // Set the version on the rule before adding to policy manager
    new_rule.version = version;
    
    // Add rule to policy manager
    policy_manager_->addRule(new_rule);
    
    // Record audit
    recordAudit(new_rule.id, "create", user, "", version);
    
    THEMIS_INFO("Created rule {} version {} by user {}", new_rule.id, version, user);
    
    return version;
}

std::string PolicyManagerWithVersioning::updateRuleVersioned(
    const std::string& rule_id,
    const PolicyRule& rule,
    const std::string& user,
    const std::string& change_description
) {
    // Get current rule to preserve version history
    auto current_rule = policy_manager_->getRule(rule_id);
    std::string old_version = "0.0.0";
    if (current_rule.has_value()) {
        old_version = current_rule->version;
    }
    
    // Update timestamps and versioning info
    PolicyRule updated_rule = rule;
    updated_rule.id = rule_id; // Ensure ID stays the same
    updated_rule.updated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    updated_rule.last_modified_by = user;
    updated_rule.change_description = change_description;
    
    // Preserve creation info
    if (current_rule.has_value()) {
        updated_rule.created_by = current_rule->created_by;
        updated_rule.created_at = current_rule->created_at;
    }
    
    // Record version before updating
    std::string new_version = version_history_->recordVersion(
        rule_id,
        updated_rule,
        user,
        change_description
    );
    
    updated_rule.version = new_version;
    
    // Update rule in policy manager
    policy_manager_->addRule(updated_rule); // addRule replaces if exists
    
    // Record audit
    recordAudit(rule_id, "update", user, old_version, new_version);
    
    THEMIS_INFO("Updated rule {} from version {} to {} by user {}", 
        rule_id, old_version, new_version, user);
    
    return new_version;
}

void PolicyManagerWithVersioning::deleteRuleVersioned(
    const std::string& rule_id,
    const std::string& user
) {
    // Get current version before deletion
    auto rule = policy_manager_->getRule(rule_id);
    std::string version = "unknown";
    if (rule.has_value()) {
        version = rule->version;
    }
    
    // Remove from policy manager
    policy_manager_->removeRule(rule_id);
    
    // Record audit
    recordAudit(rule_id, "delete", user, version, "");
    
    THEMIS_INFO("Deleted rule {} version {} by user {}", rule_id, version, user);
}

bool PolicyManagerWithVersioning::rollbackToVersion(
    const std::string& rule_id,
    const std::string& target_version,
    const std::string& user
) {
    // Get target version
    auto version_data = version_history_->getVersion(rule_id, target_version);
    if (!version_data.has_value()) {
        THEMIS_ERROR("Cannot rollback: version {} of rule {} not found", target_version, rule_id);
        return false;
    }
    
    // Get current version for audit
    auto current_rule = policy_manager_->getRule(rule_id);
    std::string current_version = "unknown";
    if (current_rule.has_value()) {
        current_version = current_rule->version;
    }
    
    // To rollback, we need to fetch the rule by ID from the rule store
    // since we only store rule_id in PolicyRuleVersion
    auto target_rule = policy_manager_->getRule(version_data->rule_id);
    if (!target_rule.has_value()) {
        spdlog::error("Cannot rollback: rule {} not found", version_data->rule_id);
        return false;
    }
    
    // Update rule to target version
    PolicyRule rollback_rule = target_rule.value();
    rollback_rule.updated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rollback_rule.last_modified_by = user;
    rollback_rule.change_description = "Rolled back to version " + target_version;
    
    // Record new version for rollback
    std::string new_version = version_history_->recordVersion(
        rule_id,
        rollback_rule,
        user,
        "Rollback to version " + target_version
    );
    
    rollback_rule.version = new_version;
    
    // Update in policy manager
    policy_manager_->addRule(rollback_rule);
    
    // Record audit
    recordAudit(rule_id, "rollback", user, current_version, new_version);
    
    THEMIS_INFO("Rolled back rule {} from version {} to version {} (new version {})", 
        rule_id, current_version, target_version, new_version);
    
    return true;
}

bool PolicyManagerWithVersioning::rollbackToPreviousVersion(
    const std::string& rule_id,
    const std::string& user
) {
    auto prev_version = version_history_->getPreviousVersion(rule_id);
    if (!prev_version.has_value()) {
        THEMIS_ERROR("Cannot rollback: no previous version found for rule {}", rule_id);
        return false;
    }
    
    return rollbackToVersion(rule_id, *prev_version, user);
}

VersionDiff PolicyManagerWithVersioning::previewRollback(
    const std::string& rule_id,
    const std::string& target_version
) {
    // Get current rule
    auto current_rule = policy_manager_->getRule(rule_id);
    if (!current_rule.has_value()) {
        VersionDiff diff;
        diff.rule_id = rule_id;
        diff.changes.push_back("Current rule not found");
        return diff;
    }
    
    // Compare current with target version
    return version_history_->compareVersions(rule_id, current_rule->version, target_version);
}

std::vector<PolicyRuleVersion> PolicyManagerWithVersioning::getRuleVersions(
    const std::string& rule_id
) const {
    return version_history_->getVersions(rule_id);
}

std::optional<PolicyRuleVersion> PolicyManagerWithVersioning::getRuleVersion(
    const std::string& rule_id,
    const std::string& version
) const {
    return version_history_->getVersion(rule_id, version);
}

VersionDiff PolicyManagerWithVersioning::compareVersions(
    const std::string& rule_id,
    const std::string& version1,
    const std::string& version2
) const {
    return version_history_->compareVersions(rule_id, version1, version2);
}

std::vector<AuditLogEntry> PolicyManagerWithVersioning::queryAudit(
    const std::optional<std::string>& rule_id,
    const std::optional<std::string>& user,
    const std::optional<std::int64_t>& start_time,
    const std::optional<std::int64_t>& end_time
) const {
    return version_history_->queryAudit(rule_id, user, start_time, end_time);
}

bool PolicyManagerWithVersioning::loadVersionHistory(const std::string& path) {
    return version_history_->loadFromFile(path);
}

bool PolicyManagerWithVersioning::saveVersionHistory(const std::string& path) const {
    return version_history_->saveToFile(path);
}

void PolicyManagerWithVersioning::recordAudit(
    const std::string& rule_id,
    const std::string& operation,
    const std::string& user,
    const std::string& old_version,
    const std::string& new_version
) {
    AuditLogEntry entry;
    entry.rule_id = rule_id;
    entry.operation = operation;
    entry.user = user;
    entry.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    entry.old_version = old_version;
    entry.new_version = new_version;
    
    version_history_->recordAudit(entry);
}

} // namespace governance
} // namespace themis
