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

/*
 * ThemisDB | File: task_result_store.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 156
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * PR History (last 5): #2651 feat(scheduler): Scheduled ... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "scheduler/task_result_store.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace scheduler {

// ===== TaskExecutionResult serialization =====

nlohmann::json TaskExecutionResult::toJson() const {
    nlohmann::json j;
    j["task_id"]      = task_id;
    j["task_name"]    = task_name;
    j["timestamp_ms"] = timestamp_ms;
    j["duration_ms"]  = duration_ms;
    j["success"]      = success;
    j["output"]       = output;
    j["error"]        = error;
    return j;
}

TaskExecutionResult TaskExecutionResult::fromJson(const nlohmann::json& j) {
    TaskExecutionResult r;
    r.task_id      = j.value("task_id",      std::string{});
    r.task_name    = j.value("task_name",    std::string{});
    r.timestamp_ms = j.value("timestamp_ms", int64_t{0});
    r.duration_ms  = j.value("duration_ms",  0.0);
    r.success      = j.value("success",      false);
    r.output       = j.contains("output") ? j["output"] : nlohmann::json{};
    r.error        = j.value("error",        std::string{});
    return r;
}

// ===== TaskResultStore =====

TaskResultStore::TaskResultStore(RocksDBWrapper& storage, size_t max_per_task)
    : storage_(storage), max_per_task_(max_per_task) {}

// Build a zero-padded 20-digit decimal timestamp so keys sort chronologically.
std::string TaskResultStore::makeKey(const std::string& task_id,
                                     int64_t timestamp_ms) {
    std::ostringstream oss;
    oss << kKeyPrefix << task_id << '/'
        << std::setw(20) << std::setfill('0') << timestamp_ms;
    return oss.str();
}

std::string TaskResultStore::makeTaskPrefix(const std::string& task_id) {
    return std::string(kKeyPrefix) + task_id + '/';
}

SchedulerError TaskResultStore::store(const TaskExecutionResult& result) {
    std::unique_lock<std::shared_mutex> lk(mutex_);

    // Enforce retention BEFORE writing: check if we're at the limit
    if (max_per_task_ > 0) {
        const std::string prefix = makeTaskPrefix(result.task_id);
        std::vector<std::string> all_keys;
        
        // Scan existing results for this task
        storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view) {
            all_keys.emplace_back(k);
            return true;  // continue scan
        });

        // If at capacity, reject the new result atomically
        if (all_keys.size() >= max_per_task_) {
            THEMIS_WARN("TaskResultStore: retention limit {} reached for task '{}'. "
                        "Cannot store new result. Oldest: {}, Current count: {}",
                        max_per_task_, result.task_id,
                        all_keys.empty() ? "N/A" : all_keys.front(),
                        all_keys.size());
            return SchedulerError::kRetentionLimitExceeded;
        }
    }

    // Write the new result
    const std::string key = makeKey(result.task_id, result.timestamp_ms);
    const std::string value = result.toJson().dump();

    if (!storage_.put(key, value)) {
        THEMIS_ERROR("TaskResultStore: failed to store result for task '{}'",
                     result.task_id);
        return SchedulerError::kInternalError;
    }

    THEMIS_DEBUG("TaskResultStore: stored result for task '{}' at key '{}'",
                 result.task_id, key);

    // Prune FIFO-style (oldest first) to stay under the soft limit
    // This allows one overage but prevents unlimited growth
    if (max_per_task_ > 0) {
        const std::string prefix = makeTaskPrefix(result.task_id);
        std::vector<std::string> all_keys;
        storage_.scanPrefix(prefix, [&](std::string_view k, std::string_view) {
            all_keys.emplace_back(k);
            return true;  // continue scan
        });

        // Keys are lexicographically ordered (oldest first due to zero-padded ts).
        // Delete excess entries to restore to max_per_task_ count
        if (all_keys.size() > max_per_task_) {
            size_t to_delete = all_keys.size() - max_per_task_;
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
    std::vector<TaskExecutionResult> results;
    results.reserve(std::min(limit, entries.size()));
    size_t start = entries.size() > limit ? entries.size() - limit : 0;
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
    std::string last_key;
    std::string last_value;
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
