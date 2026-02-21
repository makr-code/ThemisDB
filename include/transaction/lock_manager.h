/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lock_manager.h                                     ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 14:07:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
};

} // namespace themis
