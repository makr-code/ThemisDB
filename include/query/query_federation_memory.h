/**
 * @file query_federation_memory.h
 * @brief Bounded memory accumulation for federated query results
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 94/100
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

class MemoryPolicy {
public:
    enum class OverflowPolicy {
        REJECT,        // Reject new results and fail
        DROP_OLDEST,   // Drop oldest batches from each shard
        TRUNCATE,      // Return top-N results and truncate
    };

    enum class PressureLevel {
        NORMAL,        // < 70% of limit
        ELEVATED,      // 70-85% of limit
        HIGH,          // 85-95% of limit
        CRITICAL,      // > 95% of limit
    };

    struct MemoryPressureEvent {
        PressureLevel level;
        uint64_t current_bytes;
        uint64_t max_bytes;
        double utilization_percent;
        std::string shard_id;
        std::string details;
    };

    struct ResultBatch {
        size_t batch_number = 0;
        uint64_t size_bytes;
        std::string shard_id;
        std::chrono::steady_clock::time_point timestamp;
        nlohmann::json data;
    };

    class Builder {
    public:
        /**
         * @brief With Max Result Bytes.
         * @param[in] max_bytes Input parameter.
         * @return Return value.
         * @details Implements withMaxResultBytes without additional internal calls.
         */
        Builder& withMaxResultBytes(uint64_t max_bytes) {
            max_result_bytes_ = max_bytes;
            return *this;
        }

        /**
         * @brief With Overflow Policy.
         * @param[in] policy Input parameter.
         * @return Return value.
         * @details Implements withOverflowPolicy without additional internal calls.
         */
        Builder& withOverflowPolicy(OverflowPolicy policy) {
            overflow_policy_ = policy;
            return *this;
        }

        /**
         * @brief With Pressure Thresholds.
         * @param[in] elevated_pct Input parameter.
         * @param[in] high_pct Input parameter.
         * @param[in] critical_pct Input parameter.
         * @return Return value.
         * @details Implements withPressureThresholds without additional internal calls.
         */
        Builder& withPressureThresholds(
            double elevated_pct,
            double high_pct,
            double critical_pct) {
            elevated_threshold_pct_ = elevated_pct;
            high_threshold_pct_ = high_pct;
            critical_threshold_pct_ = critical_pct;
            return *this;
        }

        /**
         * @brief With Max Batches Per Shard.
         * @param[in] max_batches Input parameter.
         * @return Return value.
         * @details Implements withMaxBatchesPerShard without additional internal calls.
         */
        Builder& withMaxBatchesPerShard(size_t max_batches) {
            max_batches_per_shard_ = max_batches;
            return *this;
        }

        /**
         * @brief Build.
         * @return Return value.
         */
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

    [[nodiscard]] uint64_t getMaxResultBytes() const {
        return max_result_bytes_;
    }

    [[nodiscard]] OverflowPolicy getOverflowPolicy() const {
        return overflow_policy_;
    }

    [[nodiscard]] double getUtilizationPercent(uint64_t current_bytes) const {
        if (max_result_bytes_ == 0) {
          return 0.0;
        }
        return 100.0 * static_cast<double>(current_bytes) /
               static_cast<double>(max_result_bytes_);
    }

    [[nodiscard]] PressureLevel getPressureLevel(uint64_t current_bytes) const;

    [[nodiscard]] bool isUnderPressure(uint64_t current_bytes) const;

    [[nodiscard]] bool isLimitExceeded(uint64_t current_bytes) const {
        return current_bytes >= max_result_bytes_;
    }

    /**
     * @brief Record Pressure Event.
     * @param[in] event Input parameter.
     */
    void recordPressureEvent(const MemoryPressureEvent& event) const;

    [[nodiscard]] std::vector<MemoryPressureEvent> getPressureEvents() const;

    /**
     * @brief Clear Events.
     */
    void clearEvents();

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

class ResultAccumulator {
public:
    /**
     * @brief Result Accumulator.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    explicit ResultAccumulator(const MemoryPolicy& policy);

    /**
     * @brief Add Result.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] result Input parameter.
     * @return True when the operation succeeds.
     */
    bool addResult(const std::string& shard_id, const nlohmann::json& result);

    /**
     * @brief Add Result With Size.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] result Input parameter.
     * @param[in] size_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool addResultWithSize(
        const std::string& shard_id,
        const nlohmann::json& result,
        uint64_t size_bytes);

    [[nodiscard]] std::vector<nlohmann::json> getResults(
        const std::string& shard_id) const;

    [[nodiscard]] std::unordered_map<std::string, std::vector<nlohmann::json>>
    getAllResults() const;

    [[nodiscard]] nlohmann::json getMergedResults() const;

    [[nodiscard]] uint64_t getCurrentMemoryBytes() const;

    [[nodiscard]] double getMemoryUtilizationPercent() const;

    [[nodiscard]] MemoryPolicy::PressureLevel getPressureLevel() const;

    [[nodiscard]] bool isUnderPressure() const;

    [[nodiscard]] size_t getResultCount(const std::string& shard_id) const;

    [[nodiscard]] size_t getTotalResultCount() const;

    /**
     * @brief Clear.
     */
    void clear();

    [[nodiscard]] std::string getStatistics() const;

    [[nodiscard]] std::vector<MemoryPolicy::MemoryPressureEvent>
    getPressureEvents() const;

    [[nodiscard]] bool isMemoryLimitExceeded() const;

private:
    const MemoryPolicy& policy_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::deque<MemoryPolicy::ResultBatch>> shard_batches_;
    uint64_t current_memory_bytes_ = 0;
    size_t total_batch_count_ = 0;

    // Helper methods
    [[nodiscard]] uint64_t estimateJsonSize(const nlohmann::json& json) const;
    /**
     * @brief Handle Memory Pressure.
     * @param[in] needed_bytes Input parameter.
     */
    void handleMemoryPressure(uint64_t needed_bytes);
    /**
     * @brief Drop Oldest Batch.
     */
    void dropOldestBatch();
    /**
     * @brief Truncate Results.
     */
    void truncateResults();
};

} // namespace themis::query
