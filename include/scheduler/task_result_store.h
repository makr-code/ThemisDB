/**
 * @file task_result_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "scheduler/scheduler_api_contract.h"

namespace themis {

// Forward declaration – avoids pulling in the full RocksDB header here.
class RocksDBWrapper;

namespace scheduler {

// Use the canonical `TaskExecutionResult` defined by the scheduler API
// contract in `scheduler_api_contract.h` to avoid duplicate definitions.

/**
 * @brief Stores and retrieves scheduled task execution results in ThemisDB.
 *
 * Key layout in RocksDB:
 *   `_sched_result/<task_id>/<timestamp_ms_zero_padded_20>`
 *
 * This ordering lets us scan all results for a task via a prefix scan and
 * retrieve the most-recent N entries with a reverse scan capped at `limit`.
 */
class TaskResultStore {
public:
    /**
     * @brief Construct a result store backed by the supplied RocksDB instance.
     * @param storage         Open RocksDB wrapper.  Must outlive this object.
     * @param max_per_task    Maximum number of results to retain per task.
     *                        Oldest records are pruned when the cap is exceeded.
     */
    explicit TaskResultStore(RocksDBWrapper& storage,
                             size_t max_per_task = 100);

    /**
     * @brief Append an execution result for a task.
     *
     * Enforces retention limits BEFORE writing the new result.
     * If the store is at capacity (max_per_task entries), returns kRetentionLimitExceeded
     * without storing the result.
     *
     * On success, stores the result and performs FIFO pruning to maintain the
     * retention limit (oldest entries deleted first).
     *
     * @param result  Execution record to persist.
     * @return kSuccess on successful storage, kRetentionLimitExceeded if at capacity,
     *         kInternalError on storage failure.
     */
    SchedulerError store(const TaskExecutionResult& result);

    /**
     * @brief Retrieve the most-recent execution results for a task.
     *
     * Results are returned newest-first.
     *
     * @param task_id  Task identifier to query.
     * @param limit    Maximum number of records to return (default: 10).
     * @return Vector of results (may be empty if none are stored).
     */
    std::vector<TaskExecutionResult> getResults(const std::string& task_id,
                                                size_t limit = 10) const;

    /**
     * @brief Return the most-recent execution result for a task, if any.
     *
     * @param task_id  Task identifier.
     * @return Most-recent result, or std::nullopt if no results are stored.
     */
    std::optional<TaskExecutionResult> getLatestResult(
        const std::string& task_id) const;

    /// Key prefix used for all result entries (visible for testing/tooling).
    static constexpr const char* kKeyPrefix = "_sched_result/";

private:
    RocksDBWrapper& storage_;
    size_t          max_per_task_;
    mutable std::shared_mutex mutex_;

    /// Build the full RocksDB key for a result record.
    static std::string makeKey(const std::string& task_id, int64_t timestamp_ms);

    /// Build the prefix used to scan all results for a task.
    static std::string makeTaskPrefix(const std::string& task_id);
};

} // namespace scheduler
} // namespace themis
