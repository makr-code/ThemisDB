/**
 * @file retention.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <vector>
#include <functional>

namespace rocksdb { class TransactionDB; class ColumnFamilyHandle; }

namespace themis {

class TSStore;

struct RetentionPolicy {
    // Retention per metric in seconds (0 or missing means ignore)
    std::unordered_map<std::string, std::chrono::seconds> per_metric;
};

/**
 * @brief Staged deletion policy for graduated data retirement
 *
 * Instead of immediate hard deletion, data passes through stages:
 *   1. Mark  → flagged as expired (still readable)
 *   2. Soft  → moved to cold storage / compressed (still recoverable)
 *   3. Hard  → permanently deleted
 *
 * Each stage is optional and has a configurable age threshold.
 */
struct StagedDeletionPolicy {
    std::chrono::seconds mark_after{0};       ///< Mark expired after this age (0 = skip)
    std::chrono::seconds soft_delete_after{0};///< Soft-delete after this age (0 = skip)
    std::chrono::seconds hard_delete_after{0};///< Hard-delete after this age (required > 0)
};

/**
 * @brief Compliance audit log entry
 */
struct RetentionAuditEntry {
    int64_t timestamp_ms;     ///< When the action occurred
    std::string metric;       ///< Affected metric (empty = global)
    std::string action;       ///< "apply", "async_cycle", "staged_mark", "staged_soft", "hard_delete"
    size_t records_affected;  ///< Number of data points affected
    std::string reason;       ///< Reason/policy description
};

/**
 * @brief Retention statistics
 */
struct RetentionStats {
    std::atomic<uint64_t> total_deleted{0};          ///< Total data points deleted
    std::atomic<uint64_t> apply_count{0};             ///< Times apply() ran
    std::atomic<uint64_t> async_cycle_count{0};       ///< Async background cycles run
    std::atomic<uint64_t> total_space_reclaimed_est{0};///< Estimated bytes freed

    RetentionStats() = default;
    RetentionStats(const RetentionStats& o)
        : total_deleted(o.total_deleted.load())
        , apply_count(o.apply_count.load())
        , async_cycle_count(o.async_cycle_count.load())
        , total_space_reclaimed_est(o.total_space_reclaimed_est.load()) {}
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

    // ========== Staged Deletion ==========

    /**
     * Set staged deletion policy. When set, apply() performs graduated deletion.
     */
    void setStagedDeletion(const StagedDeletionPolicy& staged) {
        std::lock_guard<std::mutex> lock(mutex_);
        staged_policy_ = staged;
        use_staged_deletion_ = true;
    }

    /// Returns true if staged deletion is configured
    bool hasStagedDeletion() const { return use_staged_deletion_; }

    // ========== Compliance Logging ==========

    /**
     * Register a compliance audit callback.
     * Called synchronously after each retention action.
     */
    void setAuditCallback(std::function<void(const RetentionAuditEntry&)> cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        audit_callback_ = std::move(cb);
    }

    /**
     * Returns the compliance audit log (last N entries kept in memory).
     */
    std::vector<RetentionAuditEntry> getAuditLog() const {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        return audit_log_;
    }

    /// Clear the in-memory audit log
    void clearAuditLog() {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        audit_log_.clear();
    }

private:
    TSStore* store_;
    RetentionPolicy policy_;
    mutable std::mutex mutex_;

    // Staged deletion
    StagedDeletionPolicy staged_policy_;
    bool use_staged_deletion_{false};

    // Compliance audit
    mutable std::mutex audit_mutex_;
    std::vector<RetentionAuditEntry> audit_log_;
    std::function<void(const RetentionAuditEntry&)> audit_callback_;
    static constexpr size_t MAX_AUDIT_LOG_SIZE = 10000;

    // Async background thread
    std::atomic<bool> async_running_{false};
    std::thread async_thread_;
    std::condition_variable async_cv_;
    std::mutex async_mutex_;
    std::chrono::seconds async_interval_{3600};

    RetentionStats stats_;

    void asyncLoop();
    void logAudit(const RetentionAuditEntry& entry);
};

} // namespace themis
