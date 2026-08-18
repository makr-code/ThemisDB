/**
 * @file query_federation_memory.h
 * @brief Bounded memory accumulation for federated query results
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 94/100
 * @note Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Beta - Ready for Phase 3 integration
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <deque>
#include <mutex>

namespace themis::query {

/**
 * @brief Bounded memory accumulation policy for federated query results
 *
 * Manages result set accumulation with:
 * - Configurable memory limits (100MB default)
 * - Backpressure detection and handling
 * - Configurable overflow policies (REJECT, DROP_OLDEST, TRUNCATE)
 * - Per-shard memory tracking
 * - Memory pressure events
 *
 * Example:
 * ```cpp
 * auto policy = MemoryPolicy::Builder{}
 *     .withMaxResultBytes(100 * 1024 * 1024)  // 100MB
 *     .withOverflowPolicy(MemoryPolicy::OverflowPolicy::TRUNCATE)
 *     .build();
 *
 * auto accumulator = ResultAccumulator(policy);
 * accumulator.addResult("shard1", result_json);
 * ```
 */
class MemoryPolicy {
public:
    /**
     * @brief Overflow handling strategy when memory limit is reached
     */
    enum class OverflowPolicy {
        REJECT,        // Reject new results and fail
        DROP_OLDEST,   // Drop oldest batches from each shard
        TRUNCATE,      // Return top-N results and truncate
    };

    /**
     * @brief Memory pressure level
     */
    enum class PressureLevel {
        NORMAL,        // < 70% of limit
        ELEVATED,      // 70-85% of limit
        HIGH,          // 85-95% of limit
        CRITICAL,      // > 95% of limit
    };

    /**
     * @brief Memory pressure event
     */
    struct MemoryPressureEvent {
        PressureLevel level;
        uint64_t current_bytes;
        uint64_t max_bytes;
        double utilization_percent;
        std::string shard_id;
        std::string details;
    };

    /**
     * @brief Result batch with metadata
     */
    struct ResultBatch {
        size_t batch_number;
        uint64_t size_bytes;
        std::string shard_id;
        std::chrono::steady_clock::time_point timestamp;
        nlohmann::json data;
    };

    /**
     * @brief Builder for MemoryPolicy
     */
    class Builder {
    public:
        Builder& withMaxResultBytes(uint64_t max_bytes) {
            max_result_bytes_ = max_bytes;
            return *this;
        }

        Builder& withOverflowPolicy(OverflowPolicy policy) {
            overflow_policy_ = policy;
            return *this;
        }

        Builder& withPressureThresholds(
            double elevated_pct,
            double high_pct,
            double critical_pct) {
            elevated_threshold_pct_ = elevated_pct;
            high_threshold_pct_ = high_pct;
            critical_threshold_pct_ = critical_pct;
            return *this;
        }

        Builder& withMaxBatchesPerShard(size_t max_batches) {
            max_batches_per_shard_ = max_batches;
            return *this;
        }

        MemoryPolicy build() const;

    private:
        uint64_t max_result_bytes_ = 100 * 1024 * 1024;  // 100MB default
        OverflowPolicy overflow_policy_ = OverflowPolicy::TRUNCATE;
        double elevated_threshold_pct_ = 70.0;
        double high_threshold_pct_ = 85.0;
        double critical_threshold_pct_ = 95.0;
        size_t max_batches_per_shard_ = 1000;
    };

    // Deleted copy operations
    MemoryPolicy(const MemoryPolicy&) = delete;
    MemoryPolicy& operator=(const MemoryPolicy&) = delete;

    // Default move semantics
    MemoryPolicy(MemoryPolicy&&) noexcept = default;
    MemoryPolicy& operator=(MemoryPolicy&&) noexcept = default;

    /**
     * @brief Get maximum result size in bytes
     */
    [[nodiscard]] uint64_t getMaxResultBytes() const {
        return max_result_bytes_;
    }

    /**
     * @brief Get overflow policy
     */
    [[nodiscard]] OverflowPolicy getOverflowPolicy() const {
        return overflow_policy_;
    }

    /**
     * @brief Get current memory utilization percentage
     * @param current_bytes Current bytes used
     * @return Percentage 0-100
     */
    [[nodiscard]] double getUtilizationPercent(uint64_t current_bytes) const {
        if (max_result_bytes_ == 0) return 0.0;
        return 100.0 * static_cast<double>(current_bytes) /
               static_cast<double>(max_result_bytes_);
    }

    /**
     * @brief Classify memory pressure level
     * @param current_bytes Current bytes used
     * @return Pressure level
     */
    [[nodiscard]] PressureLevel getPressureLevel(uint64_t current_bytes) const;

    /**
     * @brief Check if memory is under pressure
     * @param current_bytes Current bytes used
     * @return true if pressure level >= ELEVATED
     */
    [[nodiscard]] bool isUnderPressure(uint64_t current_bytes) const;

    /**
     * @brief Check if memory limit exceeded
     * @param current_bytes Current bytes used
     * @return true if current >= max
     */
    [[nodiscard]] bool isLimitExceeded(uint64_t current_bytes) const {
        return current_bytes >= max_result_bytes_;
    }

    /**
     * @brief Record a memory pressure event
     * @param event The event to record
     */
    void recordPressureEvent(const MemoryPressureEvent& event) const;

    /**
     * @brief Get recorded pressure events
     * @return Vector of events
     */
    [[nodiscard]] std::vector<MemoryPressureEvent> getPressureEvents() const;

    /**
     * @brief Clear recorded events
     */
    void clearEvents();

    /**
     * @brief Get memory policy summary
     * @return Human-readable summary
     */
    [[nodiscard]] std::string getSummary() const;

    // Destructor
    ~MemoryPolicy() = default;

private:
    friend class Builder;
    friend class ResultAccumulator;

    MemoryPolicy(
        uint64_t max_result_bytes,
        OverflowPolicy overflow_policy,
        double elevated_threshold_pct,
        double high_threshold_pct,
        double critical_threshold_pct,
        size_t max_batches_per_shard);

    uint64_t max_result_bytes_;
    OverflowPolicy overflow_policy_;
    double elevated_threshold_pct_;
    double high_threshold_pct_;
    double critical_threshold_pct_;
    size_t max_batches_per_shard_;
    mutable std::vector<MemoryPressureEvent> pressure_events_;
};

/**
 * @brief Accumulator for federated query results with bounded memory
 *
 * Thread-safe result accumulation with memory tracking and backpressure.
 */
class ResultAccumulator {
public:
    /**
     * @brief Constructor
     * @param policy Memory policy to enforce
     */
    explicit ResultAccumulator(const MemoryPolicy& policy);

    /**
     * @brief Add a result batch from a shard
     * @param shard_id Shard identifier
     * @param result Result data
     * @return true if added, false if rejected due to policy
     * @throws std::runtime_error if REJECT policy triggered
     */
    bool addResult(const std::string& shard_id, const nlohmann::json& result);

    /**
     * @brief Add a result batch with size information
     * @param shard_id Shard identifier
     * @param result Result data
     * @param size_bytes Estimated size in bytes
     * @return true if added, false if rejected
     */
    bool addResultWithSize(
        const std::string& shard_id,
        const nlohmann::json& result,
        uint64_t size_bytes);

    /**
     * @brief Get accumulated results for a shard
     * @param shard_id Shard identifier
     * @return Vector of results
     */
    [[nodiscard]] std::vector<nlohmann::json> getResults(
        const std::string& shard_id) const;

    /**
     * @brief Get all accumulated results
     * @return Map of shard_id -> results vector
     */
    [[nodiscard]] std::unordered_map<std::string, std::vector<nlohmann::json>>
    getAllResults() const;

    /**
     * @brief Get merged results (flattened array)
     * @return Merged results as JSON array
     */
    [[nodiscard]] nlohmann::json getMergedResults() const;

    /**
     * @brief Get current memory usage in bytes
     * @return Current bytes used
     */
    [[nodiscard]] uint64_t getCurrentMemoryBytes() const;

    /**
     * @brief Get memory utilization percentage
     * @return Utilization 0-100
     */
    [[nodiscard]] double getMemoryUtilizationPercent() const;

    /**
     * @brief Get current memory pressure level
     * @return Pressure level
     */
    [[nodiscard]] MemoryPolicy::PressureLevel getPressureLevel() const;

    /**
     * @brief Check if under memory pressure
     * @return true if pressure level >= ELEVATED
     */
    [[nodiscard]] bool isUnderPressure() const;

    /**
     * @brief Get result count for a shard
     * @param shard_id Shard identifier
     * @return Number of accumulated results
     */
    [[nodiscard]] size_t getResultCount(const std::string& shard_id) const;

    /**
     * @brief Get total result count across all shards
     * @return Total count
     */
    [[nodiscard]] size_t getTotalResultCount() const;

    /**
     * @brief Clear all accumulated results
     */
    void clear();

    /**
     * @brief Get accumulator statistics
     * @return Human-readable statistics
     */
    [[nodiscard]] std::string getStatistics() const;

    /**
     * @brief Get recorded memory pressure events
     * @return Vector of events
     */
    [[nodiscard]] std::vector<MemoryPolicy::MemoryPressureEvent>
    getPressureEvents() const;

    /**
     * @brief Check if this accumulator has exceeded memory limit
     * @return true if memory limit exceeded
     */
    [[nodiscard]] bool isMemoryLimitExceeded() const;

private:
    const MemoryPolicy& policy_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<MemoryPolicy::ResultBatch>> shard_batches_;
    uint64_t current_memory_bytes_ = 0;
    size_t total_batch_count_ = 0;

    // Helper methods
    [[nodiscard]] uint64_t estimateJsonSize(const nlohmann::json& json) const;
    void handleMemoryPressure(uint64_t needed_bytes);
    void dropOldestBatch();
    void truncateResults();
};

} // namespace themis::query
