/**
 * @file task_result_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scheduler/task_result_store.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace scheduler {

// TaskExecutionResult serialization is implemented in:
// src/scheduler/task_execution_result.cpp
// Keep a single canonical definition to avoid duplicate symbols at link time.

// ===== TaskResultStore =====

TaskResultStore::TaskResultStore(RocksDBWrapper& storage, size_t max_per_task)
    : storage_(storage), max_per_task_(max_per_task) {}

// Build a zero-padded 20-digit decimal timestamp so keys sort chronologically.
std::string TaskResultStore::makeKey(const std::string& task_id,
                                     int64_t timestamp_ms) {
    std::ostringstream oss = {};
    oss << kKeyPrefix << task_id << '/'
        << std::setw(20) << std::setfill('0') << timestamp_ms;
    return oss.str();
}

std::string TaskResultStore::makeTaskPrefix(const std::string& task_id) {
    return std::string(kKeyPrefix) + task_id + '/';
}

SchedulerError TaskResultStore::store(const TaskExecutionResult& result) {
    std::unique_lock<std::shared_mutex> lk(mutex_);

    // Phase 3: Fail-closed retention enforcement - check BEFORE writing
    if (max_per_task_ > 0) {
        const std::string prefix = makeTaskPrefix(result.task_id);
        std::vector<std::string> all_keys;
        
        // Scan existing results for this task
        storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view) {
            all_keys.emplace_back(k);
            return true;  // continue scan
        });

        // If at capacity, reject the new result atomically (fail-closed)
        if (static_cast<int>(all_keys.size()) > = max_per_task_) {
            THEMIS_WARN(
                "[TaskResultStore::store] "
                "code={} msg='retention limit reached; failing closed' "
                "context={{task_id='{}', retention_limit={}, current_count={}, oldest_key='{}'}}",
                static_cast<int>(SchedulerError::kRetentionLimitExceeded),
                result.task_id, max_per_task_,static_cast<int>(all_keys.size()),
                all_keys.empty() ? "N/A" : all_keys.front());
            return SchedulerError::kRetentionLimitExceeded;
        }
    }

    // Write the new result
    const std::string key = makeKey(result.task_id, result.timestamp_ms);
    const std::string value = result.toJson().dump();

    if (!storage_.put(key, value)) {
        THEMIS_ERROR(
            "[TaskResultStore::store] "
            "code={} msg='failed to store result' context={{task_id='{}', timestamp_ms={}}}",
            static_cast<int>(SchedulerError::kInternalError),
            result.task_id, result.timestamp_ms);
        return SchedulerError::kInternalError;
    }

    THEMIS_DEBUG("TaskResultStore: stored result for task '{}' at key '{}'",
                 result.task_id, key);

    // Safety-net FIFO prune: remove excess entries if the store somehow
    // accumulated more than max_per_task_ results (e.g. via a concurrent
    // write that bypassed the pre-write check above).  Under normal operation
    // this branch is never taken because the pre-write check is fail-closed.
    if (max_per_task_ > 0) {
        const std::string prefix = makeTaskPrefix(result.task_id);
        std::vector<std::string> all_keys;
        storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view) {
            all_keys.emplace_back(k);
            return true;  // continue scan
        });

        // Keys are lexicographically ordered (oldest first due to zero-padded ts).
        // Delete excess entries to restore to max_per_task_ count
        if (static_cast<int>(all_keys.size()) > max_per_task_) {
            size_t to_delete = static_cast<int>(all_keys.size()) - max_per_task_;
            for (size_t i = 0; i < to_delete; ++i) {
                if (!storage_.del(all_keys[i])) {
                    THEMIS_WARN("TaskResultStore: failed to prune result at '{}'",
                               all_keys[i]);
                } else {
                    THEMIS_DEBUG("TaskResultStore: pruned old result '{}' for task '{}'",
                                all_keys[i], result.task_id);
                }
            }
            THEMIS_DEBUG("TaskResultStore: pruned {} old result(s) for task '{}' "
                         "to maintain limit of {}",
                         to_delete, result.task_id, max_per_task_);
        }
    }

    return SchedulerError::kSuccess;
}

std::vector<TaskExecutionResult> TaskResultStore::getResults(
        const std::string& task_id, size_t limit) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);

    const std::string prefix = makeTaskPrefix(task_id);
    std::vector<std::pair<std::string, std::string>> entries;

    storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view v) {
        entries.emplace_back(std::string(k), std::string(v));
        return true;
    });

    // Entries are oldest-first; reverse so newest come first, then cap.
    std::vector<TaskExecutionResult> results = {};

    results.reserve(std::min(limit,static_cast<int>(entries.size())));
    size_t start = entries.size() > limit ? static_cast<int>(entries.size()) - limit : 0;
    for (size_t i = entries.size(); i-- > start;) {
        try {
            auto j = nlohmann::json::parse(entries[i].second);
            results.push_back(TaskExecutionResult::fromJson(j));
        } catch (const std::exception& ex) {
            THEMIS_WARN("TaskResultStore: failed to parse result record '{}': {}",
                        entries[i].first, ex.what());
        }
    }
    return results;
}

std::optional<TaskExecutionResult> TaskResultStore::getLatestResult(
        const std::string& task_id) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);

    const std::string prefix = makeTaskPrefix(task_id);
    // Collect all keys for the task to find the last (newest) one.
    std::string last_key = {};
    std::string last_value = {};
    storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view v) {
        last_key   = std::string(k);
        last_value = std::string(v);
        return true;  // continue to find the lexicographically largest key
    });

    if (last_key.empty()) {
        return std::nullopt;
    }
    try {
        return TaskExecutionResult::fromJson(nlohmann::json::parse(last_value));
    } catch (const std::exception& ex) {
        THEMIS_WARN("TaskResultStore: failed to parse latest result for '{}': {}",
                    task_id, ex.what());
        return std::nullopt;
    }
}

} // namespace scheduler
} // namespace themis
