/**
 * @file policy_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <numeric>
#include <shared_mutex>
#include <yaml-cpp/yaml.h>

#include "governance/policy_validator.h"
#include "observability/metrics_collector.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== PolicyLifecycle Implementation ==========

bool PolicyLifecycle::canTransitionTo(PolicyState target_state) const {
    // Valid transitions:
    // DRAFT -> ACTIVE
    // ACTIVE -> DEPRECATED or RETIRED
    // DEPRECATED -> RETIRED
    // Others invalid (no RETIRED -> X, no backwards)
    
    if (current_state == PolicyState::DRAFT) {
        return target_state == PolicyState::ACTIVE;
    }
    if (current_state == PolicyState::ACTIVE) {
        return target_state == PolicyState::DEPRECATED || 
               target_state == PolicyState::RETIRED;
    }
    if (current_state == PolicyState::DEPRECATED) {
        return target_state == PolicyState::RETIRED;
    }
    return false;  // RETIRED is terminal
}

std::string PolicyLifecycle::getStateDescription() const {
    switch(current_state) {
        case PolicyState::DRAFT:
            return "state=DRAFT: policy drafted, not yet activated";
        case PolicyState::ACTIVE:
            return "state=ACTIVE: policy actively enforced";
        case PolicyState::DEPRECATED:
            return "state=DEPRECATED: deprecated policy retained for audit only";
        case PolicyState::RETIRED:
            return "state=RETIRED: policy archived";
        default:
            return "state=UNKNOWN: unknown policy state";
    }
}

// ========== PolicyRule Implementation ==========

nlohmann::json PolicyRule::toJson() const {
    nlohmann::json j;
    j["id"]                   = id;
    j["name"]                 = name;
    j["description"]          = description;
    j["classification_level"] = classification_level;
    j["enabled"]              = enabled;
    j["resources"]            = resources;
    j["actions"]              = actions;
    j["required_roles"]       = required_roles;
    j["require_encryption"]   = require_encryption;
    j["require_signature"]    = require_signature;
    j["allow_export"]         = allow_export;
    j["allow_cache"]          = allow_cache;
    j["retention_days"]       = retention_days;
    j["redaction_level"]      = redaction_level;
    j["audit_access"]         = audit_access;
    j["audit_changes"]        = audit_changes;
    j["priority"]             = priority;
    j["created_by"]           = created_by;
    j["created_at"]           = created_at;
    j["updated_at"]           = updated_at;
    j["version"]              = version;
    j["last_modified_by"]     = last_modified_by;
    j["change_description"]   = change_description;
    return j;
}

PolicyRule PolicyRule::fromJson(const nlohmann::json &j) {
    PolicyRule rule = {};
    if (j.contains("id")) {
        rule.id = j["id"].get<std::string>();
    }
    if (j.contains("name")) {
        rule.name = j["name"].get<std::string>();
    }
    if (j.contains("description")) {
        rule.description = j["description"].get<std::string>();
    }
    if (j.contains("classification_level")) {
        rule.classification_level = j["classification_level"].get<std::string>();
    }
    if (j.contains("enabled")) {
        rule.enabled = j["enabled"].get<bool>();
    }
    if (j.contains("resources")) {
        rule.resources = j["resources"].get<std::vector<std::string>>();
    }
    if (j.contains("actions")) {
        rule.actions = j["actions"].get<std::vector<std::string>>();
    }
    if (j.contains("required_roles")) {
        rule.required_roles = j["required_roles"].get<std::vector<std::string>>();
    }
    if (j.contains("require_encryption")) {
        rule.require_encryption = j["require_encryption"].get<bool>();
    }
    if (j.contains("require_signature")) {
        rule.require_signature = j["require_signature"].get<bool>();
    }
    if (j.contains("allow_export")) {
        rule.allow_export = j["allow_export"].get<bool>();
    }
    if (j.contains("allow_cache")) {
        rule.allow_cache = j["allow_cache"].get<bool>();
    }
    if (j.contains("retention_days")) {
        rule.retention_days = j["retention_days"].get<int>();
    }
    if (j.contains("redaction_level")) {
        rule.redaction_level = j["redaction_level"].get<std::string>();
    }
    if (j.contains("audit_access")) {
        rule.audit_access = j["audit_access"].get<bool>();
    }
    if (j.contains("audit_changes")) {
        rule.audit_changes = j["audit_changes"].get<bool>();
    }
    if (j.contains("priority")) {
        rule.priority = j["priority"].get<int>();
    }
    if (j.contains("created_by")) {
        rule.created_by = j["created_by"].get<std::string>();
    }
    if (j.contains("created_at")) {
        rule.created_at = j["created_at"].get<int64_t>();
    }
    if (j.contains("updated_at")) {
        rule.updated_at = j["updated_at"].get<int64_t>();
    }
    if (j.contains("version")) {
        rule.version = j["version"].get<std::string>();
    }
    if (j.contains("last_modified_by")) {
        rule.last_modified_by = j["last_modified_by"].get<std::string>();
    }
    if (j.contains("change_description")) {
        rule.change_description = j["change_description"].get<std::string>();
    }
    return rule;
}

bool PolicyRule::appliesTo(const std::string &resource, const std::string &action) const {
    if (!enabled) {
        return false;
    }

    // Check resource match
    bool resource_match = resources.empty(); // Empty means all resources
    for (const auto &pattern : resources) {
        if (pattern == "*" || pattern == resource) {
            resource_match = true;
            break;
        }
        // Simple wildcard matching: "data/*" matches "data/anything"
        if (pattern.back() == '*' && pattern.size() > 1) {
            std::string prefix = pattern.substr(0, static_cast<int>(pattern.size()) - 1);
            if (resource.find(prefix) == 0) {
                resource_match = true;
                break;
            }
        }
    }

    if (!resource_match) {
        return false;
    }

    // Check action match
    bool action_match = actions.empty(); // Empty means all actions
    for (const auto &pattern : actions) {
        if (pattern == "*" || pattern == action) {
            action_match = true;
            break;
        }
    }

    return action_match;
}

// ========== PolicyManager Implementation ==========

PolicyManager::PolicyManager() = default;

bool PolicyManager::loadRules(const std::string &path) {
    try {
        // Detect file type by extension
        bool is_yaml
            = (path.substr(path.find_last_of(".") + 1) == "yaml" || path.substr(path.find_last_of(".") + 1) == "yml");

        if (is_yaml) {
            // Load from YAML
            YAML::Node config = YAML::LoadFile(path);

            if (!config["rules"]) {
                THEMIS_ERROR("YAML file missing 'rules' field: {}", path);
                return false;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            rules_.clear();

            const auto &rules_node = config["rules"];
            for (const auto &rule_node : rules_node) {
                PolicyRule rule = {};

                if (rule_node["id"]) {
                    rule.id = rule_node["id"].as<std::string>();
                }
                if (rule_node["name"]) {
                    rule.name = rule_node["name"].as<std::string>();
                }
                if (rule_node["description"]) {
                    rule.description = rule_node["description"].as<std::string>();
                }
                if (rule_node["classification_level"]) {
                    rule.classification_level = rule_node["classification_level"].as<std::string>();
                }
                if (rule_node["enabled"]) {
                    rule.enabled = rule_node["enabled"].as<bool>();
                }

                if (rule_node["resources"]) {
                    rule.resources = rule_node["resources"].as<std::vector<std::string>>();
                }
                if (rule_node["actions"]) {
                    rule.actions = rule_node["actions"].as<std::vector<std::string>>();
                }
                if (rule_node["required_roles"]) {
                    rule.required_roles = rule_node["required_roles"].as<std::vector<std::string>>();
                }

                if (rule_node["require_encryption"]) {
                    rule.require_encryption = rule_node["require_encryption"].as<bool>();
                }
                if (rule_node["require_signature"]) {
                    rule.require_signature = rule_node["require_signature"].as<bool>();
                }
                if (rule_node["allow_export"]) {
                    rule.allow_export = rule_node["allow_export"].as<bool>();
                }
                if (rule_node["allow_cache"]) {
                    rule.allow_cache = rule_node["allow_cache"].as<bool>();
                }
                if (rule_node["retention_days"]) {
                    rule.retention_days = rule_node["retention_days"].as<int>();
                }
                if (rule_node["redaction_level"]) {
                    rule.redaction_level = rule_node["redaction_level"].as<std::string>();
                }
                if (rule_node["audit_access"]) {
                    rule.audit_access = rule_node["audit_access"].as<bool>();
                }
                if (rule_node["audit_changes"]) {
                    rule.audit_changes = rule_node["audit_changes"].as<bool>();
                }
                if (rule_node["priority"]) {
                    rule.priority = rule_node["priority"].as<int>();
                }
                if (rule_node["created_by"]) {
                    rule.created_by = rule_node["created_by"].as<std::string>();
                }
                if (rule_node["created_at"]) {
                    rule.created_at = rule_node["created_at"].as<int64_t>();
                }
                if (rule_node["updated_at"]) {
                    rule.updated_at = rule_node["updated_at"].as<int64_t>();
                }

                rules_[rule.id] = rule;
            }

            THEMIS_INFO("Loaded {} policy rules from YAML: {}", rules_.size(), path);
            return true;

        } else {
            // Load from JSON
            std::ifstream file(path);
            if (!file.is_open()) {
                THEMIS_WARN("Policy file not found: {}", path);
                return false;
            }

            nlohmann::json j;
            file >> j;

            return importRules(j);
        }

    } catch (const YAML::Exception &e) {
        THEMIS_ERROR("Failed to load policy rules from YAML {}: {}", path, e.what());
        return false;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to load policy rules from {}: {}", path, e.what());
        return false;
    }
}

bool PolicyManager::saveRules(const std::string &path) {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            THEMIS_ERROR("Failed to open file for writing: {}", path);
            return false;
        }

        nlohmann::json j = exportRules();
        file << j.dump(2);

        THEMIS_INFO("Saved {} policy rules to {}", rules_.size(), path);
        return true;

    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to save policy rules to {}: {}", path, e.what());
        return false;
    }
}

void PolicyManager::addRule(const PolicyRule &rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_[rule.id] = rule;
    THEMIS_DEBUG("Added policy rule: {} ({})", rule.id, rule.name);
}

void PolicyManager::removeRule(const std::string &rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.erase(rule_id);
    THEMIS_DEBUG("Removed policy rule: {}", rule_id);
}

std::optional<PolicyRule> PolicyManager::getRule(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rules_.find(rule_id);
    if (it != rules_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PolicyRule> PolicyManager::listRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyRule> result = {};

    result.reserve(rules_.size());
    for (const auto &[id, rule] : rules_) {
        result.push_back(rule);
    }
    return result;
}

std::vector<PolicyRule> PolicyManager::findApplicableRules(const std::string &resource, const std::string &action,
                                                           const std::vector<std::string> &user_roles) const {
    // Capture a snapshot of the active policy set without holding the write lock,
    // so that a concurrent reloadPolicies() swap does not block this read.
    std::shared_ptr<const PolicySet> snap;
    {
        std::shared_lock<std::shared_mutex> rlock(policy_set_mutex_);
        snap = active_policy_set_;
    }

    // Use the atomic snapshot when available (hot-reload path); fall back to
    // the management rules_ map for readers that call findApplicableRules()
    // before any reloadPolicies() has been executed.
    std::vector<PolicyRule> applicable;
    auto search = [&](const std::unordered_map<std::string, PolicyRule> &rules) {
        for (const auto &[id, rule] : rules) {
            if (!rule.appliesTo(resource, action)) {
                continue;
            }

            // Check if user has required roles
            if (!rule.required_roles.empty()) {
                bool has_role = false;
                for (const auto &required_role : rule.required_roles) {
                    for (const auto &user_role : user_roles) {
                        if (required_role == user_role || required_role == "*") {
                            has_role = true;
                            break;
                        }
                    }
                    if (has_role) {
                        break;
                    }
                }
                if (!has_role) {
                    continue;
                }
            }

            applicable.push_back(rule);
        }
    };

    if (snap) {
        search(snap->rules);
    } else {
        std::lock_guard<std::mutex> lock(mutex_);
        search(rules_);
    }

    // Sort by priority (highest first)
    std::sort(applicable.begin(), applicable.end(),
              [](const PolicyRule &a, const PolicyRule &b) { return a.priority > b.priority; });

    return applicable;
}

PolicyManager::PolicyDecision PolicyManager::evaluatePolicy(const std::string &resource, const std::string &action,
                                                            const std::vector<std::string> &user_roles) const {
    auto applicable_rules = findApplicableRules(resource, action, user_roles);
    return aggregateRules(applicable_rules);
}

PolicyManager::PolicyDecision PolicyManager::aggregateRules(const std::vector<PolicyRule> &rules) const {
    PolicyDecision decision = {};

    if (rules.empty()) {
        // Default permissive policy
        decision.allowed = true;
        return decision;
    }

    // Aggregate effects from all applicable rules
    // More restrictive settings take precedence
    for (const auto &rule : rules) {
        decision.applied_rules.push_back(rule.id);

        // Use highest classification level
        if (decision.classification_level.empty() || rule.classification_level > decision.classification_level) {
            decision.classification_level = rule.classification_level;
        }

        // OR logic for requirements (if any rule requires, it's required)
        decision.require_encryption = decision.require_encryption || rule.require_encryption;
        decision.require_signature  = decision.require_signature || rule.require_signature;
        decision.audit_access       = decision.audit_access || rule.audit_access;
        decision.audit_changes      = decision.audit_changes || rule.audit_changes;

        // AND logic for permissions (if any rule denies, it's denied)
        decision.allow_export = decision.allow_export && rule.allow_export;
        decision.allow_cache  = decision.allow_cache && rule.allow_cache;

        // Use shortest retention period
        decision.retention_days = std::min(decision.retention_days, rule.retention_days);

        // Use strictest redaction level
        if (rule.redaction_level == "strict"
            || (rule.redaction_level == "standard" && decision.redaction_level == "none")) {
            decision.redaction_level = rule.redaction_level;
        }
    }

    return decision;
}

PolicyManager::ValidationResult PolicyManager::validateRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ValidationResult result;
    result.valid = true;

    // Check for duplicate IDs
    std::unordered_map<std::string, int> id_counts = {};

    for (const auto &[id, rule] : rules_) {
        id_counts[id]++;
    }

    for (const auto &[id, count] : id_counts) {
        if (count > 1) {
            result.valid = false;
            result.errors.push_back("Duplicate rule ID: " + id);
        }
    }

    // Check for conflicting rules (same resource/action but different effects)
    for (const auto &[id1, rule1] : rules_) {
        for (const auto &[id2, rule2] : rules_) {
            if (id1 >= id2) {
                continue; // Avoid duplicate checks
            }

            // Simple conflict detection: same resources and actions but different encryption requirements
            if (rule1.resources == rule2.resources && rule1.actions == rule2.actions
                && rule1.require_encryption != rule2.require_encryption) {
                result.warnings.push_back("Potential conflict between rules " + id1 + " and " + id2);
            }
        }
    }

    return result;
}

PolicyManager::PolicyStats PolicyManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    PolicyStats stats;
    stats.total_rules = static_cast<int>(rules_.size());

    for (const auto &[id, rule] : rules_) {
        if (rule.enabled) {
            stats.enabled_rules++;
        } else {
            stats.disabled_rules++;
        }

        stats.rules_by_classification[rule.classification_level]++;
    }

    return stats;
}

nlohmann::json PolicyManager::exportRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    j["version"] = "1.0";
    j["rules"]   = nlohmann::json::array();

    for (const auto &[id, rule] : rules_) {
        j["rules"].push_back(rule.toJson());
    }

    return j;
}

bool PolicyManager::importRules(const nlohmann::json &j) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        if (!j.contains("rules")) {
            THEMIS_ERROR("Invalid policy JSON: missing 'rules' field");
            return false;
        }

        rules_.clear();

        for (const auto &rule_json : j["rules"]) {
            PolicyRule rule = PolicyRule::fromJson(rule_json);
            rules_[rule.id] = rule;
        }

        THEMIS_INFO("Imported {} policy rules", rules_.size());
        return true;

    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to import policy rules: {}", e.what());
        return false;
    }
}

bool PolicyManager::matchPattern(const std::string &pattern, const std::string &value) const {
    if (pattern == "*") {
        return true;
    }
    if (pattern == value) {
        return true;
    }
    // Simple wildcard matching
    if (pattern.back() == '*' && pattern.size() > 1) {
        std::string prefix = pattern.substr(0, static_cast<int>(pattern.size()) - 1);
        return value.find(prefix) == 0;
    }
    return false;
}

// ========== PolicyManager Versioning Methods ==========

std::string PolicyManager::incrementVersion(const std::string &current_version, int level) const {
    // Parse semantic version: major.minor.patch
    std::string version = current_version;
    int major = 0, minor = 0, patch = 0;

    size_t first_dot  = version.find('.');
    size_t second_dot = version.find('.', first_dot + 1);

    if (first_dot != std::string::npos && second_dot != std::string::npos) {
        major = std::stoi(version.substr(0, first_dot));
        minor = std::stoi(version.substr(first_dot + 1, second_dot - first_dot - 1));
        patch = std::stoi(version.substr(second_dot + 1));
    }

    // Increment the specified level
    if (level == 0) { // Major
        major++;
        minor = 0;
        patch = 0;
    } else if (level == 1) { // Minor
        minor++;
        patch = 0;
    } else { // Patch
        patch++;
    }

    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool PolicyManager::updateRule(const std::string &rule_id, const PolicyRule &updated_rule,
                               const std::string &modified_by, const std::string &change_description) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        THEMIS_WARN("Cannot update non-existent rule: {}", rule_id);
        return false;
    }

    // Save current version to history
    std::string timestamp_str = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    const std::string &author
        = !modified_by.empty()
              ? modified_by
              : (!it->second.last_modified_by.empty() ? it->second.last_modified_by : it->second.created_by);
    version_history_.recordVersion(rule_id, it->second, author, "Version saved before update");

    // Update the rule with new version
    PolicyRule new_rule         = updated_rule;
    new_rule.id                 = rule_id;                                 // Ensure ID doesn't change
    new_rule.version            = incrementVersion(it->second.version, 2); // Increment patch version
    new_rule.last_modified_by   = modified_by;
    new_rule.change_description = change_description;
    new_rule.updated_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    rules_[rule_id] = new_rule;

    THEMIS_INFO("Updated policy rule {} to version {}", rule_id, new_rule.version);
    return true;
}

std::vector<PolicyRuleVersion> PolicyManager::getRuleVersions(const std::string &rule_id) const {
    return version_history_.getVersions(rule_id);
}

std::optional<PolicyRuleVersion> PolicyManager::getRuleVersion(const std::string &rule_id,
                                                               const std::string &version) const {
    return version_history_.getVersion(rule_id, version);
}

bool PolicyManager::rollbackToVersion(const std::string &rule_id, const std::string &version,
                                      const std::string &modified_by) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto version_record = version_history_.getVersion(rule_id, version);
    if (!version_record || version_record->rule_snapshot.empty()) {
        THEMIS_WARN("Version {} not found or has no snapshot for rule {}", version, rule_id);
        return false;
    }

    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        THEMIS_WARN("Cannot rollback non-existent rule: {}", rule_id);
        return false;
    }

    // Save current version to history before rollback
    version_history_.recordVersion(rule_id, it->second, modified_by, "Version saved before rollback");

    // Restore rule from snapshot, preserving new incremented version number (level 2 = patch)
    PolicyRule restored         = PolicyRule::fromJson(version_record->rule_snapshot);
    restored.id                 = rule_id; // Ensure ID doesn't change
    restored.version            = incrementVersion(it->second.version, 2);
    restored.last_modified_by   = modified_by;
    restored.change_description = "Rollback to version " + version;
    restored.updated_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    rules_[rule_id] = restored;

    THEMIS_INFO("Rolled back rule {} to version {} (new version: {})", rule_id, version, restored.version);
    return true;
}

bool PolicyManager::rollbackToPreviousVersion(const std::string &rule_id, const std::string &modified_by) {
    std::string latest = version_history_.getLastRecordedVersion(rule_id);
    if (latest.empty()) {
        THEMIS_WARN("No version history found for rule {}", rule_id);
        return false;
    }
    return rollbackToVersion(rule_id, latest, modified_by);
}

std::vector<VersionDiff> PolicyManager::previewRollback(const std::string &rule_id,
                                                        const std::string &target_version) const {
    auto version_record = version_history_.getVersion(rule_id, target_version);
    if (!version_record || version_record->rule_snapshot.empty()) {
        THEMIS_WARN("Version {} not found or has no snapshot for rule {}", target_version, rule_id);
        return {};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        return {};
    }

    // Compare target snapshot against current rule to show what would change
    PolicyRule target_rule = PolicyRule::fromJson(version_record->rule_snapshot);
    VersionDiff diff       = version_history_.compareRules(target_rule, it->second);
    diff.rule_id           = rule_id;
    diff.version1          = target_version;
    diff.version2          = it->second.version;
    return {diff};
}

std::vector<VersionDiff> PolicyManager::compareRuleVersions(const std::string &rule_id, const std::string &version1,
                                                            const std::string &version2) const {
    // Use version_history_ to compare versions
    auto diff = version_history_.compareVersions(rule_id, version1, version2);
    return std::vector<VersionDiff>{diff};
}

std::vector<PolicyRuleVersion> PolicyManager::getAuditTrail(const std::string &rule_id, int64_t /*start_time*/,
                                                            int64_t /*end_time*/) const {
    return version_history_.getVersions(rule_id);
}

std::vector<PolicyRuleVersion> PolicyManager::getAuditTrailByUser(const std::string &user, int64_t /*start_time*/,
                                                                  int64_t /*end_time*/) const {
    // Collect all rule IDs under the rules mutex, then query version history
    std::vector<std::string> rule_ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &[id, rule_value] : rules_) {
            rule_ids.push_back(id);
        }
    }

    std::vector<PolicyRuleVersion> result = {};

    for (const auto &rule_id : rule_ids) {
        auto versions = version_history_.getVersions(rule_id);
        for (const auto &v : versions) {
            if (v.author == user) {
                result.push_back(v);
            }
        }
    }
    return result;
}

// ========== Hot-Reload: double-buffer implementation ==========

bool PolicyManager::reloadPolicies(const std::string &path, std::string *err) {
    // 1. Load the new rule set into a staging PolicyManager.
    //    This keeps the current rules_ untouched until validation succeeds.
    auto staging = std::make_shared<PolicyManager>();
    if (!staging->loadRules(path)) {
        const std::string msg = "reloadPolicies: failed to load rules from " + path;
        THEMIS_ERROR("{}", msg);
        if (err) {
            *err = msg;
        }
        observability::MetricsCollector::getInstance().addCounter("governance_policy_reload_total", 1,
                                                                  {{"result", "failure"}});
        return false;
    }

    // 2. Validate the staged rule set via PolicyValidator.
    PolicyValidator validator(staging);
    auto report = validator.validateRuleset();
    if (report.has_critical_issues) {
        const std::string msg = "reloadPolicies: validation failed – critical issues detected "
                                "(score="
                                + std::to_string(report.validation_score) + ")";
        THEMIS_ERROR("{}", msg);
        if (err) {
            *err = msg;
        }
        observability::MetricsCollector::getInstance().addCounter("governance_policy_reload_total", 1,
                                                                  {{"result", "failure"}});
        return false;
    }

    // 3. Capture the old version hash for the audit entry.
    std::string old_version = {};
    {
        std::shared_lock<std::shared_mutex> rlock(policy_set_mutex_);
        if (active_policy_set_) {
            old_version = active_policy_set_->version_hash;
        }
    }

    // 4. Build the new PolicySet snapshot via the public listRules() API.
    auto new_set = std::make_shared<PolicySet>();
    for (const auto &rule : staging->listRules()) {
        new_set->rules[rule.id] = rule;
    }
    // Derive a deterministic version identifier from sorted rule IDs.
    // Note: std::hash is not cryptographically secure; this hash is used
    // only as a stable version tag for logging and audit entries, not for
    // integrity verification.
    std::vector<std::string> ids = {};

    ids.reserve(new_set->rules.size());
    for (const auto &[id, rule] : new_set->rules) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    std::string concat = {};
    for (const auto &id : ids) {
        concat += id;
        concat += '|';
    }
    new_set->version_hash = std::to_string(std::hash<std::string>{}(concat));
    new_set->loaded_at
        = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
              .count();

    // 5. Atomically promote the new PolicySet (double-buffer swap).
    //    Readers that already hold a shared_ptr snapshot to the old PolicySet
    //    complete normally; the old set stays alive via ref-count until all
    //    holders release it.
    {
        std::unique_lock<std::shared_mutex> wlock(policy_set_mutex_);
        active_policy_set_ = new_set; // implicit conversion to shared_ptr<const PolicySet>
    }
    // Also update rules_ so management operations (addRule, etc.) remain consistent.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rules_ = new_set->rules;
    }

    THEMIS_INFO("PolicyManager::reloadPolicies: {} rules loaded "
                "(old_version={}, new_version={})",
                new_set->rules.size(), old_version, new_set->version_hash);

    observability::MetricsCollector::getInstance().addCounter("governance_policy_reload_total", 1,
                                                              {{"result", "success"}});
    return true;
}

std::string PolicyManager::activePolicyVersion() const {
    std::shared_lock<std::shared_mutex> rlock(policy_set_mutex_);
    if (!active_policy_set_) {
        return {};
    }
    return active_policy_set_->version_hash;
}

// ========== Phase 2-3: Lifecycle State Management ==========

bool PolicyManager::canTransitionRule(const std::string& rule_id, PolicyState target_state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto rule_it = rules_.find(rule_id);
    if (rule_it == rules_.end()) {
        return false;
    }
    
    return rule_it->second.lifecycle.canTransitionTo(target_state);
}

PolicyManager::PolicyResult PolicyManager::activateRuleWithValidation(
    const std::string& rule_id,
    const std::string& user_id) {
    PolicyResult result;
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto rule_it = rules_.find(rule_id);
    if (rule_it == rules_.end()) {
        result.error = PolicyError::kRuleNotFound;
        result.error_message = "Rule not found: " + rule_id;
        return result;
    }
    
    // Validate transition
    if (!rule_it->second.lifecycle.canTransitionTo(PolicyState::ACTIVE)) {
        result.error = PolicyError::kInvalidStateTransition;
        result.error_message = "Cannot transition from " + 
            rule_it->second.lifecycle.getStateDescription() + 
            " to ACTIVE";
        
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kStateTransitionInvalid;
        diag.component = "policy_manager";
        diag.description = "Invalid state transition for rule: " + rule_id;
        diag.remediation_steps = {"Check rule's current state", "Verify transition is allowed"};
        diagnostics_.recordDiagnostic(diag);
        
        return result;
    }
    
    // Check for conflicts with active rules
    auto conflicts = checkConflictsForRule(rule_it->second);
    if (!conflicts.empty()) {
        result.error = PolicyError::kConflictDetected;
        result.error_message = std::to_string(conflicts.size()) + 
            " conflicts detected with rules: " + 
            std::accumulate(conflicts.begin(), conflicts.end(), std::string(),
                [](const std::string& a, const std::string& b) {
                    return a.empty() ? b : a + ", " + b;
                });
        
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kConflictDetected;
        diag.component = "policy_manager";
        diag.description = "Cannot activate rule " + rule_id + 
            ": conflicts with " + std::to_string(conflicts.size()) + " active rules";
        diag.remediation_steps = {
            "Review conflicting rules: " + diag.description,
            "Resolve conflicts before activation",
            "Consider deprecating conflicting rules"
        };
        diagnostics_.recordDiagnostic(diag);
        
        return result;
    }
    
    // Update lifecycle
    rule_it->second.lifecycle.current_state = PolicyState::ACTIVE;
    rule_it->second.lifecycle.activated_at = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    rule_it->second.lifecycle.last_modified_by = user_id;
    
    // Log audit event
    version_history_.recordVersion(rule_id, rule_it->second, user_id, 
                                     "Policy rule activated");
    
    result.error = PolicyError::kSuccess;
    result.rule_version = rule_it->second.version;
    return result;
}

std::string PolicyManager::deprecateRule(
    const std::string& rule_id,
    const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto rule_it = rules_.find(rule_id);
    if (rule_it == rules_.end()) {
        return "";
    }
    
    if (!rule_it->second.lifecycle.canTransitionTo(PolicyState::DEPRECATED)) {
        return "";
    }
    
    rule_it->second.lifecycle.current_state = PolicyState::DEPRECATED;
    rule_it->second.lifecycle.deprecated_at = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    rule_it->second.lifecycle.last_modified_by = user_id;
    
    version_history_.recordVersion(rule_id, rule_it->second, user_id, 
                                     "Policy rule deprecated");
    
    return rule_it->second.version;
}

std::string PolicyManager::retireRule(
    const std::string& rule_id,
    const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto rule_it = rules_.find(rule_id);
    if (rule_it == rules_.end()) {
        return "";
    }
    
    if (!rule_it->second.lifecycle.canTransitionTo(PolicyState::RETIRED)) {
        return "";
    }
    
    rule_it->second.lifecycle.current_state = PolicyState::RETIRED;
    rule_it->second.lifecycle.retired_at = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    rule_it->second.lifecycle.last_modified_by = user_id;
    
    version_history_.recordVersion(rule_id, rule_it->second, user_id, 
                                     "Policy rule retired");
    
    return rule_it->second.version;
}

std::vector<std::string> PolicyManager::checkConflictsForRule(const PolicyRule& rule) const {
    std::vector<std::string> conflicts;
    
    for (const auto& [id, existing_rule] : rules_) {
        if (id == rule.id) {
          continue;
        }
        if (existing_rule.lifecycle.current_state != PolicyState::ACTIVE) {
          continue;
        }
        
        // Check for resource/action overlap
        bool has_resource_overlap = false;
        for (const auto& res : rule.resources) {
            for (const auto& existing_res : existing_rule.resources) {
                if (matchPattern(res, existing_res) || matchPattern(existing_res, res)) {
                    has_resource_overlap = true;
                    break;
                }
            }
            if (has_resource_overlap) {
              break;
            }
        }
        
        if (!has_resource_overlap) {
          continue;
        }
        
        bool has_action_overlap = false;
        for (const auto& act : rule.actions) {
            for (const auto& existing_act : existing_rule.actions) {
                if (act == "*" || existing_act == "*" || act == existing_act) {
                    has_action_overlap = true;
                    break;
                }
            }
            if (has_action_overlap) {
              break;
            }
        }
        
        // Only flag as conflict if effects are contradictory
        if (has_resource_overlap && has_action_overlap &&
            rule.allow_export != existing_rule.allow_export) {
            conflicts.push_back(id);
        }
    }
    
    return conflicts;
}

} // namespace governance
} // namespace themis
