#ifndef THEMIS_RETENTION_H
#define THEMIS_RETENTION_H

#include <string>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>

namespace rocksdb { class TransactionDB; class ColumnFamilyHandle; }

namespace themis {

class TSStore;

struct RetentionPolicy {
    // Retention per metric in seconds (0 or missing means ignore)
    std::unordered_map<std::string, std::chrono::seconds> per_metric;
};

/**
 * @brief Retention statistics
 */
struct RetentionStats {
    std::atomic<uint64_t> total_deleted{0};     ///< Total data points deleted
    std::atomic<uint64_t> apply_count{0};        ///< Times apply() ran
    std::atomic<uint64_t> async_cycle_count{0};  ///< Async background cycles run

    RetentionStats() = default;
    RetentionStats(const RetentionStats& o)
        : total_deleted(o.total_deleted.load())
        , apply_count(o.apply_count.load())
        , async_cycle_count(o.async_cycle_count.load()) {}
};

class RetentionManager {
public:
    RetentionManager(TSStore* store, RetentionPolicy policy)
        : store_(store), policy_(std::move(policy)) {}

    ~RetentionManager() { stopAsync(); }

    // Non-copyable (has threads)
    RetentionManager(const RetentionManager&) = delete;
    RetentionManager& operator=(const RetentionManager&) = delete;

    // Apply retention for now() – synchronous
    size_t apply();

    /**
     * Start background async retention cleanup.
     * @param interval  How often to run cleanup
     */
    void startAsync(std::chrono::seconds interval = std::chrono::hours(1));

    /**
     * Stop background async retention cleanup.
     */
    void stopAsync();

    /// True if background cleanup is running
    bool isAsyncRunning() const { return async_running_.load(); }

    /// Get retention statistics
    RetentionStats getStats() const { return stats_; }

    /// Update policy (takes effect on next apply())
    void setPolicy(RetentionPolicy policy) {
        std::lock_guard<std::mutex> lock(mutex_);
        policy_ = std::move(policy);
    }

    const RetentionPolicy& getPolicy() const { return policy_; }

private:
    TSStore* store_;
    RetentionPolicy policy_;
    mutable std::mutex mutex_;

    // Async background thread
    std::atomic<bool> async_running_{false};
    std::thread async_thread_;
    std::condition_variable async_cv_;
    std::mutex async_mutex_;
    std::chrono::seconds async_interval_{3600};

    RetentionStats stats_;

    void asyncLoop();
};

} // namespace themis

#endif // THEMIS_RETENTION_H
