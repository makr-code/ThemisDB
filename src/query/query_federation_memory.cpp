/**
 * @file query_federation_memory.cpp
 * @brief Implementation of bounded memory accumulation for federated queries
 * @version 0.0.1
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 */

#include "query/query_federation_memory.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace themis::query {

// ============================================================================
// MemoryPolicy Implementation
// ============================================================================

MemoryPolicy MemoryPolicy::Builder::build() const {
    if (max_result_bytes_ == 0) {
        throw std::invalid_argument("max_result_bytes must be positive");
    }
    if (elevated_threshold_pct_ <= 0.0 || elevated_threshold_pct_ > 100.0) {
        throw std::invalid_argument("elevated_threshold_pct must be in (0, 100]");
    }
    if (high_threshold_pct_ <= elevated_threshold_pct_) {
        throw std::invalid_argument("high_threshold_pct must be > elevated_threshold_pct");
    }
    if (critical_threshold_pct_ <= high_threshold_pct_) {
        throw std::invalid_argument("critical_threshold_pct must be > high_threshold_pct");
    }

    return MemoryPolicy(
        max_result_bytes_,
        overflow_policy_,
        elevated_threshold_pct_,
        high_threshold_pct_,
        critical_threshold_pct_,
        max_batches_per_shard_);
}

MemoryPolicy::MemoryPolicy(
    uint64_t max_result_bytes,
    OverflowPolicy overflow_policy,
    double elevated_threshold_pct,
    double high_threshold_pct,
    double critical_threshold_pct,
    size_t max_batches_per_shard)
    : max_result_bytes_(max_result_bytes),
      overflow_policy_(overflow_policy),
      elevated_threshold_pct_(elevated_threshold_pct),
      high_threshold_pct_(high_threshold_pct),
      critical_threshold_pct_(critical_threshold_pct),
      max_batches_per_shard_(max_batches_per_shard) {
    spdlog::debug(
        "MemoryPolicy constructed: max_bytes={}, overflow={}, "
        "pressure_thresholds={}%/{}%/{}%",
        max_result_bytes_,
        static_cast<int>(overflow_policy_),
        elevated_threshold_pct_,
        high_threshold_pct_,
        critical_threshold_pct_);
}

MemoryPolicy::PressureLevel MemoryPolicy::getPressureLevel(
    uint64_t current_bytes) const {
    const double util_pct = getUtilizationPercent(current_bytes);

    if (util_pct >= critical_threshold_pct_) {
        return PressureLevel::CRITICAL;
    } else if (util_pct >= high_threshold_pct_) {
        return PressureLevel::HIGH;
    } else if (util_pct >= elevated_threshold_pct_) {
        return PressureLevel::ELEVATED;
    }
    return PressureLevel::NORMAL;
}

bool MemoryPolicy::isUnderPressure([[maybe_unused]] uint64_t current_bytes) const {
    return getPressureLevel(current_bytes) >= PressureLevel::ELEVATED;
}

void MemoryPolicy::recordPressureEvent(const MemoryPressureEvent& event) const {
    pressure_events_.push_back(event);

    std::string level_str = {};
    switch (event.level) {
        case PressureLevel::NORMAL:
            level_str = "NORMAL";
            break;
        case PressureLevel::ELEVATED:
            level_str = "ELEVATED";
            break;
        case PressureLevel::HIGH:
            level_str = "HIGH";
            break;
        case PressureLevel::CRITICAL:
            level_str = "CRITICAL";
            break;
    }

    spdlog::warn(
        "Memory pressure event: level={}, utilization={}%, "
        "current={}/max={}, shard={}, details={}",
        level_str,
        event.utilization_percent,
        event.current_bytes,
        event.max_bytes,
        event.shard_id,
        event.details);
}

std::vector<MemoryPolicy::MemoryPressureEvent> MemoryPolicy::getPressureEvents()
    const {
    return pressure_events_;
}

void MemoryPolicy::clearEvents() {
    pressure_events_.clear();
}

std::string MemoryPolicy::getSummary() const {
    std::string summary = "MemoryPolicy:\n";
    summary += "  max_result_bytes: " + std::to_string(max_result_bytes_) + "\n";
    summary += "  overflow_policy: ";
    switch (overflow_policy_) {
        case OverflowPolicy::REJECT:
            summary += "REJECT";
            break;
        case OverflowPolicy::DROP_OLDEST:
            summary += "DROP_OLDEST";
            break;
        case OverflowPolicy::TRUNCATE:
            summary += "TRUNCATE";
            break;
    }
    summary += "\n";
    summary += "  pressure_thresholds: " + std::to_string(elevated_threshold_pct_) +
               "% / " + std::to_string(high_threshold_pct_) + "% / " +
               std::to_string(critical_threshold_pct_) + "%\n";
    summary += "  pressure_events: " + std::to_string(pressure_events_.size()) + "\n";
    return summary;
}

// ============================================================================
// ResultAccumulator Implementation
// ============================================================================

ResultAccumulator::ResultAccumulator(const MemoryPolicy& policy)
    : policy_(policy) {
    spdlog::debug("ResultAccumulator constructed");
}

bool ResultAccumulator::addResult(
    const std::string& shard_id,
    const nlohmann::json& result) {
    const auto size_bytes = estimateJsonSize(result);
    return addResultWithSize(shard_id, result, size_bytes);
}

bool ResultAccumulator::addResultWithSize(
    const std::string& shard_id,
    const nlohmann::json& result,
    uint64_t size_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if adding this result would exceed memory limit
    if (current_memory_bytes_ + size_bytes > policy_.getMaxResultBytes()) {
        spdlog::warn(
            "Memory limit would be exceeded: current={}, needed={}, max={}",
            current_memory_bytes_,
            size_bytes,
            policy_.getMaxResultBytes());

        handleMemoryPressure(size_bytes);

        // Check if still over limit after handling pressure
        if (current_memory_bytes_ + size_bytes > policy_.getMaxResultBytes()) {
            if (policy_.getOverflowPolicy() == MemoryPolicy::OverflowPolicy::REJECT) {
                throw std::runtime_error(
                    "ResultAccumulator: Memory limit exceeded and REJECT policy active");
            }
            return false;
        }
    }

    // Check if shard has too many batches
    auto& shard_batches = shard_batches_[shard_id];
    if (static_cast<int>(shard_batches.size()) > = policy_.max_batches_per_shard_) {
        spdlog::warn(
            "Shard {} has exceeded max batches: {}",
            shard_id,
            shard_batches.size());
        dropOldestBatch();
    }

    // Add the batch
    MemoryPolicy::ResultBatch batch{
        total_batch_count_++,
        size_bytes,
        shard_id,
        std::chrono::steady_clock::now(),
        result};

    shard_batches.push_back(batch);
    current_memory_bytes_ += size_bytes;

    // Record pressure event if transitioning to pressure
    const auto pressure_level = policy_.getPressureLevel(current_memory_bytes_);
    if (pressure_level >= MemoryPolicy::PressureLevel::ELEVATED) {
        MemoryPolicy::MemoryPressureEvent event{
            pressure_level,
            current_memory_bytes_,
            policy_.getMaxResultBytes(),
            policy_.getUtilizationPercent(current_memory_bytes_),
            shard_id,
            "Added batch, now under pressure"};
        policy_.recordPressureEvent(event);
    }

    spdlog::debug(
        "Added result batch: shard={}, size={}B, total_memory={}B, "
        "utilization={}%",
        shard_id,
        size_bytes,
        current_memory_bytes_,
        policy_.getUtilizationPercent(current_memory_bytes_));

    return true;
}

std::vector<nlohmann::json> ResultAccumulator::getResults(
    const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<nlohmann::json> results;
    auto it = shard_batches_.find(shard_id);
    if (it != shard_batches_.end()) {
        for (const auto& batch : it->second) {
            results.push_back(batch.data);
        }
    }
    return results;
}

std::unordered_map<std::string, std::vector<nlohmann::json>>
ResultAccumulator::getAllResults() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::unordered_map<std::string, std::vector<nlohmann::json>> all_results;
    for (const auto& [shard_id, batches] : shard_batches_) {
        for (const auto& batch : batches) {
            all_results[shard_id].push_back(batch.data);
        }
    }
    return all_results;
}

nlohmann::json ResultAccumulator::getMergedResults() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json merged = nlohmann::json::array();
    for (const auto& [shard_id, batches] : shard_batches_) {
        for (const auto& batch : batches) {
            if (batch.data.is_array()) {
                for (const auto& item : batch.data) {
                    merged.push_back(item);
                }
            } else {
                merged.push_back(batch.data);
            }
        }
    }
    return merged;
}

uint64_t ResultAccumulator::getCurrentMemoryBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_memory_bytes_;
}

double ResultAccumulator::getMemoryUtilizationPercent() const {
    return policy_.getUtilizationPercent(getCurrentMemoryBytes());
}

MemoryPolicy::PressureLevel ResultAccumulator::getPressureLevel() const {
    return policy_.getPressureLevel(getCurrentMemoryBytes());
}

bool ResultAccumulator::isUnderPressure() const {
    return policy_.isUnderPressure(getCurrentMemoryBytes());
}

size_t ResultAccumulator::getResultCount(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = shard_batches_.find(shard_id);
    return static_cast<bool>(it != shard_batches_.end() ? it- < static_cast<int>(second.size())) : 0;
}

size_t ResultAccumulator::getTotalResultCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [shard_id, batches] : shard_batches_) {
        total += batches.size();
    }
    return total;
}

void ResultAccumulator::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_batches_.clear();
    current_memory_bytes_ = 0;
    spdlog::debug("ResultAccumulator cleared");
}

std::string ResultAccumulator::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string stats = "ResultAccumulator Statistics:\n";
    stats += "  current_memory: " + std::to_string(current_memory_bytes_) + "B\n";
    stats += "  max_memory: " + std::to_string(policy_.getMaxResultBytes()) + "B\n";
    stats += "  utilization: " +
             std::to_string(policy_.getUtilizationPercent(current_memory_bytes_)) +
             "%\n";
    stats += "  total_batches: " + std::to_string(total_batch_count_) + "\n";
    stats += "  shards:\n";
    for (const auto& [shard_id, batches] : shard_batches_) {
        stats += "    - " + shard_id + ": " + std::to_string(batches.size()) +
                 " batches\n";
    }
    return stats;
}

std::vector<MemoryPolicy::MemoryPressureEvent>
ResultAccumulator::getPressureEvents() const {
    return policy_.getPressureEvents();
}

bool ResultAccumulator::isMemoryLimitExceeded() const {
    return policy_.isLimitExceeded(getCurrentMemoryBytes());
}

uint64_t ResultAccumulator::estimateJsonSize(const nlohmann::json& json) const {
    // Simple estimate: use the serialized string size
    // In production, could use more sophisticated sizing
    if (json.is_null()) {
        return 0;
    }
    return json.dump().size();
}

void ResultAccumulator::handleMemoryPressure([[maybe_unused]] uint64_t needed_bytes) {
    std::string reason = "Adding batch of " + std::to_string(needed_bytes) +
                        " bytes would exceed limit";

    switch (policy_.getOverflowPolicy()) {
        case MemoryPolicy::OverflowPolicy::REJECT: {
            MemoryPolicy::MemoryPressureEvent event{
                MemoryPolicy::PressureLevel::CRITICAL,
                current_memory_bytes_,
                policy_.getMaxResultBytes(),
                policy_.getUtilizationPercent(current_memory_bytes_),
                "",
                "REJECT policy triggered: " + reason};
            policy_.recordPressureEvent(event);
            break;
        }

        case MemoryPolicy::OverflowPolicy::DROP_OLDEST: {
            spdlog::info("Dropping oldest batch due to memory pressure");
            dropOldestBatch();
            break;
        }

        case MemoryPolicy::OverflowPolicy::TRUNCATE: {
            spdlog::info("Truncating results due to memory pressure");
            truncateResults();
            break;
        }
    }
}

void ResultAccumulator::dropOldestBatch() {
    if (shard_batches_.empty()) {
        return;
    }

    // Find shard with oldest batch
    std::string oldest_shard = {};
    size_t oldest_index = 0;
    auto oldest_time = std::chrono::steady_clock::now();

    for (auto& [shard_id, batches] : shard_batches_) {
        if (!batches.empty() && batches.front().timestamp < oldest_time) {
            oldest_time = batches.front().timestamp;
            oldest_shard = shard_id;
            oldest_index = 0;
        }
    }

    if (!oldest_shard.empty()) {
        auto& batches = shard_batches_[oldest_shard];
        if (!batches.empty()) {
            const auto& batch = batches.front();
            current_memory_bytes_ -= batch.size_bytes;
            batches.pop_front();

            spdlog::debug(
                "Dropped oldest batch from shard {}: freed {} bytes",
                oldest_shard,
                batch.size_bytes);
        }
    }
}

void ResultAccumulator::truncateResults() {
    // Keep only the first batch from each shard
    uint64_t freed = 0;

    for (auto& [shard_id, batches] : shard_batches_) {
        while (static_cast<int>(batches.size()) > 1) {
            const auto& batch = batches.back();
            freed += batch.size_bytes;
            current_memory_bytes_ -= batch.size_bytes;
            batches.pop_back();
        }
    }

    spdlog::debug("Truncated results: freed {} bytes", freed);
}

} // namespace themis::query
