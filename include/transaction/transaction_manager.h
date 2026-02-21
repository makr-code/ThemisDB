/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_manager.h                              ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     417                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 397f3a597  2026-02-21  Refactor header includes and documentation updates across... ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <deque>
#include "storage/rocksdb_wrapper.h"
#include "transaction/lock_manager.h"
#include "transaction/isolation_level.h"

namespace themis {

class BaseEntity;

// Forward declaration for Phase 8 crash recovery
namespace transaction { class CrashRecoveryManager; }
class SecondaryIndexManager;
class GraphIndexManager;
class VectorIndexManager;
class Saga;

/// TransactionManager: ACID-ähnliche, atomare Multi-Layer-Updates via RocksDB WriteBatch
///
/// Thread-Safety:
/// - Thread-safe for all operations
/// - Transaction IDs generated atomically
/// - Transaction map protected by internal mutex
/// - Each Transaction object is NOT thread-safe (use from single thread)
/// - Transaction::finished_ uses atomic operations to prevent double commit/rollback
/// - Safe to call commitTransaction()/rollbackTransaction() from different threads
class TransactionManager {
public:
    using TransactionId = uint64_t;
    
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    explicit TransactionManager(RocksDBWrapper& db,
                                SecondaryIndexManager& secIdx,
                                GraphIndexManager& graphIdx,
                                VectorIndexManager& vecIdx);
    ~TransactionManager();

    class Transaction {
    public:
        Transaction(TransactionId id,
                    RocksDBWrapper& db,
                    SecondaryIndexManager& secIdx,
                    GraphIndexManager& graphIdx,
                    VectorIndexManager& vecIdx,
                    IsolationLevel isolation);
        ~Transaction();

        // Keine Kopie, aber Move
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        Transaction(Transaction&&) noexcept;
        Transaction& operator=(Transaction&&) noexcept;
        
        // Transaction metadata
        TransactionId getId() const { return id_; }
        IsolationLevel getIsolationLevel() const { return isolation_; }
        std::chrono::system_clock::time_point getStartTime() const { return start_time_; }
        uint64_t getDurationMs() const;
        bool isFinished() const { return finished_.load(std::memory_order_acquire); }

        // Relational
        Status putEntity(std::string_view table, const BaseEntity& entity);
        Status eraseEntity(std::string_view table, std::string_view pk);

        // Graph
        Status addEdge(const BaseEntity& edgeEntity);
        Status deleteEdge(std::string_view edgeId);
        
        // Vector
        Status addVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
        Status updateVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
        Status removeVector(std::string_view pk);

        // Abschluss
        Status commit();
        void rollback();

        // ── Savepoints ───────────────────────────────────────────────────────

        /**
         * @brief Record a savepoint at the current write position.
         *
         * Savepoints are stacked (LIFO).  Multiple calls to setSavePoint()
         * create multiple nested savepoints.
         *
         * Returns an error if the transaction is not active.
         */
        Status setSavePoint();

        /**
         * @brief Rollback all writes since the most recent setSavePoint().
         *
         * Pops the latest savepoint.  Returns an error if there is no
         * outstanding savepoint or the transaction is not active.
         */
        Status rollbackToSavePoint();

        /**
         * @brief Discard (commit) the most recent savepoint without undoing writes.
         *
         * Returns an error if there is no outstanding savepoint or the
         * transaction is not active.
         */
        Status popSavePoint();
        
        // SAGA support
        Saga& getSaga() { return *saga_; }
        const Saga& getSaga() const { return *saga_; }

    private:
        TransactionId id_;
        RocksDBWrapper& db_;
        SecondaryIndexManager& secIdx_;
        GraphIndexManager& graphIdx_;
        VectorIndexManager& vecIdx_;
        IsolationLevel isolation_;
        std::chrono::system_clock::time_point start_time_;
        std::unique_ptr<class RocksDBWrapper::TransactionWrapper> mvcc_txn_; // MVCC Transaction
        std::unique_ptr<Saga> saga_; // SAGA pattern for compensating actions
        std::atomic<bool> finished_{false};  // Race condition fix: atomic to prevent double commit/rollback
    };

    // Session-based transaction management
    TransactionId beginTransaction(IsolationLevel isolation = IsolationLevel::ReadCommitted);
    std::shared_ptr<Transaction> getTransaction(TransactionId id);
    Status commitTransaction(TransactionId id);
    void rollbackTransaction(TransactionId id);
    
    // Direct transaction (legacy API)
    Transaction begin(IsolationLevel isolation = IsolationLevel::ReadCommitted);
    
    // Statistics
    struct Stats {
        uint64_t total_begun;
        uint64_t total_committed;
        uint64_t total_aborted;
        uint64_t active_count;
        uint64_t avg_duration_ms;
        uint64_t max_duration_ms;
    };
    
    /**
     * @brief Get transaction statistics
     * 
     * Thread-safety: Statistics are eventually consistent. The atomic counters
     * (total_begun, total_committed, total_aborted) may be slightly out of sync
     * with the active/completed transaction maps due to timing differences.
     * This is acceptable for monitoring purposes and does not affect correctness.
     * 
     * @return Stats structure with current transaction statistics
     */
    Stats getStats() const;
    
    /**
     * @brief Get transaction statistics with lock-free consistent snapshot (SOLUTION 2B)
     * 
     * Uses sequence lock pattern for lock-free consistent reads with retry on concurrent modification.
     * Guarantees all counters are captured in a consistent state without holding locks.
     * 
     * Thread-safety:
     * - Lock-free for readers (zero contention)
     * - Optimistic read with retry on concurrent modification
     * - Scales to many threads reading statistics
     * - Small overhead for writers (2 atomic increments per update)
     * 
     * Performance:
     * - Reader: <10ns in fast path (no contention)
     * - Writer: +2 atomic increments (~5ns overhead)
     * - Perfect for high-frequency monitoring dashboards
     * 
     * @return Stats structure with guaranteed consistent snapshot
     */
    Stats getStatsLockFree() const;
    
    // Cleanup old completed transactions (after 1 hour by default)
    void cleanupOldTransactions(std::chrono::seconds max_age = std::chrono::hours(1));
    
    // Deadlock detection

    /// Victim-selection policy for deadlock resolution.
    /// When a cycle is detected one transaction is aborted to break the deadlock.
    enum class DeadlockVictimPolicy {
        YOUNGEST,  ///< Abort the transaction with the highest (newest) ID (default)
        OLDEST,    ///< Abort the transaction with the lowest (oldest) ID
        LEAST_EXPENSIVE, ///< Abort the transaction that holds the fewest locks
    };

    struct DeadlockInfo {
        std::vector<TransactionId> cycle;  // Transaction IDs involved in deadlock
        std::chrono::system_clock::time_point detected_at;
        TransactionId victim_id;  // Transaction chosen to abort
        DeadlockVictimPolicy policy_used{DeadlockVictimPolicy::YOUNGEST};
    };

    /// Aggregate metrics for deadlock events.
    struct DeadlockMetrics {
        uint64_t total_detected{0};     ///< Total deadlock cycles detected
        uint64_t total_resolved{0};     ///< Successfully resolved (victim aborted)
        double   avg_cycle_length{0.0}; ///< Average number of transactions per cycle
        uint64_t max_cycle_length{0};   ///< Largest cycle seen
        DeadlockVictimPolicy active_policy{DeadlockVictimPolicy::YOUNGEST};
    };

    /**
     * @brief Enable or disable deadlock detection
     * 
     * @param enabled true to enable, false to disable
     */
    void setDeadlockDetection(bool enabled);
    
    /**
     * @brief Set deadlock detection timeout
     * 
     * @param timeout_ms timeout in milliseconds
     */
    void setDeadlockTimeout(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Set the victim-selection policy for deadlock resolution.
     */
    void setDeadlockVictimPolicy(DeadlockVictimPolicy policy);

    /**
     * @brief Get current victim-selection policy.
     */
    DeadlockVictimPolicy getDeadlockVictimPolicy() const;

    /**
     * @brief Get recent deadlocks
     * 
     * @param max_age maximum age of deadlocks to return
     * @return vector of deadlock information
     */
    std::vector<DeadlockInfo> getDeadlocks(std::chrono::seconds max_age = std::chrono::hours(24)) const;
    
    /**
     * @brief Get deadlock statistics
     * 
     * @return total number of deadlocks detected
     */
    uint64_t getDeadlockCount() const { return total_deadlocks_.load(std::memory_order_relaxed); }

    /**
     * @brief Get detailed deadlock metrics.
     */
    DeadlockMetrics getDeadlockMetrics() const;

    /// Access the shared LockManager for external lock operations.
    LockManager& getLockManager() { return lock_manager_; }
    const LockManager& getLockManager() const { return lock_manager_; }

    // ── Phase 8: Durability & Crash-Recovery ─────────────────────────────────

    /**
     * @brief Enable transaction WAL (Write-Ahead Log) for crash recovery.
     *
     * Once enabled, every beginTransaction / commit / rollback call is logged
     * to the WAL file so that a subsequent recover() call can undo any
     * in-flight transactions that were active when the process crashed.
     *
     * @param wal_path      Path to the WAL file (created if absent).
     * @param sync_on_write fsync after every WAL append (safe but slower).
     */
    void enableCrashRecovery(const std::string& wal_path,
                              bool sync_on_write = true);

    /**
     * @brief Check whether the WAL contains in-flight transactions that
     *        need to be recovered.
     *
     * Should be called at startup before the first beginTransaction().
     * Returns false if WAL is disabled or clean.
     */
    bool needsCrashRecovery() const;

    /**
     * @brief Perform crash recovery.
     *
     * Scans the WAL file, identifies uncommitted transactions, and undoes
     * their operations by writing old values back to the database.
     * A CHECKPOINT is appended so the next startup skips already-recovered
     * entries.
     *
     * @return Result summary (in-flight count, rolled-back count, etc.).
     */
    transaction::CrashRecoveryManager::RecoveryResult crashRecover();

    /**
     * @brief Access the underlying CrashRecoveryManager (for testing/monitoring).
     *
     * Returns nullptr when crash recovery is disabled.
     */
    transaction::CrashRecoveryManager* getCrashRecoveryManager() {
        return crash_recovery_mgr_.get();
    }
    const transaction::CrashRecoveryManager* getCrashRecoveryManager() const {
        return crash_recovery_mgr_.get();
    }

private:
    RocksDBWrapper& db_;
    SecondaryIndexManager& secIdx_;
    GraphIndexManager& graphIdx_;
    VectorIndexManager& vecIdx_;

    // Shared lock manager (Phase 1: Lock Management)
    LockManager lock_manager_;

    // Phase 8: WAL-based crash recovery
    std::unique_ptr<transaction::CrashRecoveryManager> crash_recovery_mgr_;

    // Session management
    mutable std::mutex sessions_mutex_;
    std::unordered_map<TransactionId, std::shared_ptr<Transaction>> active_transactions_;
    std::unordered_map<TransactionId, std::shared_ptr<Transaction>> completed_transactions_;
    
    // Transaction ID generator
    std::atomic<uint64_t> next_transaction_id_{1};
    
    // Statistics
    std::atomic<uint64_t> total_begun_{0};
    std::atomic<uint64_t> total_committed_{0};
    std::atomic<uint64_t> total_aborted_{0};
    
    // SOLUTION 2B: Sequence lock for consistent lock-free statistics reads
    mutable std::atomic<uint64_t> stats_sequence_{0};
    
    TransactionId generateTransactionId();
    void moveToCompleted(TransactionId id);
    
    // Helper to update statistics with sequence lock protocol
    void updateStatsWithSeqLock(std::function<void()> update);
    
    // Deadlock detection state
    std::atomic<bool> deadlock_detection_enabled_{false};
    std::atomic<uint64_t> deadlock_timeout_ms_{1000};
    std::atomic<uint64_t> total_deadlocks_{0};

    // Victim selection policy (stored as underlying int for atomic access)
    std::atomic<int> victim_policy_{static_cast<int>(DeadlockVictimPolicy::YOUNGEST)};

    // Cumulative deadlock metrics
    std::atomic<uint64_t> deadlock_total_cycle_len_{0};  // sum of all cycle lengths
    std::atomic<uint64_t> deadlock_max_cycle_len_{0};    // largest cycle seen
    
    // Lock tracking for deadlock detection
    struct LockInfo {
        TransactionId holder;
        std::chrono::system_clock::time_point acquired_at;
    };
    
    mutable std::mutex lock_tracking_mutex_;
    std::unordered_map<std::string, LockInfo> held_locks_;  // key -> transaction holding it
    std::unordered_map<TransactionId, std::unordered_set<std::string>> waiting_for_;  // txn -> keys it's waiting for
    std::deque<DeadlockInfo> recent_deadlocks_;  // Use deque for efficient removal from front
    
    // Deadlock detection thread
    std::unique_ptr<std::thread> deadlock_detector_thread_;
    std::atomic<bool> deadlock_detector_running_{false};
    mutable std::mutex deadlock_detector_mutex_;  // Separate mutex for condition variable
    std::condition_variable deadlock_detector_cv_;
    
    void deadlockDetectorLoop();
    bool detectDeadlockCycle(std::vector<TransactionId>& cycle);
    void resolveDeadlock(const std::vector<TransactionId>& cycle);
    
    // Lock tracking helpers called by transactions
    void trackLockAcquired(TransactionId txn_id, const std::string& key);
    void trackLockReleased(TransactionId txn_id, const std::string& key);
    void trackLockWaiting(TransactionId txn_id, const std::string& key);
    void clearWaiting(TransactionId txn_id);
};

} // namespace themis
