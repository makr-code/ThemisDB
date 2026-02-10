#include "governance/policy_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <chrono>
#include <yaml-cpp/yaml.h>

namespace themis {
namespace governance {

// ========== PolicyRule Implementation ==========

nlohmann::json PolicyRule::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["classification_level"] = classification_level;
    j["enabled"] = enabled;
    j["resources"] = resources;
    j["actions"] = actions;
    j["required_roles"] = required_roles;
    j["require_encryption"] = require_encryption;
    j["require_signature"] = require_signature;
    j["allow_export"] = allow_export;
    j["allow_cache"] = allow_cache;
    j["retention_days"] = retention_days;
    j["redaction_level"] = redaction_level;
    j["audit_access"] = audit_access;
    j["audit_changes"] = audit_changes;
    j["priority"] = priority;
    j["created_by"] = created_by;
    j["created_at"] = created_at;
    j["updated_at"] = updated_at;
    j["version"] = version;
    j["last_modified_by"] = last_modified_by;
    j["change_description"] = change_description;
    return j;
}

PolicyRule PolicyRule::fromJson(const nlohmann::json& j) {
    PolicyRule rule;
    if (j.contains("id")) rule.id = j["id"].get<std::string>();
    if (j.contains("name")) rule.name = j["name"].get<std::string>();
    if (j.contains("description")) rule.description = j["description"].get<std::string>();
    if (j.contains("classification_level")) rule.classification_level = j["classification_level"].get<std::string>();
    if (j.contains("enabled")) rule.enabled = j["enabled"].get<bool>();
    if (j.contains("resources")) rule.resources = j["resources"].get<std::vector<std::string>>();
    if (j.contains("actions")) rule.actions = j["actions"].get<std::vector<std::string>>();
    if (j.contains("required_roles")) rule.required_roles = j["required_roles"].get<std::vector<std::string>>();
    if (j.contains("require_encryption")) rule.require_encryption = j["require_encryption"].get<bool>();
    if (j.contains("require_signature")) rule.require_signature = j["require_signature"].get<bool>();
    if (j.contains("allow_export")) rule.allow_export = j["allow_export"].get<bool>();
    if (j.contains("allow_cache")) rule.allow_cache = j["allow_cache"].get<bool>();
    if (j.contains("retention_days")) rule.retention_days = j["retention_days"].get<int>();
    if (j.contains("redaction_level")) rule.redaction_level = j["redaction_level"].get<std::string>();
    if (j.contains("audit_access")) rule.audit_access = j["audit_access"].get<bool>();
    if (j.contains("audit_changes")) rule.audit_changes = j["audit_changes"].get<bool>();
    if (j.contains("priority")) rule.priority = j["priority"].get<int>();
    if (j.contains("created_by")) rule.created_by = j["created_by"].get<std::string>();
    if (j.contains("created_at")) rule.created_at = j["created_at"].get<int64_t>();
    if (j.contains("updated_at")) rule.updated_at = j["updated_at"].get<int64_t>();
    if (j.contains("version")) rule.version = j["version"].get<std::string>();
    if (j.contains("last_modified_by")) rule.last_modified_by = j["last_modified_by"].get<std::string>();
    if (j.contains("change_description")) rule.change_description = j["change_description"].get<std::string>();
    return rule;
}

bool PolicyRule::appliesTo(const std::string& resource, const std::string& action) const {
    if (!enabled) {
        return false;
    }
    
    // Check resource match
    bool resource_match = resources.empty(); // Empty means all resources
    for (const auto& pattern : resources) {
        if (pattern == "*" || pattern == resource) {
            resource_match = true;
            break;
        }
        // Simple wildcard matching: "data/*" matches "data/anything"
        if (pattern.back() == '*' && pattern.size() > 1) {
            std::string prefix = pattern.substr(0, pattern.size() - 1);
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
    for (const auto& pattern : actions) {
        if (pattern == "*" || pattern == action) {
            action_match = true;
            break;
        }
    }
    
    return action_match;
}

// ========== PolicyManager Implementation ==========

PolicyManager::PolicyManager() = default;

bool PolicyManager::loadRules(const std::string& path) {
    try {
        // Detect file type by extension
        bool is_yaml = (path.substr(path.find_last_of(".") + 1) == "yaml" || 
                        path.substr(path.find_last_of(".") + 1) == "yml");
        
        if (is_yaml) {
            // Load from YAML
            YAML::Node config = YAML::LoadFile(path);
            
            if (!config["rules"]) {
                THEMIS_ERROR("YAML file missing 'rules' field: {}", path);
                return false;
            }
            
            std::lock_guard<std::mutex> lock(mutex_);
            rules_.clear();
            
            const auto& rules_node = config["rules"];
            for (const auto& rule_node : rules_node) {
                PolicyRule rule;
                
                if (rule_node["id"]) rule.id = rule_node["id"].as<std::string>();
                if (rule_node["name"]) rule.name = rule_node["name"].as<std::string>();
                if (rule_node["description"]) rule.description = rule_node["description"].as<std::string>();
                if (rule_node["classification_level"]) rule.classification_level = rule_node["classification_level"].as<std::string>();
                if (rule_node["enabled"]) rule.enabled = rule_node["enabled"].as<bool>();
                
                if (rule_node["resources"]) {
                    rule.resources = rule_node["resources"].as<std::vector<std::string>>();
                }
                if (rule_node["actions"]) {
                    rule.actions = rule_node["actions"].as<std::vector<std::string>>();
                }
                if (rule_node["required_roles"]) {
                    rule.required_roles = rule_node["required_roles"].as<std::vector<std::string>>();
                }
                
                if (rule_node["require_encryption"]) rule.require_encryption = rule_node["require_encryption"].as<bool>();
                if (rule_node["require_signature"]) rule.require_signature = rule_node["require_signature"].as<bool>();
                if (rule_node["allow_export"]) rule.allow_export = rule_node["allow_export"].as<bool>();
                if (rule_node["allow_cache"]) rule.allow_cache = rule_node["allow_cache"].as<bool>();
                if (rule_node["retention_days"]) rule.retention_days = rule_node["retention_days"].as<int>();
                if (rule_node["redaction_level"]) rule.redaction_level = rule_node["redaction_level"].as<std::string>();
                if (rule_node["audit_access"]) rule.audit_access = rule_node["audit_access"].as<bool>();
                if (rule_node["audit_changes"]) rule.audit_changes = rule_node["audit_changes"].as<bool>();
                if (rule_node["priority"]) rule.priority = rule_node["priority"].as<int>();
                if (rule_node["created_by"]) rule.created_by = rule_node["created_by"].as<std::string>();
                if (rule_node["created_at"]) rule.created_at = rule_node["created_at"].as<int64_t>();
                if (rule_node["updated_at"]) rule.updated_at = rule_node["updated_at"].as<int64_t>();
                
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
        
    } catch (const YAML::Exception& e) {
        THEMIS_ERROR("Failed to load policy rules from YAML {}: {}", path, e.what());
        return false;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load policy rules from {}: {}", path, e.what());
        return false;
    }
}

bool PolicyManager::saveRules(const std::string& path) {
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
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save policy rules to {}: {}", path, e.what());
        return false;
    }
}

void PolicyManager::addRule(const PolicyRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_[rule.id] = rule;
    THEMIS_DEBUG("Added policy rule: {} ({})", rule.id, rule.name);
}

void PolicyManager::removeRule(const std::string& rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.erase(rule_id);
    THEMIS_DEBUG("Removed policy rule: {}", rule_id);
}

std::optional<PolicyRule> PolicyManager::getRule(const std::string& rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rules_.find(rule_id);
    if (it != rules_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PolicyRule> PolicyManager::listRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyRule> result;
    result.reserve(rules_.size());
    for (const auto& [id, rule] : rules_) {
        result.push_back(rule);
    }
    return result;
}

std::vector<PolicyRule> PolicyManager::findApplicableRules(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyRule> applicable;
    
    for (const auto& [id, rule] : rules_) {
        if (!rule.appliesTo(resource, action)) {
            continue;
        }
        
        // Check if user has required roles
        if (!rule.required_roles.empty()) {
            bool has_role = false;
            for (const auto& required_role : rule.required_roles) {
                for (const auto& user_role : user_roles) {
                    if (required_role == user_role || required_role == "*") {
                        has_role = true;
                        break;
                    }
                }
                if (has_role) break;
            }
            if (!has_role) {
                continue;
            }
        }
        
        applicable.push_back(rule);
    }
    
    // Sort by priority (highest first)
    std::sort(applicable.begin(), applicable.end(),
              [](const PolicyRule& a, const PolicyRule& b) {
                  return a.priority > b.priority;
              });
    
    return applicable;
}

PolicyManager::PolicyDecision PolicyManager::evaluatePolicy(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    auto applicable_rules = findApplicableRules(resource, action, user_roles);
    return aggregateRules(applicable_rules);
}

PolicyManager::PolicyDecision PolicyManager::aggregateRules(
    const std::vector<PolicyRule>& rules
) const {
    PolicyDecision decision;
    
    if (rules.empty()) {
        // Default permissive policy
        decision.allowed = true;
        return decision;
    }
    
    // Aggregate effects from all applicable rules
    // More restrictive settings take precedence
    for (const auto& rule : rules) {
        decision.applied_rules.push_back(rule.id);
        
        // Use highest classification level
        if (decision.classification_level.empty() || 
            rule.classification_level > decision.classification_level) {
            decision.classification_level = rule.classification_level;
        }
        
        // OR logic for requirements (if any rule requires, it's required)
        decision.require_encryption = decision.require_encryption || rule.require_encryption;
        decision.require_signature = decision.require_signature || rule.require_signature;
        decision.audit_access = decision.audit_access || rule.audit_access;
        decision.audit_changes = decision.audit_changes || rule.audit_changes;
        
        // AND logic for permissions (if any rule denies, it's denied)
        decision.allow_export = decision.allow_export && rule.allow_export;
        decision.allow_cache = decision.allow_cache && rule.allow_cache;
        
        // Use shortest retention period
        decision.retention_days = std::min(decision.retention_days, rule.retention_days);
        
        // Use strictest redaction level
        if (rule.redaction_level == "strict" || 
            (rule.redaction_level == "standard" && decision.redaction_level == "none")) {
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
    std::unordered_map<std::string, int> id_counts;
    for (const auto& [id, rule] : rules_) {
        id_counts[id]++;
    }
    
    for (const auto& [id, count] : id_counts) {
        if (count > 1) {
            result.valid = false;
            result.errors.push_back("Duplicate rule ID: " + id);
        }
    }
    
    // Check for conflicting rules (same resource/action but different effects)
    for (const auto& [id1, rule1] : rules_) {
        for (const auto& [id2, rule2] : rules_) {
            if (id1 >= id2) continue; // Avoid duplicate checks
            
            // Simple conflict detection: same resources and actions but different encryption requirements
            if (rule1.resources == rule2.resources && 
                rule1.actions == rule2.actions &&
                rule1.require_encryption != rule2.require_encryption) {
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
    
    for (const auto& [id, rule] : rules_) {
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
    j["rules"] = nlohmann::json::array();
    
    for (const auto& [id, rule] : rules_) {
        j["rules"].push_back(rule.toJson());
    }
    
    return j;
}

bool PolicyManager::importRules(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        if (!j.contains("rules")) {
            THEMIS_ERROR("Invalid policy JSON: missing 'rules' field");
            return false;
        }
        
        rules_.clear();
        
        for (const auto& rule_json : j["rules"]) {
            PolicyRule rule = PolicyRule::fromJson(rule_json);
            rules_[rule.id] = rule;
        }
        
        THEMIS_INFO("Imported {} policy rules", rules_.size());
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to import policy rules: {}", e.what());
        return false;
    }
}

bool PolicyManager::matchPattern(const std::string& pattern, const std::string& value) const {
    if (pattern == "*") {
        return true;
    }
    if (pattern == value) {
        return true;
    }
    // Simple wildcard matching
    if (pattern.back() == '*' && pattern.size() > 1) {
        std::string prefix = pattern.substr(0, pattern.size() - 1);
        return value.find(prefix) == 0;
    }
    return false;
}

// ========== PolicyVersionHistory Implementation ==========

nlohmann::json PolicyVersionHistory::VersionRecord::toJson() const {
    nlohmann::json j;
    j["version"] = version;
    j["rule"] = rule.toJson();
    j["timestamp"] = timestamp;
    j["modified_by"] = modified_by;
    j["change_description"] = change_description;
    return j;
}

PolicyVersionHistory::VersionRecord PolicyVersionHistory::VersionRecord::fromJson(const nlohmann::json& j) {
    VersionRecord record;
    if (j.contains("version")) record.version = j["version"].get<std::string>();
    if (j.contains("rule")) record.rule = PolicyRule::fromJson(j["rule"]);
    if (j.contains("timestamp")) record.timestamp = j["timestamp"].get<int64_t>();
    if (j.contains("modified_by")) record.modified_by = j["modified_by"].get<std::string>();
    if (j.contains("change_description")) record.change_description = j["change_description"].get<std::string>();
    return record;
}

void PolicyVersionHistory::addVersion(const std::string& rule_id, const VersionRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    history_[rule_id].push_back(record);
    
    // Sort by timestamp (oldest first)
    std::sort(history_[rule_id].begin(), history_[rule_id].end(),
              [](const VersionRecord& a, const VersionRecord& b) {
                  return a.timestamp < b.timestamp;
              });
}

std::vector<PolicyVersionHistory::VersionRecord> PolicyVersionHistory::getVersions(const std::string& rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = history_.find(rule_id);
    if (it != history_.end()) {
        return it->second;
    }
    return {};
}

std::optional<PolicyVersionHistory::VersionRecord> PolicyVersionHistory::getVersion(
    const std::string& rule_id, const std::string& version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = history_.find(rule_id);
    if (it != history_.end()) {
        for (const auto& record : it->second) {
            if (record.version == version) {
                return record;
            }
        }
    }
    return std::nullopt;
}

std::optional<PolicyVersionHistory::VersionRecord> PolicyVersionHistory::getLatestVersion(const std::string& rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = history_.find(rule_id);
    if (it != history_.end() && !it->second.empty()) {
        return it->second.back(); // Last element is the latest
    }
    return std::nullopt;
}

std::vector<PolicyVersionHistory::VersionDiff> PolicyVersionHistory::compareVersions(
    const std::string& rule_id, const std::string& version1, const std::string& version2) const {
    std::vector<VersionDiff> diffs;
    
    auto v1 = getVersion(rule_id, version1);
    auto v2 = getVersion(rule_id, version2);
    
    if (!v1 || !v2) {
        return diffs;
    }
    
    const auto& r1 = v1->rule;
    const auto& r2 = v2->rule;
    
    // Compare fields
    if (r1.name != r2.name) {
        diffs.push_back({"name", r1.name, r2.name});
    }
    if (r1.description != r2.description) {
        diffs.push_back({"description", r1.description, r2.description});
    }
    if (r1.classification_level != r2.classification_level) {
        diffs.push_back({"classification_level", r1.classification_level, r2.classification_level});
    }
    if (r1.enabled != r2.enabled) {
        diffs.push_back({"enabled", r1.enabled ? "true" : "false", r2.enabled ? "true" : "false"});
    }
    if (r1.require_encryption != r2.require_encryption) {
        diffs.push_back({"require_encryption", r1.require_encryption ? "true" : "false", r2.require_encryption ? "true" : "false"});
    }
    if (r1.require_signature != r2.require_signature) {
        diffs.push_back({"require_signature", r1.require_signature ? "true" : "false", r2.require_signature ? "true" : "false"});
    }
    if (r1.allow_export != r2.allow_export) {
        diffs.push_back({"allow_export", r1.allow_export ? "true" : "false", r2.allow_export ? "true" : "false"});
    }
    if (r1.allow_cache != r2.allow_cache) {
        diffs.push_back({"allow_cache", r1.allow_cache ? "true" : "false", r2.allow_cache ? "true" : "false"});
    }
    if (r1.retention_days != r2.retention_days) {
        diffs.push_back({"retention_days", std::to_string(r1.retention_days), std::to_string(r2.retention_days)});
    }
    if (r1.priority != r2.priority) {
        diffs.push_back({"priority", std::to_string(r1.priority), std::to_string(r2.priority)});
    }
    
    return diffs;
}

std::vector<PolicyVersionHistory::VersionRecord> PolicyVersionHistory::getAuditTrail(
    const std::string& rule_id, int64_t start_time, int64_t end_time) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<VersionRecord> trail;
    
    auto it = history_.find(rule_id);
    if (it != history_.end()) {
        for (const auto& record : it->second) {
            if (record.timestamp >= start_time && record.timestamp <= end_time) {
                trail.push_back(record);
            }
        }
    }
    
    return trail;
}

nlohmann::json PolicyVersionHistory::exportHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    
    for (const auto& [rule_id, versions] : history_) {
        nlohmann::json versions_array = nlohmann::json::array();
        for (const auto& version : versions) {
            versions_array.push_back(version.toJson());
        }
        j[rule_id] = versions_array;
    }
    
    return j;
}

bool PolicyVersionHistory::importHistory(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        history_.clear();
        
        for (auto& [rule_id, versions_json] : j.items()) {
            std::vector<VersionRecord> versions;
            for (const auto& version_json : versions_json) {
                versions.push_back(VersionRecord::fromJson(version_json));
            }
            history_[rule_id] = versions;
        }
        
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to import version history: {}", e.what());
        return false;
    }
}

void PolicyVersionHistory::clearHistory(const std::string& rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.erase(rule_id);
}

// ========== PolicyManager Versioning Methods ==========

std::string PolicyManager::incrementVersion(const std::string& current_version, int level) const {
    // Parse semantic version: major.minor.patch
    std::string version = current_version;
    int major = 0, minor = 0, patch = 0;
    
    size_t first_dot = version.find('.');
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

bool PolicyManager::updateRule(const std::string& rule_id, const PolicyRule& updated_rule,
                               const std::string& modified_by, const std::string& change_description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        THEMIS_WARN("Cannot update non-existent rule: {}", rule_id);
        return false;
    }
    
    // Save current version to history
    PolicyVersionHistory::VersionRecord record;
    record.version = it->second.version;
    record.rule = it->second;
    record.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.modified_by = it->second.last_modified_by.empty() ? it->second.created_by : it->second.last_modified_by;
    record.change_description = "Version saved before update";
    
    version_history_.addVersion(rule_id, record);
    
    // Update the rule with new version
    PolicyRule new_rule = updated_rule;
    new_rule.id = rule_id; // Ensure ID doesn't change
    new_rule.version = incrementVersion(it->second.version, 2); // Increment patch version
    new_rule.last_modified_by = modified_by;
    new_rule.change_description = change_description;
    new_rule.updated_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    rules_[rule_id] = new_rule;
    
    THEMIS_INFO("Updated policy rule {} to version {}", rule_id, new_rule.version);
    return true;
}

std::vector<PolicyVersionHistory::VersionRecord> PolicyManager::getRuleVersions(const std::string& rule_id) const {
    return version_history_.getVersions(rule_id);
}

std::optional<PolicyVersionHistory::VersionRecord> PolicyManager::getRuleVersion(
    const std::string& rule_id, const std::string& version) const {
    return version_history_.getVersion(rule_id, version);
}

bool PolicyManager::rollbackToVersion(const std::string& rule_id, const std::string& version,
                                      const std::string& modified_by) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto version_record = version_history_.getVersion(rule_id, version);
    if (!version_record) {
        THEMIS_WARN("Version {} not found for rule {}", version, rule_id);
        return false;
    }
    
    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        THEMIS_WARN("Cannot rollback non-existent rule: {}", rule_id);
        return false;
    }
    
    // Save current version to history before rollback
    PolicyVersionHistory::VersionRecord current_record;
    current_record.version = it->second.version;
    current_record.rule = it->second;
    current_record.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    current_record.modified_by = modified_by;
    current_record.change_description = "Before rollback to version " + version;
    
    version_history_.addVersion(rule_id, current_record);
    
    // Restore the old version
    PolicyRule restored_rule = version_record->rule;
    restored_rule.version = incrementVersion(it->second.version, 2); // New version for rollback
    restored_rule.last_modified_by = modified_by;
    restored_rule.change_description = "Rolled back to version " + version;
    restored_rule.updated_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    rules_[rule_id] = restored_rule;
    
    THEMIS_INFO("Rolled back rule {} to version {} (new version: {})", rule_id, version, restored_rule.version);
    return true;
}

bool PolicyManager::rollbackToPreviousVersion(const std::string& rule_id, const std::string& modified_by) {
    auto versions = version_history_.getVersions(rule_id);
    if (versions.size() < 2) {
        THEMIS_WARN("Not enough versions to rollback rule {}", rule_id);
        return false;
    }
    
    // Get the second-to-last version (before the current one)
    const auto& previous_version = versions[versions.size() - 2];
    return rollbackToVersion(rule_id, previous_version.version, modified_by);
}

std::vector<PolicyVersionHistory::VersionDiff> PolicyManager::previewRollback(
    const std::string& rule_id, const std::string& target_version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        return {};
    }
    
    auto target_record = version_history_.getVersion(rule_id, target_version);
    if (!target_record) {
        return {};
    }
    
    // Compare current rule with target version
    std::vector<PolicyVersionHistory::VersionDiff> diffs;
    const auto& current = it->second;
    const auto& target = target_record->rule;
    
    // Same comparison logic as compareVersions
    if (current.name != target.name) {
        diffs.push_back({"name", current.name, target.name});
    }
    if (current.description != target.description) {
        diffs.push_back({"description", current.description, target.description});
    }
    if (current.enabled != target.enabled) {
        diffs.push_back({"enabled", current.enabled ? "true" : "false", target.enabled ? "true" : "false"});
    }
    if (current.require_encryption != target.require_encryption) {
        diffs.push_back({"require_encryption", current.require_encryption ? "true" : "false", target.require_encryption ? "true" : "false"});
    }
    
    return diffs;
}

std::vector<PolicyVersionHistory::VersionDiff> PolicyManager::compareRuleVersions(
    const std::string& rule_id, const std::string& version1, const std::string& version2) const {
    return version_history_.compareVersions(rule_id, version1, version2);
}

std::vector<PolicyVersionHistory::VersionRecord> PolicyManager::getAuditTrail(
    const std::string& rule_id, int64_t start_time, int64_t end_time) const {
    return version_history_.getAuditTrail(rule_id, start_time, end_time);
}

std::vector<PolicyVersionHistory::VersionRecord> PolicyManager::getAuditTrailByUser(
    const std::string& user, int64_t start_time, int64_t end_time) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyVersionHistory::VersionRecord> trail;
    
    // Get all audit records
    auto all_history = version_history_.exportHistory();
    
    for (auto& [rule_id, versions_json] : all_history.items()) {
        for (const auto& version_json : versions_json) {
            auto record = PolicyVersionHistory::VersionRecord::fromJson(version_json);
            if (record.modified_by == user &&
                record.timestamp >= start_time &&
                record.timestamp <= end_time) {
                trail.push_back(record);
            }
        }
    }
    
    // Sort by timestamp
    std::sort(trail.begin(), trail.end(),
              [](const PolicyVersionHistory::VersionRecord& a, const PolicyVersionHistory::VersionRecord& b) {
                  return a.timestamp < b.timestamp;
              });
    
    return trail;
}

} // namespace governance
} // namespace themis
