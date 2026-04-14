/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_result_store.h                                ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:25:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f79e072b99  2026-02-23  feat(scheduler): implement scheduled task output persiste... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file task_result_store.h
 * @brief Persistent storage for scheduled task execution results in ThemisDB.
 *
 * Stores task output records in RocksDB under the key prefix
 * `_sched_result/<task_id>/<timestamp_ms_20digits>` so that results are
 * naturally ordered by task and time, enabling prefix-scan queries.
 *
 * Features:
 * - Append execution results (success or failure) after every task run.
 * - Query the N most-recent results for a given task.
 * - Automatic retention: trims oldest records when the per-task cap is exceeded.
 * - Thread-safe: all public methods are guarded by an internal mutex.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declaration – avoids pulling in the full RocksDB header here.
class RocksDBWrapper;

namespace scheduler {

/**
 * @brief A single recorded execution of a scheduled task.
 */
struct TaskExecutionResult {
    std::string task_id;      ///< Task identifier
    std::string task_name;    ///< Human-readable task name
    int64_t     timestamp_ms; ///< Wall-clock start time (ms since Unix epoch)
    double      duration_ms;  ///< Execution duration in milliseconds
    bool        success;      ///< true if the task completed without error
    nlohmann::json output;    ///< JSON result returned by the task (empty on failure)
    std::string error;        ///< Error message (empty on success)

    /// Serialize to JSON (for storage / API responses).
    nlohmann::json toJson() const;

    /// Deserialize from JSON (used when loading from RocksDB).
    static TaskExecutionResult fromJson(const nlohmann::json& j);
};

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
     * Stores the result and, if the number of stored results for this task
     * exceeds `max_per_task`, deletes the oldest entries.
     *
     * @param result  Execution record to persist.
     */
    void store(const TaskExecutionResult& result);

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
