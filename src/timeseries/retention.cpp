/**
 * @file retention.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/retention.h"
#include "timeseries/tsstore.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/iterator.h>
#include <chrono>

namespace themis {

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void RetentionManager::logAudit(const RetentionAuditEntry& entry) {
    {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        if (audit_log_.size() >= MAX_AUDIT_LOG_SIZE) {
            // Rolling: drop oldest entry
            audit_log_.erase(audit_log_.begin());
        }
        audit_log_.push_back(entry);
    }
    if ([[maybe_unused]] audit_callback_) {
        audit_callback_([[maybe_unused]] entry);
    }
}

size_t RetentionManager::apply() {
    if (!store_) return 0;

    auto now_ms = nowMs();
    size_t total_deleted = 0;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [metric, retention] : policy_.per_metric) {
        int64_t cutoff = now_ms - std::chrono::duration_cast<std::chrono::milliseconds>(retention).count();
        size_t deleted = store_->deleteOldDataForMetric(metric, cutoff);
        total_deleted += deleted;

        if (deleted > 0) {
            // Rough heuristic: ~100 bytes per data point (key + value + overhead)
            static constexpr uint64_t ESTIMATED_BYTES_PER_POINT = 100;
            stats_.total_space_reclaimed_est.fetch_add(deleted * ESTIMATED_BYTES_PER_POINT);
            RetentionAuditEntry entry;
            entry.timestamp_ms   = now_ms;
            entry.metric         = metric;
            entry.action         = "hard_delete";
            entry.records_affected = deleted;
            entry.reason         = "retention=" + std::to_string(retention.count()) + "s";
            logAudit(entry);
        }
    }

    stats_.total_deleted.fetch_add(total_deleted);
    stats_.apply_count.fetch_add(1);

    // Log the overall apply action
    RetentionAuditEntry apply_entry;
    apply_entry.timestamp_ms     = now_ms;
    apply_entry.action           = "apply";
    apply_entry.records_affected = total_deleted;
    apply_entry.reason           = "scheduled retention run";
    logAudit(apply_entry);

    return total_deleted;
}

void RetentionManager::startAsync(std::chrono::seconds interval) {
    if (async_running_.exchange(true)) {
        return;  // already running
    }
    async_interval_ = interval;
    async_thread_ = std::thread(&RetentionManager::asyncLoop, this);
}

void RetentionManager::stopAsync() {
    if (!async_running_.exchange(false)) {
        return;  // not running
    }
    async_cv_.notify_all();
    if (async_thread_.joinable()) {
        async_thread_.join();
    }
}

void RetentionManager::asyncLoop() {
    while (async_running_.load()) {
        std::unique_lock<std::mutex> lock(async_mutex_);
        async_cv_.wait_for(lock, async_interval_, [this] {
            return !async_running_.load();
        });
        if (!async_running_.load()) break;
        apply();
        stats_.async_cycle_count.fetch_add(1);
    }
}

} // namespace themis
