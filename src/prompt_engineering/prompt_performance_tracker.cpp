/**
 * @file prompt_performance_tracker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/prompt_performance_tracker.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <ctime>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// PromptMetrics Implementation
// ============================================================================

nlohmann::json PromptMetrics::toJson() const {
    nlohmann::json j;
    j["prompt_id"] = prompt_id;
    j["success_rate"] = success_rate;
    j["avg_latency_ms"] = avg_latency_ms;
    j["user_satisfaction"] = user_satisfaction;
    j["total_executions"] = total_executions;
    j["failed_executions"] = failed_executions;
    j["feedback_count"] = feedback_count;
    
    auto last_updated_time = std::chrono::system_clock::to_time_t(last_updated);
    j["last_updated"] = last_updated_time;
    
    auto created_time = std::chrono::system_clock::to_time_t(created_at);
    j["created_at"] = created_time;
    
    return j;
}

PromptMetrics PromptMetrics::fromJson(const nlohmann::json& j) {
    PromptMetrics m;
    m.prompt_id = j.value("prompt_id", "");
    m.success_rate = j.value("success_rate", 0.0);
    m.avg_latency_ms = j.value("avg_latency_ms", 0.0);
    m.user_satisfaction = j.value("user_satisfaction", 0.0);
    m.total_executions = j.value("total_executions", 0);
    m.failed_executions = j.value("failed_executions", 0);
    m.feedback_count = j.value("feedback_count", 0);
    
    if (j.contains("last_updated")) {
        auto time_val = j["last_updated"].get<std::time_t>();
        m.last_updated = std::chrono::system_clock::from_time_t(time_val);
    }
    
    if (j.contains("created_at")) {
        auto time_val = j["created_at"].get<std::time_t>();
        m.created_at = std::chrono::system_clock::from_time_t(time_val);
    }
    
    return m;
}

// ============================================================================
// PromptPerformanceTracker Implementation
// ============================================================================

PromptPerformanceTracker::PromptPerformanceTracker() = default;

PromptPerformanceTracker::PromptPerformanceTracker(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {
    if (db_) {
        loadFromDB();
    }
}

void PromptPerformanceTracker::recordExecution(
    const std::string& prompt_id,
    bool success,
    double latency_ms,
    double user_feedback
) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(prompt_id);
    if (it == metrics_.end()) {
        // Create new metrics entry
        PromptMetrics metrics;
        metrics.prompt_id = prompt_id;
        metrics.created_at = std::chrono::system_clock::now();
        metrics.last_updated = metrics.created_at;
        
        auto result = metrics_.emplace(prompt_id, metrics);
        it = result.first;
        
        THEMIS_DEBUG("Created new metrics for prompt: {}", prompt_id);
    }
    
    // Update metrics using incremental averaging
    updateAverages(it->second, success, latency_ms, user_feedback);
    
    // Persist to DB if available
    if (db_) {
        persist(prompt_id, it->second);
    }

    // Safe logging note: this tracker emits only prompt identifiers and
    // aggregate execution metrics. It never logs prompt bodies, user content,
    // bearer tokens, or other credential material.
    THEMIS_DEBUG("Recorded execution for prompt {}: success={}, latency={}ms, success_rate={:.2f}",
                 prompt_id, success, latency_ms, it->second.success_rate);
}

std::optional<PromptMetrics> PromptPerformanceTracker::getMetrics(const std::string& prompt_id) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(prompt_id);
    if (it != metrics_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<PromptMetrics> PromptPerformanceTracker::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::vector<PromptMetrics> result = {};

    result.reserve(metrics_.size());
    
    for (const auto& [id, metrics] : metrics_) {
        result.push_back(metrics);
    }
    
    return result;
}

std::vector<std::string> PromptPerformanceTracker::getLowPerformingPrompts(
    double threshold,
    size_t min_executions
) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::vector<std::string> low_performers;
    
    for (const auto& [id, metrics] : metrics_) {
        if (metrics.total_executions >= min_executions &&
            metrics.success_rate < threshold) {
            low_performers.push_back(id);
        }
    }
    
    THEMIS_INFO("Found {} low-performing prompts (threshold={:.2f}, min_executions={})",
                low_performers.size(), threshold, min_executions);
    
    return low_performers;
}

std::vector<std::pair<std::string, double>> PromptPerformanceTracker::getTopPerformingPrompts(
    size_t count,
    size_t min_executions
) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Filter prompts with sufficient executions
    std::vector<std::pair<std::string, double>> candidates;
    for (const auto& [id, metrics] : metrics_) {
        if (metrics.total_executions >= min_executions) {
            candidates.emplace_back(id, metrics.success_rate);
        }
    }
    
    // Sort by success rate (descending)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top N
    if (static_cast<int>(candidates.size()) > count) {
        candidates.resize(count);
    }
    
    return candidates;
}

bool PromptPerformanceTracker::resetMetrics(const std::string& prompt_id) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(prompt_id);
    if (it == metrics_.end()) {
        return false;
    }
    
    metrics_.erase(it);
    
    // Remove from DB if available
    if (db_) {
        std::string key = std::string(KEY_PREFIX) + prompt_id;
        db_->del(key);
    }
    
    THEMIS_INFO("Reset metrics for prompt: {}", prompt_id);
    return true;
}

void PromptPerformanceTracker::clearAllMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    metrics_.clear();
    
    // Clear from DB if available
    if (db_) {
        std::string prefix = KEY_PREFIX;
        db_->scanPrefix(prefix, [this](std::string_view key, std::string_view) -> bool {
            db_->del(std::string(key));
            return true; // continue scanning
        });
    }
    
    THEMIS_INFO("Cleared all prompt metrics");
}

nlohmann::json PromptPerformanceTracker::getSummaryStatistics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    if (metrics_.empty()) {
        return nlohmann::json{
            {"total_prompts", 0},
            {"total_executions", 0},
            {"avg_success_rate", 0.0},
            {"avg_latency_ms", 0.0},
            {"avg_user_satisfaction", 0.0}
        };
    }
    
    double sum_success_rate = 0.0;
    double sum_latency = 0.0;
    double sum_satisfaction = 0.0;
    size_t total_executions = 0;
    size_t prompts_with_satisfaction = 0;
    
    for (const auto& [id, metrics] : metrics_) {
        sum_success_rate += metrics.success_rate;
        sum_latency += metrics.avg_latency_ms;
        total_executions += metrics.total_executions;
        
        if (metrics.user_satisfaction > 0.0) {
            sum_satisfaction += metrics.user_satisfaction;
            prompts_with_satisfaction++;
        }
    }
    
    size_t num_prompts = metrics_.size();
    
    return nlohmann::json{
        {"total_prompts", num_prompts},
        {"total_executions", total_executions},
        {"avg_success_rate", sum_success_rate / num_prompts},
        {"avg_latency_ms", sum_latency / num_prompts},
        {"avg_user_satisfaction", prompts_with_satisfaction > 0 ? 
            sum_satisfaction / prompts_with_satisfaction : 0.0}
    };
}

// ============================================================================
// Private Methods
// ============================================================================

void PromptPerformanceTracker::persist(const std::string& prompt_id, const PromptMetrics& metrics) {
    if (!db_) {
      return;
    }
    
    std::string key = std::string(KEY_PREFIX) + prompt_id;
    std::string value = metrics.toJson().dump();
    std::vector<uint8_t> bytes(value.begin(), value.end());
    
    if (!db_->put(key, bytes)) {
        THEMIS_ERROR("Failed to persist metrics for prompt: {}", prompt_id);
    }
}

void PromptPerformanceTracker::loadFromDB() {
    if (!db_) {
      return;
    }
    
    std::string prefix = KEY_PREFIX;
    size_t loaded = 0;
    
    db_->scanPrefix(prefix, [this, &loaded](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto j = nlohmann::json::parse(std::string(value));
            auto metrics = PromptMetrics::fromJson(j);
            metrics_[metrics.prompt_id] = metrics;
            loaded++;
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to parse metrics from DB: {}", e.what());
        }
        return true; // continue scanning
    });
    
    THEMIS_INFO("Loaded {} prompt metrics from DB", loaded);
}

void PromptPerformanceTracker::updateAverages(
    PromptMetrics& metrics,
    bool success,
    double latency_ms,
    double user_feedback
) {
    // Update execution counts
    metrics.total_executions++;
    if (!success) {
        metrics.failed_executions++;
    }
    
    // Update success rate (incremental average)
    size_t n = metrics.total_executions;
    metrics.success_rate = ((metrics.success_rate * (n - 1)) + (success ? 1.0 : 0.0)) / n;
    
    // Update average latency (incremental average)
    metrics.avg_latency_ms = ((metrics.avg_latency_ms * (n - 1)) + latency_ms) / n;
    
    // Update user satisfaction if provided
    if (user_feedback > 0.0) {
        metrics.feedback_count++;
        size_t fb_count = metrics.feedback_count;
        
        // Calculate incremental average for satisfaction
        metrics.user_satisfaction = ((metrics.user_satisfaction * (fb_count - 1)) + user_feedback) / fb_count;
    }
    
    // Update timestamp
    metrics.last_updated = std::chrono::system_clock::now();
}

} // namespace prompt_engineering
} // namespace themis
