/**
 * @file transaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <deque>
#include "storage/rocksdb_wrapper.h"
#include "storage/history_manager.h"
#include "transaction/lock_manager.h"
#include "transaction/isolation_level.h"
#include "transaction/crash_recovery_manager.h"
#include "transaction/deadlock_predictor.h"

namespace themis {

class BaseEntity;

class SecondaryIndexManager;
class GraphIndexManager;
class VectorIndexManager;
class Saga;

namespace transaction { class SnapshotManager; }

class TransactionManager {
public:
    using TransactionId = uint64_t;
    
    struct Status {
        bool ok = true;
        std::string message;
        std::string conflict_id = {};
        std::string conflict_set_id;
        std::vector<std::string> affected_keys;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg), "", "", {}}; }
        /**
         * @brief Conflict.
         * @param[in] msg Input parameter.
         * @param[in] cid Input parameter.
         * @param[in] keys Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Conflict(std::string msg, std::string cid,
                               std::vector<std::string> keys) {
            Status s;
            s.ok              = false;
            s.message         = std::move(msg);
            s.conflict_id     = cid;
            s.conflict_set_id = std::move(cid);
            s.affected_keys   = std::move(keys);
            return s;
        }
    };

    /**
     * @brief Transaction Manager.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] secIdx Input/output parameter.
     * @param[in,out] graphIdx Input/output parameter.
     * @param[in,out] vecIdx Input/output parameter.
     * @return Return value.
     */
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
                    IsolationLevel isolation,
                    LockManager* lock_manager = nullptr,
                    std::string_view tenant_id = {});
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
        /**
         * @brief Get Duration Ms.
         * @return Return value.
         */
        uint64_t getDurationMs() const;
        bool isFinished() const { return finished_.load(std::memory_order_acquire); }

        const std::string& getTenantId() const { return tenant_id_; }


        /**
         * @brief Set Timeout.
         * @param[in] timeout Input parameter.
         * @details Calls: count(), store().
         */
        void setTimeout(std::chrono::milliseconds timeout) {
            // Clamp to 0: a negative duration is treated the same as "no timeout".
            auto ms = timeout.count();
            timeout_ms_.store(ms > 0 ? static_cast<uint64_t>(ms) : 0u,
                              std::memory_order_relaxed);
        }

        std::chrono::milliseconds getTimeout() const {
            return std::chrono::milliseconds(timeout_ms_.load(std::memory_order_relaxed));
        }

        bool isTimedOut() const {
            uint64_t tms = timeout_ms_.load(std::memory_order_relaxed);
            if (tms == 0) {
              return false;
            }
            return getDurationMs() >= tms;
        }

        // Relational
        /**
         * @brief Put Entity.
         * @param[in] table Input parameter.
         * @param[in] entity Input parameter.
         * @return Return value.
         */
        Status putEntity(std::string_view table, const BaseEntity& entity);
        /**
         * @brief Erase Entity.
         * @param[in] table Input parameter.
         * @param[in] pk Input parameter.
         * @return Return value.
         */
        Status eraseEntity(std::string_view table, std::string_view pk);

        /**
         * @brief Read Entity Json.
         * @param[in] table Input parameter.
         * @param[in] pk Input parameter.
         * @return Return value.
         */
        std::optional<std::string> readEntityJson(std::string_view table,
                              std::string_view pk);

        // Graph
        /**
         * @brief Add Edge.
         * @param[in] edgeEntity Input parameter.
         * @return Return value.
         */
        Status addEdge(const BaseEntity& edgeEntity);
        /**
         * @brief Delete Edge.
         * @param[in] edgeId Input parameter.
         * @return Return value.
         */
        Status deleteEdge(std::string_view edgeId);
        
        // Vector
        Status addVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
        Status updateVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
        /**
         * @brief Remove Vector.
         * @param[in] pk Input parameter.
         * @return Return value.
         */
        Status removeVector(std::string_view pk);

        // Abschluss
        /**
         * @brief Commit.
         * @return Return value.
         */
        Status commit();
        /**
         * @brief Rollback.
         */
        void rollback();


        /**
         * @brief Get Entity Version.
         * @param[in] table Input parameter.
         * @param[in] pk Input parameter.
         * @return Return value.
         */
        std::optional<uint64_t> getEntityVersion(std::string_view table,
                                                  std::string_view pk);

        /**
         * @brief Optimistic Put.
         * @param[in] table Input parameter.
         * @param[in] entity Input parameter.
         * @param[in] expected_version Input parameter.
         * @return Return value.
         */
        Status optimisticPut(std::string_view table,
                              const BaseEntity& entity,
                              uint64_t expected_version);

        /**
         * @brief Optimistic Erase.
         * @param[in] table Input parameter.
         * @param[in] pk Input parameter.
         * @param[in] expected_version Input parameter.
         * @return Return value.
         */
        Status optimisticErase(std::string_view table,
                                std::string_view pk,
                                uint64_t expected_version);


        /**
         * @brief Track Predicate Read.
         * @param[in] start_key Input parameter.
         * @param[in] end_key Input parameter.
         * @return Return value.
         */
        Status trackPredicateRead(const std::string& start_key,
                                  const std::string& end_key);

        /**
         * @brief Check Serializable Write Conflict.
         * @param[in] key Input parameter.
         * @return Return value.
         */
        std::string checkSerializableWriteConflict(const std::string& key) const;


        /**
         * @brief Set Save Point.
         * @return Return value.
         */
        Status setSavePoint();

        /**
         * @brief Rollback To Save Point.
         * @return Return value.
         */
        Status rollbackToSavePoint();

        /**
         * @brief Pop Save Point.
         * @return Return value.
         */
        Status popSavePoint();


        /**
         * @brief Create Savepoint.
         * @param[in] name Input parameter.
         * @return Return value.
         */
        Status createSavepoint(std::string_view name);

        /**
         * @brief Rollback To Savepoint.
         * @param[in] name Input parameter.
         * @return Return value.
         */
        Status rollbackToSavepoint(std::string_view name);

        /**
         * @brief Release Savepoint.
         * @param[in] name Input parameter.
         * @return Return value.
         */
        Status releaseSavepoint(std::string_view name);

        /**
         * @brief Get Savepoints.
         * @return Return value.
         */
        std::vector<std::string> getSavepoints() const;

        /**
         * @brief Has Savepoint.
         * @param[in] name Input parameter.
         * @return True when the operation succeeds.
         */
        bool hasSavepoint(std::string_view name) const;
        

        /**
         * @brief Bulk Put Entities.
         * @param[in] table Input parameter.
         * @param[in] entities Input parameter.
         * @return Return value.
         */
        Status bulkPutEntities(std::string_view table,
                               const std::vector<BaseEntity>& entities);

        /**
         * @brief Bulk Erase Entities.
         * @param[in] table Input parameter.
         * @param[in] pks Input parameter.
         * @return Return value.
         */
        Status bulkEraseEntities(std::string_view table,
                                 const std::vector<std::string>& pks);

        // ── Read-Only Transaction Optimization ───────────────────────────────

        Status setReadOnly(bool read_only = true);

        bool isReadOnly() const { return read_only_; }

        bool hasWrites() const { return !write_set_.empty(); }

        // SAGA support
        /**
         * @brief Get Saga.
         * @return Return value.
         * @details Implements getSaga without additional internal calls.
         */
        Saga& getSaga() { return *saga_; }
        const Saga& getSaga() const { return *saga_; }

        // ── Transaction Explain ───────────────────────────────────────────────

        struct ExplainLockEntry {
            std::string key;       ///< Storage key that is locked
            std::string lock_type; ///< "SHARED", "EXCLUSIVE", "INTENT_SHARED", or "INTENT_EXCLUSIVE"
        };

        struct ExplainWriteEntry {
            std::string key;       ///< Storage key written or deleted
            std::string operation; ///< "put" or "delete"
        };

        struct ExplainResult {
            TransactionId txn_id{0};
            std::string   isolation_level;  ///< Human-readable isolation level name
            uint64_t      duration_ms{0};
            bool          is_finished{false};
            std::vector<ExplainLockEntry>  locks_held; ///< Locks currently held by this transaction
            std::vector<ExplainWriteEntry> write_set;  ///< Keys written/deleted (MVCC chain entries)
        };

        /**
         * @brief Explain.
         * @return Return value.
         */
        ExplainResult explain() const;

    private:
        /**
         * @brief Track Write.
         * @param[in] key Input parameter.
         * @param[in] operation Input parameter.
         */
        void trackWrite(std::string key, std::string operation);
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
        std::atomic<uint64_t> timeout_ms_{0}; ///< 0 = no timeout
        std::atomic<uint64_t> finished_duration_ms_{0}; ///< wall-clock duration captured at commit/rollback time

        LockManager* lock_manager_{nullptr};

        HistoryManager*  history_mgr_{nullptr};
        ConflictManager* conflict_mgr_{nullptr};

        std::unordered_map<std::string, std::vector<uint8_t>> base_values_;

        std::unordered_map<std::string, std::vector<uint8_t>> our_values_;

        /**
         * @brief Capture Duration.
         * @note Exception safety: noexcept.
         */
        void captureDuration() noexcept;

        struct SavepointEntry {
            std::string name = {};
            size_t saga_step_count{0}; ///< SAGA step count at the time the savepoint was created
        };
        std::vector<SavepointEntry> savepoints_; ///< named savepoints in creation order

        std::vector<ExplainWriteEntry> write_set_; ///< write-set accumulated for explain()

        bool read_only_{false};

        std::string tenant_id_;

        std::string makeNamespacedTable(std::string_view table) const {
            if (tenant_id_.empty()) {
              return std::string(table);
            }
            return "tenant:" + tenant_id_ + ":" + std::string(table);
        }

        std::string makeNamespacedKey(std::string_view key) const {
            if (tenant_id_.empty()) {
              return std::string(key);
            }
            return "tenant:" + tenant_id_ + ":" + std::string(key);
        }

        friend class TransactionManager;  ///< allow TransactionManager to set history_mgr_/conflict_mgr_
    };

    // Session-based transaction management
    TransactionId beginTransaction(IsolationLevel isolation = IsolationLevel::ReadCommitted);

    TransactionId beginTransaction(std::string_view tenant_id,
                                   IsolationLevel isolation = IsolationLevel::ReadCommitted);

    /**
     * @brief Get Transaction.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::shared_ptr<Transaction> getTransaction(TransactionId id);
    /**
     * @brief Commit Transaction.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    Status commitTransaction(TransactionId id);
    /**
     * @brief Rollback Transaction.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool rollbackTransaction(TransactionId id);

    /**
     * @brief Explain Transaction.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::optional<Transaction::ExplainResult> explainTransaction(TransactionId id) const;

    // ── Per-tenant transaction namespace ─────────────────────────────────────

    struct TenantTransactionStats {
        std::string tenant_id;
        uint64_t total_begun{0};
        uint64_t total_committed{0};
        uint64_t total_aborted{0};
        uint64_t active_count{0};
    };

    /**
     * @brief Get Tenant Transaction Stats.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    TenantTransactionStats getTenantTransactionStats(std::string_view tenant_id) const;

    /**
     * @brief Get All Tenant Transaction Stats.
     * @return Return value.
     */
    std::vector<TenantTransactionStats> getAllTenantTransactionStats() const;

    /**
     * @brief Get Active Tenant Transaction Count.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t getActiveTenantTransactionCount(std::string_view tenant_id) const;

    /**
     * @brief List Tenant Transaction Ids.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::vector<TransactionId> listTenantTransactionIds(std::string_view tenant_id) const;

    /**
     * @brief Abort Tenant Transactions.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t abortTenantTransactions(std::string_view tenant_id);
    
    // Direct transaction (legacy API)
    Transaction begin(IsolationLevel isolation = IsolationLevel::ReadCommitted);


    /**
     * @brief Set Default Transaction Timeout.
     * @param[in] timeout Input parameter.
     */
    void setDefaultTransactionTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Get Default Transaction Timeout.
     * @return Return value.
     */
    std::chrono::milliseconds getDefaultTransactionTimeout() const;

    uint64_t getTimeoutCount() const {
        return total_timed_out_.load(std::memory_order_relaxed);
    }
    
    // Statistics
    struct Stats {
        uint64_t total_begun = 0;
        uint64_t total_committed;
        uint64_t total_aborted;
        uint64_t total_timed_out;  ///< Transactions rolled back due to timeout
        uint64_t active_count;
        uint64_t avg_duration_ms;
        uint64_t max_duration_ms;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
    /**
     * @brief Get Stats Lock Free.
     * @return Return value.
     */
    Stats getStatsLockFree() const;
    
    // Cleanup old completed transactions (after 1 hour by default)
    void cleanupOldTransactions(std::chrono::seconds max_age = std::chrono::hours(1));


    /**
     * @brief Set Transaction Timeout.
     * @param[in] timeout_ms Input parameter.
     */
    void setTransactionTimeout(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Get Transaction Timeout.
     * @return Return value.
     */
    std::chrono::milliseconds getTransactionTimeout() const;

    /**
     * @brief Get Timed Out Count.
     * @return Return value.
     */
    uint64_t getTimedOutCount() const;

    /**
     * @brief Abort Timed Out Transactions.
     * @return Return value.
     */
    size_t abortTimedOutTransactions();
    
    // Deadlock detection

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

    struct DeadlockMetrics {
        uint64_t total_detected{0};     ///< Total deadlock cycles detected
        uint64_t total_resolved{0};     ///< Successfully resolved (victim aborted)
        double   avg_cycle_length{0.0}; ///< Average number of transactions per cycle
        uint64_t max_cycle_length{0};   ///< Largest cycle seen
        DeadlockVictimPolicy active_policy{DeadlockVictimPolicy::YOUNGEST};
    };

    /**
     * @brief Set Deadlock Detection.
     * @param[in] enabled Input parameter.
     */
    void setDeadlockDetection(bool enabled);
    
    /**
     * @brief Set Deadlock Timeout.
     * @param[in] timeout_ms Input parameter.
     */
    void setDeadlockTimeout(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Set Deadlock Victim Policy.
     * @param[in] policy Input parameter.
     */
    void setDeadlockVictimPolicy(DeadlockVictimPolicy policy);

    /**
     * @brief Get Deadlock Victim Policy.
     * @return Return value.
     */
    DeadlockVictimPolicy getDeadlockVictimPolicy() const;

    std::vector<DeadlockInfo> getDeadlocks(std::chrono::seconds max_age = std::chrono::hours(24)) const;
    
    uint64_t getDeadlockCount() const { return total_deadlocks_.load(std::memory_order_relaxed); }

    /**
     * @brief Get Deadlock Metrics.
     * @return Return value.
     */
    DeadlockMetrics getDeadlockMetrics() const;


    /**
     * @brief Set Deadlock Predictor.
     * @param[in,out] predictor Input/output parameter.
     */
    void setDeadlockPredictor(DeadlockPredictor* predictor);

    /**
     * @brief Get Deadlock Predictor.
     * @return Pointer to the result.
     */
    DeadlockPredictor* getDeadlockPredictor() const;

    /**
     * @brief Predict Deadlock Probability.
     * @param[in] proposed_locks Input parameter.
     * @return Return value.
     */
    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks) const;

    /**
     * @brief Recommend Lock Order.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys) const;

    /**
     * @brief Recommend Timeout.
     * @param[in] keys Input parameter.
     * @return Return value.
     */
    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys) const;

    /**
     * @brief Get Lock Manager.
     * @return Return value.
     * @details Implements getLockManager without additional internal calls.
     */
    LockManager& getLockManager() { return lock_manager_; }
    const LockManager& getLockManager() const { return lock_manager_; }

    // ── Phase 8: Durability & Crash-Recovery ─────────────────────────────────

    void enableCrashRecovery(const std::string& wal_path,
                              bool sync_on_write = true);

    /**
     * @brief Needs Crash Recovery.
     * @return True when the operation succeeds.
     */
    bool needsCrashRecovery() const;

    /**
     * @brief Crash Recover.
     * @return Return value.
     */
    transaction::CrashRecoveryManager::RecoveryResult crashRecover();

    /**
     * @brief Get Crash Recovery Manager.
     * @return Pointer to the result.
     * @details Calls: get().
     */
    transaction::CrashRecoveryManager* getCrashRecoveryManager() {
        return crash_recovery_mgr_.get();
    }
    const transaction::CrashRecoveryManager* getCrashRecoveryManager() const {
        return crash_recovery_mgr_.get();
    }


    /**
     * @brief Set History Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setHistoryManager without additional internal calls.
     */
    void setHistoryManager(HistoryManager* mgr) { history_mgr_ = mgr; }

    /**
     * @brief Set Conflict Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setConflictManager without additional internal calls.
     */
    void setConflictManager(ConflictManager* mgr) { conflict_mgr_ = mgr; }

    // ── Time-travel queries ───────────────────────────────────────────────────

    struct TimeTravelRecord {
        std::string base_key;          ///< Live storage key (e.g. "entity:users:u1")
        HLCTimestamp timestamp;        ///< HLC timestamp of this version
        std::string op;                ///< "put" (value present) or "del" (tombstone)
        std::vector<uint8_t> value;    ///< Serialized entity bytes; empty for "del"
        uint64_t txn_id{0};           ///< Transaction that produced this version
    };

    /**
     * @brief Set Snapshot Manager.
     * @param[in,out] mgr Input/output parameter.
     * @details Implements setSnapshotManager without additional internal calls.
     */
    void setSnapshotManager(transaction::SnapshotManager* mgr) {
        snapshot_mgr_ = mgr;
    }

    /**
     * @brief Read Entity At Timestamp.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] ts Input parameter.
     * @return Return value.
     */
    std::optional<TimeTravelRecord> readEntityAtTimestamp(
        std::string_view table,
        std::string_view pk,
        HLCTimestamp ts) const;

    /**
     * @brief Read Entity At Unix Ms.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] unix_ms Input parameter.
     * @return Return value.
     */
    std::optional<TimeTravelRecord> readEntityAtUnixMs(
        std::string_view table,
        std::string_view pk,
        int64_t unix_ms) const;

    /**
     * @brief Read Entity At Snapshot.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @param[in] tag_name Name of the tag.
     * @return Return value.
     */
    std::optional<TimeTravelRecord> readEntityAtSnapshot(
        std::string_view table,
        std::string_view pk,
        const std::string& tag_name) const;

    /**
     * @brief List Entity Versions.
     * @param[in] table Input parameter.
     * @param[in] pk Input parameter.
     * @return Return value.
     */
    std::vector<TimeTravelRecord> listEntityVersions(
        std::string_view table,
        std::string_view pk) const;

    // ── Serializable Snapshot Isolation (SSI) configuration ──────────────────

    struct SSIConfig {
        bool enable_predicate_locking = true;

        size_t max_predicate_locks = 10000;

        std::chrono::milliseconds conflict_detection_interval{100};
    };

    /**
     * @brief Set SSIConfig.
     * @param[in] config Input parameter.
     */
    void setSSIConfig(const SSIConfig& config);

    /**
     * @brief Get SSIConfig.
     * @return Return value.
     */
    SSIConfig getSSIConfig() const;

    struct SerializationConflict {
        TransactionId other_txn_id{0};

        std::string key = {};

        std::string conflict_type;

        std::string message;
    };

    /**
     * @brief Detect Conflicts.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    std::vector<SerializationConflict> detectConflicts(TransactionId txn_id) const;

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
    std::atomic<uint64_t> total_timed_out_{0};     ///< Incremented by abortTimedOutTransactions()
    
    // SOLUTION 2B: Sequence lock for consistent lock-free statistics reads
    mutable std::atomic<uint64_t> stats_sequence_{0};
    
    /**
     * @brief Generate Transaction Id.
     * @return Return value.
     */
    TransactionId generateTransactionId();
    /**
     * @brief Move To Completed.
     * @param[in] id Input parameter.
     */
    void moveToCompleted(TransactionId id);
    
    // Helper to update statistics with sequence lock protocol
    void updateStatsWithSeqLock(std::function<void()> update);

    // Per-tenant statistics: protected by sessions_mutex_
    struct TenantStatsEntry {
        uint64_t total_begun{0};
        uint64_t total_committed{0};
        uint64_t total_aborted{0};
    };
    std::unordered_map<std::string, TenantStatsEntry> tenant_stats_;  ///< keyed by tenant_id

    /**
     * @brief Count Active Tenant Transactions Locked.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    size_t countActiveTenantTransactionsLocked(std::string_view tenant_id) const;

    // Transaction timeout
    std::atomic<uint64_t> default_transaction_timeout_ms_{0}; ///< 0 = no default timeout
    /**
     * @brief Timeout Expired Transactions.
     */
    void timeoutExpiredTransactions(); ///< roll back active transactions that exceeded their timeout
    /**
     * @brief Apply Default Timeout.
     * @param[in,out] txn Input/output parameter.
     */
    void applyDefaultTimeout(Transaction& txn) const; ///< apply default timeout if configured
    
    // Deadlock detection state
    std::atomic<bool> deadlock_detection_enabled_{false};
    std::atomic<uint64_t> deadlock_timeout_ms_{1000};

    // Transaction timeout (0 = disabled)
    std::atomic<uint64_t> transaction_timeout_ms_{0};
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
    
    /**
     * @brief Deadlock Detector Loop.
     */
    void deadlockDetectorLoop();
    /**
     * @brief Detect Deadlock Cycle.
     * @param[in,out] cycle Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool detectDeadlockCycle(std::vector<TransactionId>& cycle);
    /**
     * @brief Resolve Deadlock.
     * @param[in] cycle Input parameter.
     */
    void resolveDeadlock(const std::vector<TransactionId>& cycle);
    
    /**
     * @brief Track Lock Acquired.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     */
    void trackLockAcquired(TransactionId txn_id, const std::string& key);
    /**
     * @brief Track Lock Released.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     */
    void trackLockReleased(TransactionId txn_id, const std::string& key);
    /**
     * @brief Track Lock Waiting.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] key Input parameter.
     */
    void trackLockWaiting(TransactionId txn_id, const std::string& key);
    /**
     * @brief Clear Waiting.
     * @param[in] txn_id Identifier of the txn.
     */
    void clearWaiting(TransactionId txn_id);

    HistoryManager*  history_mgr_{nullptr};
    ConflictManager* conflict_mgr_{nullptr};
    transaction::SnapshotManager* snapshot_mgr_{nullptr};
    std::atomic<DeadlockPredictor*> deadlock_predictor_{nullptr};

    // SSI configuration – protected by ssi_config_mutex_
    mutable std::mutex ssi_config_mutex_;
    SSIConfig ssi_config_;
};

} // namespace themis

