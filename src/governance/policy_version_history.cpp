/**
 * @file policy_version_history.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_version_history.h"

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <fstream>
#include <sstream>

#include "governance/policy_manager.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== PolicyRuleVersion Implementation ==========

nlohmann::json PolicyRuleVersion::toJson() const {
    nlohmann::json j;
    j["version"]            = version;
    j["rule_id"]            = rule_id;
    j["author"]             = author;
    j["timestamp"]          = timestamp;
    j["change_description"] = change_description;
    if (!rule_snapshot.empty()) {
        j["rule_snapshot"] = rule_snapshot;
    }
    return j;
}

PolicyRuleVersion PolicyRuleVersion::fromJson(const nlohmann::json &j) {
    PolicyRuleVersion v = {};
    if (j.contains("version")) {
        v.version = j["version"].get<std::string>();
    }
    if (j.contains("rule_id")) {
        v.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("rule")) {
        v.rule_id = j["rule"].get<std::string>(); // Backwards compatibility
    }
    if (j.contains("author")) {
        v.author = j["author"].get<std::string>();
    }
    if (j.contains("timestamp")) {
        v.timestamp = j["timestamp"].get<std::int64_t>();
    }
    if (j.contains("change_description")) {
        v.change_description = j["change_description"].get<std::string>();
    }
    if (j.contains("rule_snapshot")) {
        v.rule_snapshot = j["rule_snapshot"];
    }
    return v;
}

// ========== AuditLogEntry Implementation ==========

nlohmann::json AuditLogEntry::toJson() const {
    nlohmann::json j;
    j["rule_id"]     = rule_id;
    j["operation"]   = operation;
    j["user"]        = user;
    j["timestamp"]   = timestamp;
    j["old_version"] = old_version;
    j["new_version"] = new_version;
    j["details"]     = details;
    return j;
}

AuditLogEntry AuditLogEntry::fromJson(const nlohmann::json &j) {
    AuditLogEntry e = {};
    if (j.contains("rule_id")) {
        e.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("operation")) {
        e.operation = j["operation"].get<std::string>();
    }
    if (j.contains("user")) {
        e.user = j["user"].get<std::string>();
    }
    if (j.contains("timestamp")) {
        e.timestamp = j["timestamp"].get<std::int64_t>();
    }
    if (j.contains("old_version")) {
        e.old_version = j["old_version"].get<std::string>();
    }
    if (j.contains("new_version")) {
        e.new_version = j["new_version"].get<std::string>();
    }
    if (j.contains("details")) {
        e.details = j["details"];
    }
    return e;
}

// ========== VersionDiff Implementation ==========

nlohmann::json VersionDiff::toJson() const {
    nlohmann::json j;
    j["rule_id"]  = rule_id;
    j["version1"] = version1;
    j["version2"] = version2;
    j["changes"]  = changes;
    j["details"]  = details;
    return j;
}

// ========== PolicyVersionHistory Implementation ==========

PolicyVersionHistory::PolicyVersionHistory() {}

std::string PolicyVersionHistory::recordVersion(const std::string &rule_id, const PolicyRule &rule,
                                                const std::string &author, const std::string &change_description) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Get latest version and increment
    std::string latest      = getLatestVersion(rule_id);
    std::string new_version = incrementVersion(latest);

    // Create version entry
    PolicyRuleVersion version;
    version.version            = new_version;
    version.rule_id            = rule_id;
    version.author             = author;
    version.timestamp          = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    version.change_description = change_description;
    version.rule_snapshot      = rule.toJson(); // Capture full rule state

    // Store version
    versions_[rule_id].push_back(version);

    THEMIS_INFO("Recorded version {} for rule {}", new_version, rule_id);

    return new_version;
}

std::vector<PolicyRuleVersion> PolicyVersionHistory::getVersions(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = versions_.find(rule_id);
    if (it == versions_.end()) {
        return {};
    }

    // Return in reverse order (newest first)
    std::vector<PolicyRuleVersion> result = it->second;
    std::reverse(result.begin(), result.end());
    return result;
}

std::optional<PolicyRuleVersion> PolicyVersionHistory::getVersion(const std::string &rule_id,
                                                                  const std::string &version) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = versions_.find(rule_id);
    if (it == versions_.end()) {
        return std::nullopt;
    }

    for (const auto &v : it->second) {
        if (v.version == version) {
            return v;
        }
    }

    return std::nullopt;
}

std::string PolicyVersionHistory::getLatestVersion(const std::string &rule_id) const {
    // Note: This method assumes the caller already holds the mutex lock
    // It is called from recordVersion which holds the lock

    auto it = versions_.find(rule_id);
    if (it == versions_.end() || it->second.empty()) {
        return "0.0.0";
    }

    return it->second.back().version;
}

std::optional<std::string> PolicyVersionHistory::getPreviousVersion(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = versions_.find(rule_id);
    if (it == versions_.end() || it-> static_cast<int>(second.size()) < 2) {
        return std::nullopt;
    }

    return static_cast<bool>(it->second[it- < static_cast<int>(second.size())) - 2].version;
}

std::string PolicyVersionHistory::getLastRecordedVersion(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = versions_.find(rule_id);
    if (it == versions_.end() || it->second.empty()) {
        return "";
    }

    return it->second.back().version;
}

VersionDiff PolicyVersionHistory::compareRules(const PolicyRule &rule1, const PolicyRule &rule2) const {
    VersionDiff diff;
    diff.rule_id  = rule1.id;
    diff.version1 = rule1.version;
    diff.version2 = rule2.version;
    diff.changes  = identifyChanges(rule1, rule2);
    return diff;
}

VersionDiff PolicyVersionHistory::compareVersions(const std::string &rule_id, const std::string &version1,
                                                  const std::string &version2) const {
    VersionDiff diff;
    diff.rule_id  = rule_id;
    diff.version1 = version1;
    diff.version2 = version2;

    // Get versions without holding the lock to avoid deadlock
    auto v1 = getVersion(rule_id, version1);
    auto v2 = getVersion(rule_id, version2);

    if (!v1.has_value() || !v2.has_value()) {
        diff.changes.push_back("One or both versions not found");
        return diff;
    }

    // Use stored rule snapshots for field-level diff when available
    if (!v1->rule_snapshot.empty() && !v2->rule_snapshot.empty()) {
        PolicyRule rule1 = PolicyRule::fromJson(v1->rule_snapshot);
        PolicyRule rule2 = PolicyRule::fromJson(v2->rule_snapshot);
        diff.changes     = identifyChanges(rule1, rule2);
    } else {
        // Fallback: note that versions differ without field-level detail
        diff.changes.push_back(fmt::format("Version {} -> {}", version1, version2));
    }

    // Create detailed diff with version metadata
    nlohmann::json v1_data;
    v1_data["version"]            = v1->version;
    v1_data["rule_id"]            = v1->rule_id;
    v1_data["author"]             = v1->author;
    v1_data["timestamp"]          = v1->timestamp;
    v1_data["change_description"] = v1->change_description;

    nlohmann::json v2_data;
    v2_data["version"]            = v2->version;
    v2_data["rule_id"]            = v2->rule_id;
    v2_data["author"]             = v2->author;
    v2_data["timestamp"]          = v2->timestamp;
    v2_data["change_description"] = v2->change_description;

    diff.details["version1_data"]      = v1_data;
    diff.details["version2_data"]      = v2_data;
    diff.details["version1_author"]    = v1->author;
    diff.details["version2_author"]    = v2->author;
    diff.details["version1_timestamp"] = v1->timestamp;
    diff.details["version2_timestamp"] = v2->timestamp;

    return diff;
}

void PolicyVersionHistory::recordAudit(const AuditLogEntry &entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_log_.push_back(entry);
    THEMIS_INFO("Audit: {} operation on rule {} by user {}", entry.operation, entry.rule_id, entry.user);
}

std::vector<AuditLogEntry> PolicyVersionHistory::queryAudit(const std::optional<std::string> &rule_id,
                                                            const std::optional<std::string> &user,
                                                            const std::optional<std::int64_t> &start_time,
                                                            const std::optional<std::int64_t> &end_time) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<AuditLogEntry> result;

    for (const auto &entry : audit_log_) {
        // Apply filters
        if (rule_id.has_value() && entry.rule_id != *rule_id) {
            continue;
        }
        if (user.has_value() && entry.user != *user) {
            continue;
        }
        if (start_time.has_value() && entry.timestamp < *start_time) {
            continue;
        }
        if (end_time.has_value() && entry.timestamp > *end_time) {
            continue;
        }

        result.push_back(entry);
    }

    return result;
}

void PolicyVersionHistory::deleteVersionHistory(const std::string &rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    versions_.erase(rule_id);
    THEMIS_INFO("Deleted version history for rule {}", rule_id);
}

nlohmann::json PolicyVersionHistory::exportHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;

    // Export versions
    nlohmann::json versions_json = nlohmann::json::object();
    for (const auto &[rule_id, versions] : versions_) {
        nlohmann::json version_array = nlohmann::json::array();
        for (const auto &v : versions) {
            version_array.push_back(v.toJson());
        }
        versions_json[rule_id] = version_array;
    }
    j["versions"] = versions_json;

    // Export audit log
    nlohmann::json audit_array = nlohmann::json::array();
    for (const auto &entry : audit_log_) {
        audit_array.push_back(entry.toJson());
    }
    j["audit_log"] = audit_array;

    return j;
}

bool PolicyVersionHistory::importHistory(const nlohmann::json &j) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Import versions
        if (j.contains("versions") && j["versions"].is_object()) {
            for (auto &[rule_id, version_array] : j["versions"].items()) {
                std::vector<PolicyRuleVersion> versions = {};

                for (const auto &v_json : version_array) {
                    versions.push_back(PolicyRuleVersion::fromJson(v_json));
                }
                versions_[rule_id] = versions;
            }
        }

        // Import audit log
        if (j.contains("audit_log") && j["audit_log"].is_array()) {
            for (const auto &entry_json : j["audit_log"]) {
                audit_log_.push_back(AuditLogEntry::fromJson(entry_json));
            }
        }

        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Error importing version history: {}", e.what());
        return false;
    }
}

bool PolicyVersionHistory::saveToFile(const std::string &path) const {
    try {
        nlohmann::json j = exportHistory();
        std::ofstream file(path);
        if (!file.is_open()) {
            THEMIS_ERROR("Failed to open file for writing: {}", path);
            return false;
        }
        file << j.dump(2);
        THEMIS_INFO("Saved version history to {}", path);
        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Error saving version history to {}: {}", path, e.what());
        return false;
    }
}

bool PolicyVersionHistory::loadFromFile(const std::string &path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            THEMIS_ERROR("Failed to open file for reading: {}", path);
            return false;
        }
        nlohmann::json j;
        file >> j;
        bool result = importHistory(j);
        if (result) {
            THEMIS_INFO("Loaded version history from {}", path);
        }
        return result;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Error loading version history from {}: {}", path, e.what());
        return false;
    }
}

std::string PolicyVersionHistory::incrementVersion(const std::string &current_version) const {
    // Parse version string (major.minor.patch)
    int major = 0, minor = 0, patch = 0;
    std::sscanf(current_version.c_str(), "%d.%d.%d", &major, &minor, &patch);

    // For new rules (0.0.0), start with 1.0.0
    if (major == 0 && minor == 0 && patch == 0) {
        return "1.0.0";
    }

    // Otherwise increment patch version
    // Note: To increment major or minor versions, use explicit versioning
    // This is the default auto-increment behavior for regular updates
    patch++;

    // Format new version
    std::ostringstream oss = {};
    oss << major << "." << minor << "." << patch;
    return oss.str();
}

std::vector<std::string> PolicyVersionHistory::identifyChanges(const PolicyRule &rule1, const PolicyRule &rule2) const {
    std::vector<std::string> changes;

    if (rule1.name != rule2.name) {
        changes.push_back("name");
    }
    if (rule1.description != rule2.description) {
        changes.push_back("description");
    }
    if (rule1.classification_level != rule2.classification_level) {
        changes.push_back("classification_level");
    }
    if (rule1.enabled != rule2.enabled) {
        changes.push_back("enabled");
    }
    if (rule1.resources != rule2.resources) {
        changes.push_back("resources");
    }
    if (rule1.actions != rule2.actions) {
        changes.push_back("actions");
    }
    if (rule1.required_roles != rule2.required_roles) {
        changes.push_back("required_roles");
    }
    if (rule1.require_encryption != rule2.require_encryption) {
        changes.push_back("require_encryption");
    }
    if (rule1.require_signature != rule2.require_signature) {
        changes.push_back("require_signature");
    }
    if (rule1.allow_export != rule2.allow_export) {
        changes.push_back("allow_export");
    }
    if (rule1.allow_cache != rule2.allow_cache) {
        changes.push_back("allow_cache");
    }
    if (rule1.retention_days != rule2.retention_days) {
        changes.push_back("retention_days");
    }
    if (rule1.redaction_level != rule2.redaction_level) {
        changes.push_back("redaction_level");
    }
    if (rule1.audit_access != rule2.audit_access) {
        changes.push_back("audit_access");
    }
    if (rule1.audit_changes != rule2.audit_changes) {
        changes.push_back("audit_changes");
    }
    if (rule1.priority != rule2.priority) {
        changes.push_back("priority");
    }

    return changes;
}

} // namespace governance
} // namespace themis
