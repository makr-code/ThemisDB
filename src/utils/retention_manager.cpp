/**
 * @file retention_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/retention_manager.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace vcc {

RetentionManager::RetentionManager(const std::string& config_path)
    : audit_enabled_(true) {
    
    if (!config_path.empty()) {
        if (!loadPolicies(config_path)) {
            spdlog::warn("RetentionManager: Failed to load policies from '{}', starting empty", config_path);
        }
    }
    
    spdlog::info("RetentionManager: Initialized with {} policy/policies", policies_.size());
}

bool RetentionManager::registerPolicy(const RetentionPolicy& policy) {
    if (policy.name.empty()) {
        last_error_ = "Policy name cannot be empty";
        return false;
    }
    
    if (policy.retention_period.count() <= 0) {
        last_error_ = "Retention period must be positive";
        return false;
    }
    
    policies_[policy.name] = policy;
    
    // Initialize stats for this policy
    if (policy_stats_.find(policy.name) == policy_stats_.end()) {
        policy_stats_[policy.name] = RetentionStats{};
    }
    
    spdlog::info("RetentionManager: Registered policy '{}' (retention={}s, archive={}s)", 
                 policy.name, policy.retention_period.count(), policy.archive_after.count());
    
    return true;
}

bool RetentionManager::removePolicy(const std::string& policy_name) {
    auto it = policies_.find(policy_name);
    if (it == policies_.end()) {
        last_error_ = "Policy not found: " + policy_name;
        return false;
    }
    
    policies_.erase(it);
    spdlog::info("RetentionManager: Removed policy '{}'", policy_name);
    return true;
}

std::vector<RetentionManager::RetentionPolicy> RetentionManager::getPolicies() const {
    std::vector<RetentionPolicy> result;
    result.reserve(policies_.size());
    
    for (const auto& [name, policy] : policies_) {
        result.push_back(policy);
    }
    
    return result;
}

themis::Result<const RetentionManager::RetentionPolicy*> RetentionManager::getPolicy(const std::string& policy_name) const {
    auto it = policies_.find(policy_name);
    if (it == policies_.end()) {
        return themis::Err<const RetentionPolicy*>(
            themis::errors::ErrorCode::ERR_UTIL_POLICY_NOT_FOUND,
            "Retention policy not found: " + policy_name);
    }
    return themis::Ok(&it->second);
}

bool RetentionManager::shouldArchive(
    [[maybe_unused]] const std::string& entity_id,
    std::chrono::system_clock::time_point created_at,
    const std::string& policy_name) const {
    
    auto policy_result = getPolicy(policy_name);
    if (!policy_result) {
        return false;
    }
    const auto* policy = *policy_result;
    
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - created_at);
    
    return age >= policy->archive_after;
}

bool RetentionManager::shouldPurge(
    [[maybe_unused]] const std::string& entity_id,
    std::chrono::system_clock::time_point created_at,
    const std::string& policy_name) const {
    
    auto policy_result = getPolicy(policy_name);
    if (!policy_result) {
        return false;
    }
    const auto* policy = *policy_result;
    
    if (!policy->auto_purge_enabled) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - created_at);
    
    return age >= policy->retention_period;
}

RetentionManager::RetentionAction RetentionManager::archiveEntity(
    const std::string& entity_id,
    const std::string& policy_name,
    std::function<bool([[maybe_unused]] const std::string&)> archive_handler) {
    
    RetentionAction action;
    action.entity_id = entity_id;
    action.action = "archived";
    action.policy_name = policy_name;
    action.timestamp = std::chrono::system_clock::now();
    
    auto policy_result = getPolicy(policy_name);
    if (!policy_result) {
        action.success = false;
        action.error_message = "Policy not found: " + policy_name;
        logAction(action);
        return action;
    }
    try {
        action.success = archive_handler([[maybe_unused]] entity_id);
        if (!action.success) {
            action.error_message = "Archive handler returned false";
        }
    } catch (const std::exception& e) {
        action.success = false;
        action.error_message = std::string("Archive exception: ") + e.what();
    }
    
    logAction(action);
    
    // Update stats
    if (action.success) {
        policy_stats_[policy_name].archived_count++;
    } else {
        policy_stats_[policy_name].error_count++;
    }
    
    return action;
}

RetentionManager::RetentionAction RetentionManager::purgeEntity(
    const std::string& entity_id,
    const std::string& policy_name,
    std::function<bool([[maybe_unused]] const std::string&)> purge_handler) {
    
    RetentionAction action;
    action.entity_id = entity_id;
    action.action = "purged";
    action.policy_name = policy_name;
    action.timestamp = std::chrono::system_clock::now();
    
    auto policy_result = getPolicy(policy_name);
    if (!policy_result) {
        action.success = false;
        action.error_message = "Policy not found: " + policy_name;
        logAction(action);
        return action;
    }
    const auto* policy = *policy_result;
    
    if (!policy->auto_purge_enabled) {
        action.success = false;
        action.error_message = "Auto-purge not enabled for policy: " + policy_name;
        logAction(action);
        return action;
    }
    
    try {
        action.success = purge_handler([[maybe_unused]] entity_id);
        if (!action.success) {
            action.error_message = "Purge handler returned false";
        }
    } catch (const std::exception& e) {
        action.success = false;
        action.error_message = std::string("Purge exception: ") + e.what();
    }
    
    logAction(action);
    
    // Update stats
    if (action.success) {
        policy_stats_[policy_name].purged_count++;
    } else {
        policy_stats_[policy_name].error_count++;
    }
    
    return action;
}

RetentionManager::RetentionStats RetentionManager::runRetentionCheck(
    std::function<std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>(const std::string&)> entity_provider,
    std::function<bool(const std::string&)> archive_handler,
    std::function<bool([[maybe_unused]] const std::string&)> purge_handler) {
    
    auto start = std::chrono::steady_clock::now();
    
    RetentionStats total_stats{};
    
    for (const auto& [policy_name, policy] : policies_) {
        spdlog::info("RetentionManager: Running retention check for policy '{}'", policy_name);
        
        auto entities = entity_provider(policy_name);
        total_stats.total_entities_scanned += entities.size();
        
        for (const auto& [entity_id, created_at] : entities) {
            // Check if should purge first (more restrictive)
            if (shouldPurge(entity_id, created_at, policy_name)) {
                auto action = purgeEntity(entity_id, policy_name, purge_handler);
                if (action.success) {
                    total_stats.purged_count++;
                } else {
                    total_stats.error_count++;
                }
            }
            // Otherwise check if should archive
            else if (shouldArchive(entity_id, created_at, policy_name)) {
                auto action = archiveEntity(entity_id, policy_name, archive_handler);
                if (action.success) {
                    total_stats.archived_count++;
                } else {
                    total_stats.error_count++;
                }
            }
            // Entity is still within retention period
            else {
                total_stats.retained_count++;
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    total_stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    spdlog::info("RetentionManager: Check complete - scanned={}, archived={}, purged={}, retained={}, errors={}, duration={}ms",
                 total_stats.total_entities_scanned,
                 total_stats.archived_count,
                 total_stats.purged_count,
                 total_stats.retained_count,
                 total_stats.error_count,
                 total_stats.duration.count());
    
    return total_stats;
}

std::vector<RetentionManager::RetentionAction> RetentionManager::getHistory([[maybe_unused]] size_t limit) const {
    if (limit == 0 || limit >= action_history_.size()) {
        return action_history_;
    }
    
    // Return the most recent 'limit' actions
    auto start_it = action_history_.end() - static_cast<std::ptrdiff_t>(limit);
    return std::vector<RetentionAction>(start_it, action_history_.end());
}

RetentionManager::RetentionStats RetentionManager::getPolicyStats(const std::string& policy_name) const {
    auto it = policy_stats_.find(policy_name);
    if (it == policy_stats_.end()) {
        return RetentionStats{};
    }
    return it->second;
}

bool RetentionManager::loadPolicies(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);

        YAML::Node policy_list;
        bool using_legacy_schema = false;
        if (config["retention_policies"]) {
            policy_list = config["retention_policies"];
            using_legacy_schema = true;
        } else if (config["policies"]) {
            policy_list = config["policies"];
        } else {
            last_error_ = "No policy section found in config (expected 'retention_policies' or 'policies')";
            return false;
        }

        for (const auto& policy_node : policy_list) {
            RetentionPolicy policy;

            policy.name = policy_node["name"].as<std::string>();

            // Backward-compatible schema support:
            // - legacy: retention_period_days/archive_after_days/auto_purge_enabled
            // - modern: retention_days/archive_days/auto_purge
            const int retention_days = using_legacy_schema
                ? policy_node["retention_period_days"].as<int>()
                : policy_node["retention_days"].as<int>();
            const int archive_days = using_legacy_schema
                ? policy_node["archive_after_days"].as<int>(0)
                : policy_node["archive_days"].as<int>(0);

            policy.retention_period = std::chrono::seconds(static_cast<int64_t>(retention_days) * 86400);
            policy.archive_after = std::chrono::seconds(static_cast<int64_t>(archive_days) * 86400);
            policy.auto_purge_enabled = using_legacy_schema
                ? policy_node["auto_purge_enabled"].as<bool>(false)
                : policy_node["auto_purge"].as<bool>(false);
            policy.require_audit_trail = policy_node["require_audit_trail"].as<bool>(true);
            policy.classification_level = policy_node["classification_level"].as<std::string>("offen");

            if (policy_node["metadata"]) {
                // Keep metadata as an object for future enrichment.
                policy.metadata = nlohmann::json::object();
            }

            registerPolicy(policy);
        }

        spdlog::info("RetentionManager: Loaded {} policies from '{}'", policies_.size(), config_path);
        return true;

    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to load policies: ") + e.what();
        spdlog::error("RetentionManager: {}", last_error_);
        return false;
    }
}

void RetentionManager::logAction(const RetentionAction& action) {
    // Add to history (keep last 10000 actions)
    action_history_.push_back(action);
    if (action_history_.size() > 10000) {
        action_history_.erase(action_history_.begin());
    }
    
    // Log to spdlog
    if (audit_enabled_) {
        if (action.success) {
            spdlog::info("RetentionManager: Action {} for entity '{}' under policy '{}' succeeded",
                         action.action, action.entity_id, action.policy_name);
        } else {
            spdlog::warn("RetentionManager: Action {} for entity '{}' under policy '{}' failed: {}",
                         action.action, action.entity_id, action.policy_name, action.error_message);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: Async Background Job & Compliance Metrics
// ─────────────────────────────────────────────────────────────────────────────

void RetentionManager::startBackgroundJob(
    std::chrono::seconds interval,
    std::function<std::vector<std::pair<std::string,
        std::chrono::system_clock::time_point>>(const std::string&)> entity_provider,
    std::function<bool(const std::string&)> archive_handler,
    std::function<bool([[maybe_unused]] const std::string&)> purge_handler) {

    if (bg_running_.exchange(true)) {
        // Already running
        return;
    }

    {
        std::lock_guard<std::mutex> lk(bg_mutex_);
        bg_stop_ = false;
    }

    bg_thread_ = std::thread([this, interval,
                               ep = std::move(entity_provider),
                               ah = std::move(archive_handler),
                               ph = std::move([[maybe_unused]] purge_handler)]() {
        while (true) {
            // Wait for the interval or until stopped
            std::unique_lock<std::mutex> lk(bg_mutex_);
            bool stopped = bg_cv_.wait_for(lk, interval, [this]{ return bg_stop_; });
            if (stopped) break;
            lk.unlock();

            try {
                auto stats = runRetentionCheck(ep, ah, ph);

                std::lock_guard<std::mutex> mlk(metrics_mutex_);
                compliance_metrics_.entities_archived   += stats.archived_count;
                compliance_metrics_.entities_purged     += stats.purged_count;
                compliance_metrics_.policies_active      = policies_.size();
                compliance_metrics_.last_run             = std::chrono::system_clock::now();
                compliance_metrics_.last_run_success     = true;
            } catch (const std::exception& e) {
                spdlog::error("RetentionManager background job error: {}", e.what());
                std::lock_guard<std::mutex> mlk(metrics_mutex_);
                compliance_metrics_.last_run_success = false;
            }
        }
        bg_running_.store(false);
    });
}

void RetentionManager::stopBackgroundJob() {
    {
        std::lock_guard<std::mutex> lk(bg_mutex_);
        bg_stop_ = true;
    }
    bg_cv_.notify_all();
    if (bg_thread_.joinable()) {
        bg_thread_.join();
    }
    bg_running_.store(false);
}

bool RetentionManager::isBackgroundJobRunning() const {
    return bg_running_.load();
}

RetentionManager::ComplianceMetrics RetentionManager::getComplianceMetrics() const {
    std::lock_guard<std::mutex> lk(metrics_mutex_);
    auto m = compliance_metrics_;
    m.policies_active = policies_.size();
    return m;
}

} // namespace vcc

