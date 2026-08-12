/**
 * @file policy_manager_versioned.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_manager_versioned.h"

#include <chrono>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== ConflictInfo Implementation ==========

nlohmann::json ConflictInfo::toJson() const {
    nlohmann::json j;
    j["conflict_type"]          = conflict_type;
    j["severity"]               = severity;
    j["new_rule_id"]            = new_rule_id;
    j["conflicting_rule_ids"]   = conflicting_rule_ids;
    j["description"]            = description;
    j["resolution_suggestions"] = resolution_suggestions;
    j["detected_at"]            = detected_at;
    return j;
}

// ========== Helpers ==========

/// True if two rules overlap on at least one resource AND one action pattern.
static bool rulesOverlap(const PolicyRule &a, const PolicyRule &b) {
    bool res_overlap = false;
    for (const auto &ra : a.resources) {
        for (const auto &rb : b.resources) {
            if (ra == rb || ra == "*" || rb == "*") {
                res_overlap = true;
                break;
            }
        }
        if (res_overlap) {
            break;
        }
    }
    if (!res_overlap) {
        return false;
    }

    for (const auto &aa : a.actions) {
        for (const auto &ab : b.actions) {
            if (aa == ab || aa == "*" || ab == "*") {
                return true;
            }
        }
    }
    return false;
}

/// Build a ConflictInfo for a contradictory pair of rules.
static ConflictInfo makeContradictoryConflict(const PolicyRule &new_rule, const PolicyRule &existing) {
    ConflictInfo conflict;
    conflict.conflict_type        = "contradictory";
    conflict.new_rule_id          = new_rule.id;
    conflict.conflicting_rule_ids = {existing.id};
    conflict.detected_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (new_rule.require_encryption != existing.require_encryption) {
        conflict.severity = "critical";
        conflict.description
            = "Conflicting encryption requirements between rules '" + new_rule.name + "' and '" + existing.name + "'";
        conflict.resolution_suggestions = {"Enforce encryption requirement consistently across overlapping rules",
                                           "Narrow the resource scope of one rule to eliminate the overlap",
                                           "Use rule priority to establish clear enforcement precedence"};
    } else if (new_rule.allow_export != existing.allow_export) {
        conflict.severity = "high";
        conflict.description
            = "Conflicting export permissions between rules '" + new_rule.name + "' and '" + existing.name + "'";
        conflict.resolution_suggestions = {"Align export permissions or narrow rule scope",
                                           "Assign a higher priority to the rule with the stricter export policy",
                                           "Consolidate rules to avoid overlapping export settings"};
    } else {
        // allow_cache conflict
        conflict.severity = "medium";
        conflict.description
            = "Conflicting cache permissions between rules '" + new_rule.name + "' and '" + existing.name + "'";
        conflict.resolution_suggestions
            = {"Align cache permissions or narrow rule scope", "Use rule priority to resolve cache policy ambiguity"};
    }
    return conflict;
}

/// Build a ConflictInfo for two overlapping rules with the same priority.
static ConflictInfo makeOverlappingConflict(const PolicyRule &new_rule, const PolicyRule &existing) {
    ConflictInfo conflict;
    conflict.conflict_type        = "overlapping";
    conflict.severity             = "low";
    conflict.new_rule_id          = new_rule.id;
    conflict.conflicting_rule_ids = {existing.id};
    conflict.detected_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    conflict.description            = "Rules '" + new_rule.name + "' and '" + existing.name
                                      + "' overlap with the same priority, creating evaluation ambiguity";
    conflict.resolution_suggestions = {"Assign distinct priorities to establish a clear evaluation order",
                                       "Narrow the resource patterns to eliminate the overlap",
                                       "Consider merging rules if they have identical effects"};
    return conflict;
}

PolicyManagerWithVersioning::PolicyManagerWithVersioning()
    : policy_manager_(std::make_shared<PolicyManager>()), version_history_(std::make_shared<PolicyVersionHistory>()) {}

PolicyManagerWithVersioning::PolicyManagerWithVersioning(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager)), version_history_(std::make_shared<PolicyVersionHistory>()) {
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

std::string PolicyManagerWithVersioning::addRuleVersioned(const PolicyRule &rule, const std::string &user,
                                                          const std::string &change_description) {
    // Set timestamps
    PolicyRule new_rule         = rule;
    auto now                    = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    new_rule.created_at         = now;
    new_rule.updated_at         = now;
    new_rule.created_by         = user;
    new_rule.last_modified_by   = user;
    new_rule.change_description = change_description;

    // Record version first to get the correct version number
    std::string version = version_history_->recordVersion(new_rule.id, new_rule, user, change_description);

    // Set the version on the rule before adding to policy manager
    new_rule.version = version;

    // Add rule to policy manager
    policy_manager_->addRule(new_rule);

    // Detect conflicts introduced by the new rule and emit warnings
    auto conflicts = checkConflictsForRule(new_rule);
    for (const auto &c : conflicts) {
        THEMIS_WARN("Policy conflict detected after adding rule {}: {} (severity: {})", new_rule.id, c.description,
                    c.severity);
    }

    // Record audit
    recordAudit(new_rule.id, "create", user, "", version);

    THEMIS_INFO("Created rule {} version {} by user {}", new_rule.id, version, user);

    return version;
}

std::string PolicyManagerWithVersioning::updateRuleVersioned(const std::string &rule_id, const PolicyRule &rule,
                                                             const std::string &user,
                                                             const std::string &change_description) {
    // Get current rule to preserve version history
    auto current_rule       = policy_manager_->getRule(rule_id);
    std::string old_version = "0.0.0";
    if (current_rule.has_value()) {
        old_version = current_rule->version;
    }

    // Update timestamps and versioning info
    PolicyRule updated_rule         = rule;
    updated_rule.id                 = rule_id; // Ensure ID stays the same
    updated_rule.updated_at         = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    updated_rule.last_modified_by   = user;
    updated_rule.change_description = change_description;

    // Preserve creation info
    if (current_rule.has_value()) {
        updated_rule.created_by = current_rule->created_by;
        updated_rule.created_at = current_rule->created_at;
    }

    // Record version before updating
    std::string new_version = version_history_->recordVersion(rule_id, updated_rule, user, change_description);

    updated_rule.version = new_version;

    // Update rule in policy manager
    policy_manager_->addRule(updated_rule); // addRule replaces if exists

    // Detect conflicts introduced by the updated rule and emit warnings
    auto conflicts = checkConflictsForRule(updated_rule);
    for (const auto &c : conflicts) {
        THEMIS_WARN("Policy conflict detected after updating rule {}: {} (severity: {})", rule_id, c.description,
                    c.severity);
    }

    // Record audit
    recordAudit(rule_id, "update", user, old_version, new_version);

    THEMIS_INFO("Updated rule {} from version {} to {} by user {}", rule_id, old_version, new_version, user);

    return new_version;
}

void PolicyManagerWithVersioning::deleteRuleVersioned(const std::string &rule_id, const std::string &user) {
    // Get current version before deletion
    auto rule           = policy_manager_->getRule(rule_id);
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

bool PolicyManagerWithVersioning::rollbackToVersion(const std::string &rule_id, const std::string &target_version,
                                                    const std::string &user) {
    // Get target version
    auto version_data = version_history_->getVersion(rule_id, target_version);
    if (!version_data.has_value()) {
        THEMIS_ERROR("Cannot rollback: version {} of rule {} not found", target_version, rule_id);
        return false;
    }

    // Get current version for audit
    auto current_rule           = policy_manager_->getRule(rule_id);
    std::string current_version = "unknown";
    if (current_rule.has_value()) {
        current_version = current_rule->version;
    }

    // Validate that the version has a snapshot to restore
    if (version_data->rule_snapshot.empty()) {
        THEMIS_ERROR("Cannot rollback: version {} of rule {} has no snapshot", target_version, rule_id);
        return false;
    }

    // Restore the rule state from the stored snapshot
    PolicyRule rollback_rule = PolicyRule::fromJson(version_data->rule_snapshot);
    if (rollback_rule.id != rule_id) {
        THEMIS_WARN("Snapshot ID mismatch for rule {}: snapshot contains '{}', overriding", rule_id, rollback_rule.id);
    }
    rollback_rule.id                 = rule_id; // Ensure ID doesn't change
    rollback_rule.updated_at         = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rollback_rule.last_modified_by   = user;
    rollback_rule.change_description = "Rolled back to version " + target_version;

    // Record new version for rollback
    std::string new_version
        = version_history_->recordVersion(rule_id, rollback_rule, user, "Rollback to version " + target_version);

    rollback_rule.version = new_version;

    // Update in policy manager
    policy_manager_->addRule(rollback_rule);

    // Record audit
    recordAudit(rule_id, "rollback", user, current_version, new_version);

    THEMIS_INFO("Rolled back rule {} from version {} to version {} (new version {})", rule_id, current_version,
                target_version, new_version);

    return true;
}

bool PolicyManagerWithVersioning::rollbackToPreviousVersion(const std::string &rule_id, const std::string &user) {
    auto prev_version = version_history_->getPreviousVersion(rule_id);
    if (!prev_version.has_value()) {
        THEMIS_ERROR("Cannot rollback: no previous version found for rule {}", rule_id);
        return false;
    }

    return rollbackToVersion(rule_id, *prev_version, user);
}

VersionDiff PolicyManagerWithVersioning::previewRollback(const std::string &rule_id,
                                                         const std::string &target_version) {
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

std::vector<PolicyRuleVersion> PolicyManagerWithVersioning::getRuleVersions(const std::string &rule_id) const {
    return version_history_->getVersions(rule_id);
}

std::optional<PolicyRuleVersion> PolicyManagerWithVersioning::getRuleVersion(const std::string &rule_id,
                                                                             const std::string &version) const {
    return version_history_->getVersion(rule_id, version);
}

VersionDiff PolicyManagerWithVersioning::compareVersions(const std::string &rule_id, const std::string &version1,
                                                         const std::string &version2) const {
    return version_history_->compareVersions(rule_id, version1, version2);
}

std::vector<AuditLogEntry> PolicyManagerWithVersioning::queryAudit(const std::optional<std::string> &rule_id,
                                                                   const std::optional<std::string> &user,
                                                                   const std::optional<std::int64_t> &start_time,
                                                                   const std::optional<std::int64_t> &end_time) const {
    return version_history_->queryAudit(rule_id, user, start_time, end_time);
}

bool PolicyManagerWithVersioning::loadVersionHistory(const std::string &path) {
    return version_history_->loadFromFile(path);
}

bool PolicyManagerWithVersioning::saveVersionHistory(const std::string &path) const {
    return version_history_->saveToFile(path);
}

std::vector<ConflictInfo> PolicyManagerWithVersioning::checkConflictsForRule(const PolicyRule &new_rule) const {
    std::vector<ConflictInfo> conflicts;

    auto existing_rules = policy_manager_->listRules();

    for (const auto &existing : existing_rules) {
        if (existing.id == new_rule.id) {
            continue; // skip self
        }
        if (!existing.enabled) {
            continue;
        }

        if (!rulesOverlap(new_rule, existing)) {
            continue;
        }

        // Contradictory effects take precedence over overlap reporting
        bool contradictory = (new_rule.require_encryption != existing.require_encryption)
                             || (new_rule.allow_export != existing.allow_export)
                             || (new_rule.allow_cache != existing.allow_cache);

        if (contradictory) {
            conflicts.push_back(makeContradictoryConflict(new_rule, existing));
        } else if (new_rule.priority == existing.priority) {
            conflicts.push_back(makeOverlappingConflict(new_rule, existing));
        }
    }

    if (!conflicts.empty()) {
        THEMIS_WARN("Rule '{}' has {} conflict(s) with existing rules", new_rule.id, conflicts.size());
    }
    return conflicts;
}

std::vector<ConflictInfo> PolicyManagerWithVersioning::getActiveConflicts() const {
    std::vector<ConflictInfo> all_conflicts;

    auto rules = policy_manager_->listRules();

    for (std::size_t i = 0; i < rules.size(); ++i) {
        if (!rules[i].enabled) {
            continue;
        }

        for (std::size_t j = i + 1; j < rules.size(); ++j) {
            if (!rules[j].enabled) {
                continue;
            }

            if (!rulesOverlap(rules[i], rules[j])) {
                continue;
            }

            bool contradictory = (rules[i].require_encryption != rules[j].require_encryption)
                                 || (rules[i].allow_export != rules[j].allow_export)
                                 || (rules[i].allow_cache != rules[j].allow_cache);

            if (contradictory) {
                // Treat rules[i] as the "newer" perspective for the report
                all_conflicts.push_back(makeContradictoryConflict(rules[i], rules[j]));
            } else if (rules[i].priority == rules[j].priority) {
                all_conflicts.push_back(makeOverlappingConflict(rules[i], rules[j]));
            }
        }
    }

    THEMIS_DEBUG("getActiveConflicts: {} conflict(s) across {} rules", all_conflicts.size(), rules.size());
    return all_conflicts;
}

void PolicyManagerWithVersioning::recordAudit(const std::string &rule_id, const std::string &operation,
                                              const std::string &user, const std::string &old_version,
                                              const std::string &new_version) {
    AuditLogEntry entry;
    entry.rule_id     = rule_id;
    entry.operation   = operation;
    entry.user        = user;
    entry.timestamp   = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    entry.old_version = old_version;
    entry.new_version = new_version;

    version_history_->recordAudit(entry);
}

} // namespace governance
} // namespace themis
