#include "timeseries/retention.h"
#include "timeseries/tsstore.h"
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/iterator.h>

namespace themis {

// (removed) list_metrics helper was unused; removed to avoid C4505 warning

size_t RetentionManager::apply() {
    if (!store_) return 0;
    // Extract DB internals from store
    // We rely on TSStore API: we will scan metrics and call deleteOldData
    size_t total_deleted = 0;
    // TSStore does not expose db handle; we scan via deleteOldData per metric by iterating entities
    // Simpler: iterate all keys and delete if older than threshold per metric using deleteOldData
    // But TSStore::deleteOldData deletes globally across all metrics before timestamp.
    // We'll call deleteOldData for each metric threshold.

    // Build now timestamp
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [metric, retention] : policy_.per_metric) {
        int64_t cutoff = now_ms - std::chrono::duration_cast<std::chrono::milliseconds>(retention).count();
        total_deleted += store_->deleteOldDataForMetric(metric, cutoff);
    }

    stats_.total_deleted.fetch_add(total_deleted);
    stats_.apply_count.fetch_add(1);
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
