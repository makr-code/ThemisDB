/**
 * @file lock_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.45
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <functional>

namespace themis {

/// Lock types for multi-granularity locking
enum class LockType {
    SHARED,           ///< Read lock – many transactions can hold simultaneously
    EXCLUSIVE,        ///< Write lock – only one holder allowed
    INTENT_SHARED,    ///< Signals intent to acquire SHARED lock on a sub-resource
    INTENT_EXCLUSIVE  ///< Signals intent to acquire EXCLUSIVE lock on a sub-resource
};

/// Two-Phase Locking phase for a transaction
enum class TwoPLPhase {
    GROWING,  ///< Transaction is in growing phase – may acquire, must not release
    SHRINKING ///< Transaction is in shrinking phase – may release, must not acquire
};

/// Outcome of a lock-acquire request
enum class LockStatus {
    GRANTED,  ///< Lock granted immediately
    TIMEOUT,  ///< Lock wait exceeded timeout
    DENIED    ///< Lock denied (2PL violation, or incompatible held by same txn)
};

/// LockManager – row-level and table-level locking for ACID transactions.
///
/// Supports Read/Write/Intent-Shared/Intent-Exclusive lock modes, configurable
/// wait timeout, lock escalation (row → table), and strict Two-Phase Locking.
///
/// Thread-Safety: all public methods are thread-safe.
class LockManager {
public:
    using TransactionId = uint64_t;

    static constexpr std::chrono::milliseconds DEFAULT_LOCK_TIMEOUT{5000};
    static constexpr size_t DEFAULT_ESCALATION_THRESHOLD = 100;

    struct LockResult {
        LockStatus status{LockStatus::GRANTED};
        std::string message;

        static LockResult Granted() { return {LockStatus::GRANTED, ""}; }
        static LockResult Timeout() { return {LockStatus::TIMEOUT, "lock wait timeout"}; }
        static LockResult Denied(std::string msg) { return {LockStatus::DENIED, std::move(msg)}; }
    };

    struct LockStats {
        uint64_t total_acquired{0};
        uint64_t total_released{0};
        uint64_t total_timeouts{0};
        uint64_t total_escalations{0};
        uint64_t current_held{0};
        uint64_t current_waiting{0};
    };

    LockManager();
    ~LockManager() = default;

    LockManager(const LockManager&) = delete;
    LockManager& operator=(const LockManager&) = delete;

    /// Acquire a lock on @p key for transaction @p txn_id.
    /// Blocks (up to @p timeout) if an incompatible lock is held.
    LockResult acquireLock(
        TransactionId txn_id,
        const std::string& key,
        LockType type,
        std::chrono::milliseconds timeout = DEFAULT_LOCK_TIMEOUT);

    /// Release the lock held by @p txn_id on @p key.
    bool releaseLock(TransactionId txn_id, const std::string& key);

    /// Release every lock held by @p txn_id.
    void releaseAllLocks(TransactionId txn_id);

    /// Upgrade an existing SHARED lock to EXCLUSIVE.
    LockResult upgradeLock(
        TransactionId txn_id,
        const std::string& key,
        std::chrono::milliseconds timeout = DEFAULT_LOCK_TIMEOUT);

    /// Check whether @p txn_id currently holds at least @p type on @p key.
    bool holdsLock(TransactionId txn_id, const std::string& key, LockType type) const;

    /// Return all (key, LockType) pairs currently held by @p txn_id.
    std::vector<std::pair<std::string, LockType>> getLocksHeld(TransactionId txn_id) const;

    /// Transition @p txn_id into the shrinking phase (no new acquisitions allowed).
    void beginShrinkingPhase(TransactionId txn_id);

    /// True if @p txn_id is in the shrinking phase.
    bool isInShrinkingPhase(TransactionId txn_id) const;

    /// Set the row-lock count threshold that triggers table-lock escalation.
    void setEscalationThreshold(size_t threshold);

    /// Set the default lock-wait timeout used when none is supplied.
    void setDefaultTimeout(std::chrono::milliseconds timeout);

    /// Aggregate statistics snapshot.
    LockStats getStats() const;

    /// Transactions currently waiting on @p key (used by deadlock detector).
    std::vector<TransactionId> getWaiters(const std::string& key) const;

    /// Keys that @p txn_id is currently blocked on (used by deadlock detector).
    std::vector<std::string> getWaitingFor(TransactionId txn_id) const;

    // ── Predicate Locking for SSI (Serializable Snapshot Isolation) ──────────

    /// Acquire a predicate (range) lock for SERIALIZABLE isolation.
    ///
    /// Records that @p txn_id has read all keys in [@p start_key, @p end_key]
    /// (inclusive on both ends). Any other transaction that subsequently writes
    /// a key in this range will be detected as a serialization conflict when
    /// it calls checkPredicateConflict().
    ///
    /// @param txn_id     Owning transaction.
    /// @param start_key  Lower bound of the predicate range (inclusive).
    /// @param end_key    Upper bound of the predicate range (inclusive; may
    ///                   equal @p start_key for a single-key predicate).
    /// @return true when the lock was recorded; false when the global limit
    ///         set by setMaxPredicateLocks() has been reached (the lock is
    ///         silently dropped in that case – the false-positive abort rate
    ///         may increase but correctness is preserved).
    bool acquirePredicateLock(TransactionId txn_id,
                              const std::string& start_key,
                              const std::string& end_key);

    /// Set the maximum total number of predicate locks that may be held
    /// simultaneously across all active transactions.
    ///
    /// Once the limit is reached, acquirePredicateLock() returns false and
    /// does not record the lock.  Pass 0 to disable the limit (default).
    ///
    /// Thread-safe.
    void setMaxPredicateLocks(size_t max_locks);

    /// Return the current maximum predicate-lock limit (0 = unlimited).
    size_t getMaxPredicateLocks() const;

    /// Enable or disable predicate-lock tracking globally.
    ///
    /// When disabled, acquirePredicateLock() is a no-op (returns false) and
    /// checkPredicateConflict() always returns 0.
    ///
    /// Thread-safe.
    void setPredicateLockingEnabled(bool enabled);

    /// Return whether predicate-lock tracking is currently enabled.
    bool isPredicateLockingEnabled() const;

    /// Release all predicate locks held by @p txn_id.
    ///
    /// Must be called when a SERIALIZABLE transaction commits or rolls back.
    void releasePredicateLocks(TransactionId txn_id);

    /// Check whether writing @p key by @p writing_txn_id conflicts with a
    /// predicate lock held by another active transaction.
    ///
    /// Returns the TransactionId of the first conflicting holder, or 0 if
    /// no conflict exists.
    TransactionId checkPredicateConflict(TransactionId writing_txn_id,
                                         const std::string& key) const;

    /// Return the number of predicate locks currently held by @p txn_id.
    size_t getPredicateLockCount(TransactionId txn_id) const;

    /// Return all predicate-lock ranges held by @p txn_id as (start_key, end_key) pairs.
    ///
    /// The returned vector is a snapshot copy; the caller may iterate it without
    /// holding any lock.  An empty vector is returned when txn_id has no
    /// predicate locks or when predicate locking is disabled.
    std::vector<std::pair<std::string, std::string>> getPredicateLockRanges(
        TransactionId txn_id) const;

private:
    /// One active lock holder for a key.
    struct LockEntry {
        TransactionId holder;
        LockType      type;
        std::chrono::system_clock::time_point acquired_at;
    };

    /// One pending lock request for a key.
    struct LockRequest {
        TransactionId txn_id;
        LockType      type;
        bool          granted{false};
        // Each waiter has its own CV so it can be woken individually.
        std::condition_variable cv;

        LockRequest(TransactionId t, LockType lt) : txn_id(t), type(lt) {}
    };

    /// Per-key state in the lock table.
    struct LockTableEntry {
        std::vector<LockEntry>                          holders;
        std::list<std::shared_ptr<LockRequest>>         waiters;
    };

    /// Compatibility matrix: returns true when @p requested can be granted
    /// alongside an already-held lock of type @p held.
    static bool compatible(LockType held, LockType requested) noexcept;

    /// Try to immediately grant the lock; returns true on success.
    /// Precondition: mutex_ is held.
    bool tryGrantLock(const std::string& key, TransactionId txn_id, LockType type);

    /// Wake all waiters for @p key and try to grant their requests.
    /// Precondition: mutex_ is held.
    void processWaiters(const std::string& key);

    /// Escalate row-level locks to a table lock when the threshold is exceeded.
    /// Precondition: mutex_ is held.
    void checkEscalation(TransactionId txn_id, const std::string& key);

    mutable std::mutex mutex_;

    std::unordered_map<std::string, LockTableEntry> lock_table_;

    /// Per-transaction: key → LockType held.
    std::unordered_map<TransactionId,
                       std::unordered_map<std::string, LockType>> held_by_txn_;

    /// Transactions currently in the shrinking phase.
    std::unordered_set<TransactionId> shrinking_txns_;

    /// For deadlock detection: txn_id → key it is waiting on.
    std::unordered_map<TransactionId, std::string> waiting_for_;

    std::atomic<size_t>   escalation_threshold_{DEFAULT_ESCALATION_THRESHOLD};
    std::atomic<uint64_t> default_timeout_ms_{5000};

    // Statistics
    std::atomic<uint64_t> stats_acquired_{0};
    std::atomic<uint64_t> stats_released_{0};
    std::atomic<uint64_t> stats_timeouts_{0};
    std::atomic<uint64_t> stats_escalations_{0};
    std::atomic<uint64_t> stats_waiting_{0};
    std::atomic<uint64_t> stats_deadlocks_{0};

    // ── Predicate locks for SSI ───────────────────────────────────────────────

    /// A predicate (range) lock for Serializable Snapshot Isolation.
    struct PredicateLock {
        TransactionId txn_id;
        std::string   start_key; ///< lower bound (inclusive)
        std::string   end_key;   ///< upper bound (inclusive)
    };

    /// All active predicate locks, protected by mutex_.
    std::vector<PredicateLock> predicate_locks_;

    /// Maximum total predicate locks allowed (0 = unlimited), protected by mutex_.
    std::atomic<size_t> max_predicate_locks_{0};

    /// Whether predicate-lock tracking is enabled (default: true).
    std::atomic<bool> predicate_locking_enabled_{true};

    /// Counter: number of predicate locks dropped due to max_locks capacity (Wave 4C T4).
    std::atomic<uint64_t> predicate_lock_drops_{0};

public:
    /// Return the cumulative count of predicate locks dropped due to capacity limits.
    /// Non-zero values indicate SSI false-abort rate may have increased; tune
    /// setMaxPredicateLocks() or increase capacity.
    uint64_t predicateLockDropCount() const noexcept {
        return predicate_lock_drops_.load(std::memory_order_relaxed);
    }

private:

}; // class LockManager

} // namespace themis
