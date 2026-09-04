/**
 * @file policy_change_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @date 2026-08-18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Advanced rollback with dependency tracking and safety verification
 */

#include "governance/policy_change_manager.h"
#include "governance/policy_manager.h"
#include "governance/policy_version_history.h"

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <uuid.h>

namespace themis {
namespace governance {

// ========== PolicyDependency Implementation ==========

nlohmann::json PolicyDependency::toJson() const {
    nlohmann::json j;
    j["dependent_rule_id"]   = dependent_rule_id;
    j["dependency_rule_id"]  = dependency_rule_id;
    j["dependency_type"]     = dependency_type;
    j["reason"]              = reason;
    return j;
}

PolicyDependency PolicyDependency::fromJson(const nlohmann::json& j) {
    PolicyDependency d;
    if (j.contains("dependent_rule_id")) {
        d.dependent_rule_id = j["dependent_rule_id"].get<std::string>();
    }
    if (j.contains("dependency_rule_id")) {
        d.dependency_rule_id = j["dependency_rule_id"].get<std::string>();
    }
    if (j.contains("dependency_type")) {
        d.dependency_type = j["dependency_type"].get<std::string>();
    }
    if (j.contains("reason")) {
        d.reason = j["reason"].get<std::string>();
    }
    return d;
}

// ========== RollbackSafetyReport Implementation ==========

nlohmann::json RollbackSafetyReport::toJson() const {
    nlohmann::json j;
    j["rule_id"]                   = rule_id;
    j["target_version"]            = target_version;
    j["safety_level"]              = static_cast<int>(safety_level);
    j["conflicts"]                 = conflicts;
    j["warnings"]                  = warnings;
    j["estimated_duration_ms"]     = estimated_duration_ms;
    j["is_reversible"]             = is_reversible;
    
    j["affected_dependencies"] = nlohmann::json::array();
    for (const auto& dep : affected_dependencies) {
        j["affected_dependencies"].push_back(dep.toJson());
    }
    
    return j;
}

RollbackSafetyReport RollbackSafetyReport::fromJson(const nlohmann::json& j) {
    RollbackSafetyReport r;
    if (j.contains("rule_id")) {
        r.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("target_version")) {
        r.target_version = j["target_version"].get<std::string>();
    }
    if (j.contains("safety_level")) {
        r.safety_level = static_cast<RollbackSafetyLevel>(j["safety_level"].get<int>());
    }
    if (j.contains("conflicts")) {
        r.conflicts = j["conflicts"].get<std::vector<std::string>>();
    }
    if (j.contains("warnings")) {
        r.warnings = j["warnings"].get<std::vector<std::string>>();
    }
    if (j.contains("estimated_duration_ms")) {
        r.estimated_duration_ms = j["estimated_duration_ms"].get<int>();
    }
    if (j.contains("is_reversible")) {
        r.is_reversible = j["is_reversible"].get<bool>();
    }
    if (j.contains("affected_dependencies")) {
        for (const auto& dep_json : j["affected_dependencies"]) {
            r.affected_dependencies.push_back(PolicyDependency::fromJson(dep_json));
        }
    }
    return r;
}

// ========== RollbackOperation Implementation ==========

nlohmann::json RollbackOperation::toJson() const {
    nlohmann::json j;
    j["operation_id"]       = operation_id;
    j["rule_id"]            = rule_id;
    j["multi_rule_ids"]     = multi_rule_ids;
    j["from_version"]       = from_version;
    j["to_version"]         = to_version;
    j["operator_user"]      = operator_user;
    j["started_at"]         = started_at;
    j["completed_at"]       = completed_at;
    j["success"]            = success;
    j["error_message"]      = error_message;
    j["reason"]             = reason;
    return j;
}

RollbackOperation RollbackOperation::fromJson(const nlohmann::json& j) {
    RollbackOperation o;
    if (j.contains("operation_id")) {
        o.operation_id = j["operation_id"].get<std::string>();
    }
    if (j.contains("rule_id")) {
        o.rule_id = j["rule_id"].get<std::string>();
    }
    if (j.contains("multi_rule_ids")) {
        o.multi_rule_ids = j["multi_rule_ids"].get<std::vector<std::string>>();
    }
    if (j.contains("from_version")) {
        o.from_version = j["from_version"].get<std::string>();
    }
    if (j.contains("to_version")) {
        o.to_version = j["to_version"].get<std::string>();
    }
    if (j.contains("operator_user")) {
        o.operator_user = j["operator_user"].get<std::string>();
    }
    if (j.contains("started_at")) {
        o.started_at = j["started_at"].get<int64_t>();
    }
    if (j.contains("completed_at")) {
        o.completed_at = j["completed_at"].get<int64_t>();
    }
    if (j.contains("success")) {
        o.success = j["success"].get<bool>();
    }
    if (j.contains("error_message")) {
        o.error_message = j["error_message"].get<std::string>();
    }
    if (j.contains("reason")) {
        o.reason = j["reason"].get<std::string>();
    }
    return o;
}

// ========== PolicyChangeManager Implementation ==========

PolicyChangeManager::PolicyChangeManager() = default;

PolicyChangeManager::PolicyChangeManager(
    std::shared_ptr<PolicyManager> policy_manager,
    std::shared_ptr<PolicyVersionHistory> version_history
)
    : policy_manager_(policy_manager),
      version_history_(version_history) {}

void PolicyChangeManager::registerDependency(
    const std::string& dependent_rule_id,
    const std::string& dependency_rule_id,
    const std::string& dependency_type,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    PolicyDependency dep;
    dep.dependent_rule_id  = dependent_rule_id;
    dep.dependency_rule_id = dependency_rule_id;
    dep.dependency_type    = dependency_type;
    dep.reason             = reason;
    
    dependencies_[dependent_rule_id].push_back(dep);
    reverse_dependencies_[dependency_rule_id].push_back(dep);
}

std::vector<PolicyDependency> PolicyChangeManager::getDependencies(
    const std::string& rule_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = dependencies_.find(rule_id);
    if (it == dependencies_.end()) {
        return {};
    }
    
    return it->second;
}

std::vector<PolicyDependency> PolicyChangeManager::getReverseDependencies(
    const std::string& rule_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = reverse_dependencies_.find(rule_id);
    if (it == reverse_dependencies_.end()) {
        return {};
    }
    
    return it->second;
}

RollbackSafetyReport PolicyChangeManager::checkRollbackSafety(
    const std::string& rule_id,
    const std::string& target_version
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RollbackSafetyReport report;
    report.rule_id         = rule_id;
    report.target_version  = target_version;
    report.safety_level    = RollbackSafetyLevel::SAFE;
    report.is_reversible   = true;
    
    // Check if target version exists
    if (!version_history_) {
        report.safety_level = RollbackSafetyLevel::BLOCKED;
        report.warnings.push_back("Version history manager not available");
        return report;
    }
    
    auto version = version_history_->getVersion(rule_id, target_version);
    if (!version) {
        report.safety_level = RollbackSafetyLevel::BLOCKED;
        report.warnings.push_back(fmt::format("Target version {} not found", target_version));
        return report;
    }
    
    // Check for circular dependencies
    if (hasCircularDependency(rule_id)) {
        report.safety_level = RollbackSafetyLevel::WARNING;
        report.warnings.push_back("Circular dependency detected");
    }
    
    // Find affected rules
    auto affected = findAffectedRules(rule_id);
    report.affected_dependencies = getReverseDependencies(rule_id);
    
    if (!affected.empty()) {
        report.safety_level = RollbackSafetyLevel::WARNING;
        report.warnings.push_back(
            fmt::format("{} rules depend on this policy", affected.size())
        );
    }
    
    // Estimate duration
    report.estimated_duration_ms = 50 + (affected.size() * 10);
    
    return report;
}

RollbackSafetyReport PolicyChangeManager::previewRollback(
    const std::string& rule_id,
    const std::string& target_version
) {
    return checkRollbackSafety(rule_id, target_version);
}

RollbackOperation PolicyChangeManager::performRollback(
    const std::string& rule_id,
    const std::string& target_version,
    const std::string& operator_user,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RollbackOperation operation;
    operation.operation_id   = generateOperationId();
    operation.rule_id        = rule_id;
    operation.target_version = target_version;
    operation.operator_user  = operator_user;
    operation.reason         = reason;
    operation.started_at     = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Mark as in-progress
    in_progress_rollbacks_[rule_id] = operation.operation_id;
    
    // Perform rollback
    if (executeRollback(rule_id, target_version, operation)) {
        operation.success = true;
    } else {
        operation.success = false;
        operation.error_message = "Failed to apply rollback atomically";
    }
    
    operation.completed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Remove from in-progress
    in_progress_rollbacks_.erase(rule_id);
    
    // Record operation
    rollback_history_.push_back(operation);
    
    return operation;
}

RollbackOperation PolicyChangeManager::performCoordinatedRollback(
    const std::vector<std::string>& rule_ids,
    const std::string& target_version,
    const std::string& operator_user,
    const std::string& reason
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (rule_ids.empty()) {
        RollbackOperation op;
        op.success = false;
        op.error_message = "No rules specified";
        return op;
    }
    
    RollbackOperation operation;
    operation.operation_id   = generateOperationId();
    operation.rule_id        = rule_ids[0];
    operation.multi_rule_ids = rule_ids;
    operation.target_version = target_version;
    operation.operator_user  = operator_user;
    operation.reason         = reason;
    operation.started_at     = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Mark all as in-progress
    for (const auto& rule_id : rule_ids) {
        in_progress_rollbacks_[rule_id] = operation.operation_id;
    }
    
    // Perform coordinated rollback - if any fails, all fail
    bool all_success = true;
    for (const auto& rule_id : rule_ids) {
        if (!executeRollback(rule_id, target_version, operation)) {
            all_success = false;
            operation.error_message = fmt::format("Failed to rollback rule {}", rule_id);
            break;
        }
    }
    
    operation.success = all_success;
    operation.completed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // Remove from in-progress
    for (const auto& rule_id : rule_ids) {
        in_progress_rollbacks_.erase(rule_id);
    }
    
    // Record operation
    rollback_history_.push_back(operation);
    
    return operation;
}

RollbackOperation PolicyChangeManager::rollbackToPrevious(
    const std::string& rule_id,
    const std::string& operator_user,
    const std::string& reason
) {
    if (!version_history_) {
        RollbackOperation op;
        op.success = false;
        op.error_message = "Version history manager not available";
        return op;
    }
    
    auto prev_version = version_history_->getPreviousVersion(rule_id);
    if (!prev_version) {
        RollbackOperation op;
        op.success = false;
        op.error_message = "No previous version found";
        return op;
    }
    
    return performRollback(rule_id, prev_version.value(), operator_user, reason);
}

std::optional<RollbackOperation> PolicyChangeManager::reverseRollback(
    const std::string& operation_id,
    const std::string& operator_user
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(
        rollback_history_.begin(),
        rollback_history_.end(),
        [&operation_id](const RollbackOperation& op) {
            return op.operation_id == operation_id;
        }
    );
    
    if (it == rollback_history_.end()) {
        return std::nullopt;
    }
    
    if (!it->is_reversible) {
        return std::nullopt;
    }
    
    // Create reverse operation
    RollbackOperation reverse_op;
    reverse_op.operation_id   = generateOperationId();
    reverse_op.rule_id        = it->rule_id;
    reverse_op.multi_rule_ids = it->multi_rule_ids;
    reverse_op.from_version   = it->to_version;
    reverse_op.to_version     = it->from_version;
    reverse_op.operator_user  = operator_user;
    reverse_op.reason         = fmt::format("Reverse of {}", operation_id);
    reverse_op.started_at     = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (executeRollback(it->rule_id, it->from_version, reverse_op)) {
        reverse_op.success = true;
    } else {
        reverse_op.success = false;
    }
    
    reverse_op.completed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    rollback_history_.push_back(reverse_op);
    return reverse_op;
}

std::vector<RollbackOperation> PolicyChangeManager::getRollbackHistory(
    const std::optional<std::string>& rule_id,
    const std::optional<int64_t>& start_time,
    const std::optional<int64_t>& end_time
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RollbackOperation> result;
    
    for (const auto& op : rollback_history_) {
        if (rule_id && op.rule_id != rule_id.value()) {
            continue;
        }
        if (start_time && op.started_at < start_time.value()) {
            continue;
        }
        if (end_time && op.completed_at > end_time.value()) {
            continue;
        }
        
        result.push_back(op);
    }
    
    return result;
}

std::optional<RollbackOperation> PolicyChangeManager::getRollbackOperation(
    const std::string& operation_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(
        rollback_history_.begin(),
        rollback_history_.end(),
        [&operation_id](const RollbackOperation& op) {
            return op.operation_id == operation_id;
        }
    );
    
    if (it == rollback_history_.end()) {
        return std::nullopt;
    }
    
    return *it;
}

bool PolicyChangeManager::isRollbackInProgress(const std::string& rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return in_progress_rollbacks_.find(rule_id) != in_progress_rollbacks_.end();
}

nlohmann::json PolicyChangeManager::exportChangeData() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json j;
    
    j["dependencies"] = nlohmann::json::object();
    for (const auto& [rule_id, deps] : dependencies_) {
        j["dependencies"][rule_id] = nlohmann::json::array();
        for (const auto& dep : deps) {
            j["dependencies"][rule_id].push_back(dep.toJson());
        }
    }
    
    j["rollback_history"] = nlohmann::json::array();
    for (const auto& op : rollback_history_) {
        j["rollback_history"].push_back(op.toJson());
    }
    
    return j;
}

bool PolicyChangeManager::importChangeData(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!j.contains("dependencies") || !j.contains("rollback_history")) {
        return false;
    }
    
    dependencies_.clear();
    reverse_dependencies_.clear();
    rollback_history_.clear();
    
    for (const auto& [rule_id, deps_json] : j["dependencies"].items()) {
        for (const auto& dep_json : deps_json) {
            auto dep = PolicyDependency::fromJson(dep_json);
            dependencies_[rule_id].push_back(dep);
            reverse_dependencies_[dep.dependency_rule_id].push_back(dep);
        }
    }
    
    for (const auto& op_json : j["rollback_history"]) {
        rollback_history_.push_back(RollbackOperation::fromJson(op_json));
    }
    
    return true;
}

bool PolicyChangeManager::saveToFile(const std::string& path) const {
    try {
        auto json = exportChangeData();
        std::ofstream file(path);
        file << json.dump(2);
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool PolicyChangeManager::loadFromFile(const std::string& path) {
    try {
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        return importChangeData(json);
    } catch (...) {
        return false;
    }
}

void PolicyChangeManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    dependencies_.clear();
    reverse_dependencies_.clear();
    rollback_history_.clear();
    in_progress_rollbacks_.clear();
}

std::string PolicyChangeManager::generateOperationId() {
    static uint64_t counter = 0;
    return fmt::format("rollback-{}-{}", 
        std::chrono::system_clock::now().time_since_epoch().count(), 
        counter++);
}

bool PolicyChangeManager::hasCircularDependency(const std::string& rule_id) const {
    // Simple DFS-based cycle detection
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> rec_stack;
    
    std::function<bool(const std::string&)> hasCycle = [&]([[maybe_unused]] const std::string& node) -> bool {
        visited.insert(node);
        rec_stack.insert(node);
        
        auto deps_it = dependencies_.find(node);
        if (deps_it != dependencies_.end()) {
            for (const auto& dep : deps_it->second) {
                if (visited.find(dep.dependency_rule_id) == visited.end()) {
                    if (hasCycle(dep.dependency_rule_id)) {
                        return true;
                    }
                } else if (rec_stack.find(dep.dependency_rule_id) != rec_stack.end()) {
                    return true;
                }
            }
        }
        
        rec_stack.erase(node);
        return false;
    };
    
    return hasCycle(rule_id);
}

std::vector<std::string> PolicyChangeManager::findAffectedRules(
    const std::string& rule_id
) const {
    std::vector<std::string> affected;
    
    auto rev_deps_it = reverse_dependencies_.find(rule_id);
    if (rev_deps_it != reverse_dependencies_.end()) {
        for (const auto& dep : rev_deps_it->second) {
            affected.push_back(dep.dependent_rule_id);
        }
    }
    
    return affected;
}

bool PolicyChangeManager::canApplyAtomically(
    const std::string& rule_id,
    const std::string& target_version
) {
    // Check if all preconditions are met for atomic application
    if (!version_history_) {
        return false;
    }
    
    auto version = version_history_->getVersion(rule_id, target_version);
    return version.has_value();
}

bool PolicyChangeManager::executeRollback(
    const std::string& rule_id,
    const std::string& target_version,
    RollbackOperation& operation
) {
    if (!canApplyAtomically(rule_id, target_version)) {
        return false;
    }
    
    operation.from_version = version_history_->getLatestVersion(rule_id);
    operation.to_version   = target_version;
    
    // TODO: Implement actual rollback operation with policy manager
    // [RESOLVED] — delegate to PolicyManager::rollbackToVersion().
    if (!policy_manager_) {
        operation.error_message = "PolicyManager not available — cannot execute rollback";
        return false;
    }
    const std::string& modified_by =
        operation.operator_user.empty() ? "system" : operation.operator_user;
    const bool ok = policy_manager_->rollbackToVersion(
        rule_id, target_version, modified_by);
    if (!ok) {
        operation.error_message =
            "PolicyManager::rollbackToVersion failed for rule=" + rule_id +
            " target=" + target_version;
    }
    return ok;
}

} // namespace governance
} // namespace themis
