/**
 * @file transaction_manager.h
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
        /// Non-empty when the commit failed due to a write-write conflict and a
        /// ConflictRecord was persisted.  For single-key conflicts this is the
        /// individual ConflictRecord ID.  When a ConflictSet is also created,
        /// this field holds the conflict_set_id for backwards compatibility with
        /// callers that only inspect conflict_id.
        std::string conflict_id;
        /// ID of the ConflictSet that groups all per-key ConflictRecord artifacts
        /// for this failed commit.  Use ConflictManager::getConflictSet() to
        /// retrieve the full list of affected keys and individual conflict IDs.
        /// Equal to conflict_id when set (both hold the set ID).
        std::string conflict_set_id;
        /// Keys involved in the conflict (filled alongside conflict_id).
        std::vector<std::string> affected_keys;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg), "", "", {}}; }
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

    explicit TransactionManager(RocksDBWrapper& db,
                                SecondaryIndexManager& secIdx,
                                GraphIndexManager& graphIdx,
                                VectorIndexManager& vecIdx);
    ~TransactionManager();

    /** @brief Transaction object. */
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
        uint64_t getDurationMs() const;
        bool isFinished() const { return finished_.load(std::memory_order_acquire); }

        /**
         * @brief Return the tenant ID associated with this transaction.
         *
         * Returns an empty string when no tenant namespace was set (i.e. the
         * transaction operates in the global / default namespace).
         */
        const std::string& getTenantId() const { return tenant_id_; }

        // ── Transaction timeout ───────────────────────────────────────────────

        /**
         * @brief Set a timeout for this transaction.
         *
         * Once the transaction has been active for longer than @p timeout,
         * all further write operations and commit() will return an error,
         * and the background monitor in TransactionManager will automatically
         * roll it back.
         *
         * @param timeout  Maximum lifetime; pass 0 to disable the timeout.
         */
        void setTimeout(std::chrono::milliseconds timeout) {
            // Clamp to 0: a negative duration is treated the same as "no timeout".
            auto ms = timeout.count();
            timeout_ms_.store(ms > 0 ? static_cast<uint64_t>(ms) : 0u,
                              std::memory_order_relaxed);
        }

        /**
         * @brief Return the configured timeout (0 = no timeout).
         */
        std::chrono::milliseconds getTimeout() const {
            return std::chrono::milliseconds(timeout_ms_.load(std::memory_order_relaxed));
        }

        /**
         * @brief Return true if the transaction has exceeded its timeout.
         *
         * Always returns false when no timeout is set (timeout == 0).
         */
        bool isTimedOut() const {
            uint64_t tms = timeout_ms_.load(std::memory_order_relaxed);
            if (tms == 0) {
              return false;
            }
            return getDurationMs() >= tms;
        }

        // Relational
        Status putEntity(std::string_view table, const BaseEntity& entity);
        Status eraseEntity(std::string_view table, std::string_view pk);

        /**
         * @brief Read an entity through the active transaction snapshot.
         *
         * Performs a point lookup on the canonical entity key
         * `entity:{table}:{pk}` using the transaction's MVCC view. The lookup
         * observes this transaction's own uncommitted writes (read-your-writes)
         * and remains isolated from uncommitted writes of other transactions.
         *
         * @param table Logical table/collection name.
         * @param pk    Primary key.
         * @return JSON string representation of the entity when present;
         *         std::nullopt when the entity does not exist in the current
         *         transaction snapshot or when no active transaction exists.
         */
        std::optional<std::string> readEntityJson(std::string_view table,
                              std::string_view pk);

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

        // ── Optimistic Concurrency Control (OCC) ─────────────────────────────

        /**
         * @brief Read the current OCC version of an entity without acquiring a lock.
         *
         * Version numbers are stored under `occ:ver:{table}:{pk}` as a
         * little-endian uint64_t.  A missing key means the entity does not yet
         * exist and its effective version is 0.
         *
         * This method is safe to call from any isolation level.  It reads
         * through the MVCC snapshot so the result is consistent with all other
         * reads performed by this transaction.
         *
         * @param table  Table name.
         * @param pk     Primary key of the entity.
         * @return       Current version (0 if the entity does not exist), or
         *               std::nullopt when the transaction is not active.
         */
        std::optional<uint64_t> getEntityVersion(std::string_view table,
                                                  std::string_view pk);

        /**
         * @brief Write an entity only if its current version matches @p expected_version.
         *
         * Implements optimistic locking: the entity is written (or created) only
         * when no concurrent transaction has already modified it.  On success the
         * stored version is atomically incremented to `expected_version + 1`.
         *
         * Pass `expected_version = 0` to create a new entity (fails if the entity
         * already exists with version > 0).
         *
         * @param table             Table name.
         * @param entity            Entity to write.
         * @param expected_version  Version the caller observed; write proceeds
         *                          only when the stored version equals this value.
         * @return Status::OK() on success.
         *         Error with "OCC version conflict" when the stored version differs
         *         from @p expected_version (caller should retry the transaction).
         *         Error with "OCC entity already exists" when @p expected_version
         *         is 0 but a stored version > 0 is found (entity already created
         *         by another transaction).
         */
        Status optimisticPut(std::string_view table,
                              const BaseEntity& entity,
                              uint64_t expected_version);

        /**
         * @brief Delete an entity only if its current version matches @p expected_version.
         *
         * On success, both the entity and its version key are removed from the
         * transaction's write set.  The effective version after a successful erase
         * is 0 (entity no longer exists).
         *
         * @param table             Table name.
         * @param pk                Primary key of the entity to delete.
         * @param expected_version  Version the caller observed; deletion proceeds
         *                          only when the stored version equals this value
         *                          and is greater than 0.
         * @return Status::OK() on success.
         *         Error with "OCC version conflict" when versions differ.
         *         Error with "OCC entity not found" when the entity does not exist
         *         (stored version is 0 or missing).
         */
        Status optimisticErase(std::string_view table,
                                std::string_view pk,
                                uint64_t expected_version);

        // ── Serializable Snapshot Isolation (SSI) / Predicate Locking ────────

        /**
         * @brief Acquire a SIREAD (predicate) lock for SERIALIZABLE isolation (SSI).
         *
         * Records that this transaction has read all keys in the closed interval
         * [@p start_key, @p end_key].  Any other SERIALIZABLE transaction that
         * subsequently writes a key inside this range will be detected as a
         * serialization conflict and aborted.
         *
         * This is the SIREAD ("Serializable Isolation READ") lock described in
         * the SSI literature.  Unlike 2PL read locks, SIREAD locks do not block
         * concurrent writers; conflicts are detected lazily at write time, which
         * makes this approach better than traditional 2PL for read-heavy workloads.
         *
         * No-op when the isolation level is not SERIALIZABLE.
         *
         * @param start_key  Lower bound of the range (inclusive).
         * @param end_key    Upper bound of the range (inclusive). May equal
         *                   @p start_key for a single-key predicate.
         * @return Status::OK() on success; error if the transaction is not active.
         */
        Status trackPredicateRead(const std::string& start_key,
                                  const std::string& end_key);

        /**
         * @brief Check whether writing @p key would violate serializability.
         *
         * For SERIALIZABLE transactions only: returns a non-empty error message
         * when @p key falls within a predicate range held by another active
         * SERIALIZABLE transaction, indicating a potential phantom / write-skew
         * anomaly.
         *
         * Always returns an empty string for non-SERIALIZABLE isolation levels.
         *
         * @param key  The storage key that is about to be written.
         * @return Non-empty error message on conflict; empty string if safe.
         */
        std::string checkSerializableWriteConflict(const std::string& key) const;


        /**
         * @brief Record a savepoint at the current write position.
         *
         * Savepoints are stacked (LIFO).  Multiple calls to setSavePoint()
         * create multiple nested savepoints.
         *
         * Returns an error if the transaction is not active.
         *
         * @warning Do NOT mix this anonymous stack API with the named savepoint
         *          API (createSavepoint / rollbackToSavepoint / releaseSavepoint).
         *          Both share the same underlying RocksDB savepoint stack.  Using
         *          both on the same Transaction will corrupt the named-savepoint
         *          bookkeeping and produce undefined behaviour.
         */
        Status setSavePoint();

        /**
         * @brief Rollback all writes since the most recent setSavePoint().
         *
         * Pops the latest savepoint.  Returns an error if there is no
         * outstanding savepoint or the transaction is not active.
         *
         * @warning Do not mix with the named savepoint API. See setSavePoint().
         */
        Status rollbackToSavePoint();

        /**
         * @brief Discard (commit) the most recent savepoint without undoing writes.
         *
         * Returns an error if there is no outstanding savepoint or the
         * transaction is not active.
         *
         * @warning Do not mix with the named savepoint API. See setSavePoint().
         */
        Status popSavePoint();

        // ── Named savepoints (partial rollback) ──────────────────────────────

        /**
         * @brief Create a named savepoint at the current write position.
         *
         * Named savepoints allow rolling back to a specific, labelled point
         * rather than relying on LIFO stack ordering.  Multiple savepoints
         * may be active simultaneously; each name must be unique within the
         * transaction.
         *
         * @param name  Non-empty identifier for the savepoint.
         * @return      Error if the transaction is not active or the name is
         *              already in use.
         */
        Status createSavepoint(std::string_view name);

        /**
         * @brief Rollback all writes made after the named savepoint.
         *
         * The named savepoint and any savepoints created after it are all
         * removed.  An error is returned if no savepoint with @p name exists
         * or the transaction is not active.
         *
         * @param name  Name of the target savepoint.
         */
        Status rollbackToSavepoint(std::string_view name);

        /**
         * @brief Discard the named savepoint (and any newer ones) without
         *        rolling back any writes.
         *
         * Equivalent to SQL "RELEASE SAVEPOINT".  Returns an error if no
         * savepoint with @p name exists or the transaction is not active.
         *
         * @param name  Name of the savepoint to release.
         */
        Status releaseSavepoint(std::string_view name);

        /**
         * @brief Return the names of all active savepoints in creation order.
         */
        std::vector<std::string> getSavepoints() const;

        /**
         * @brief Return true if a savepoint with the given name exists.
         */
        bool hasSavepoint(std::string_view name) const;
        
        // ── Bulk API ──────────────────────────────────────────────────────────

        /**
         * @brief Insert or update multiple entities in a single batch.
         *
         * Equivalent to calling putEntity() for each entity in @p entities,
         * but with only one active/timed-out check for the whole batch instead
         * of one per row.  All writes are accumulated inside the same underlying
         * MVCC transaction and committed atomically together with any other
         * operations in this transaction.
         *
         * @param table    Table name.
         * @param entities Entities to insert or update.  Each entity must have
         *                 a non-empty primary key.  An empty vector is a no-op.
         * @return Status::OK() on success; the first error encountered,
         *         including the (0-based) index of the failing entity in the
         *         message.
         */
        Status bulkPutEntities(std::string_view table,
                               const std::vector<BaseEntity>& entities);

        /**
         * @brief Delete multiple entities in a single batch.
         *
         * Equivalent to calling eraseEntity() for each primary key in @p pks,
         * but with only one active/timed-out check for the whole batch.  All
         * deletes are accumulated inside the same underlying MVCC transaction.
         *
         * @param table  Table name.
         * @param pks    Primary keys of entities to delete.  Each key must be
         *               non-empty.  An empty vector is a no-op.
         * @return Status::OK() on success; the first error encountered.
         */
        Status bulkEraseEntities(std::string_view table,
                                 const std::vector<std::string>& pks);

        // ── Read-Only Transaction Optimization ───────────────────────────────

        /**
         * @brief Mark this transaction as read-only.
         *
         * When @p read_only is true, any subsequent write operation (putEntity,
         * eraseEntity, addEdge, deleteEdge, addVector, updateVector, removeVector,
         * optimisticPut, optimisticErase, bulkPutEntities, bulkEraseEntities)
         * will immediately return an error without modifying the write set.
         * commit() on a read-only transaction is a no-op that releases the
         * underlying snapshot without writing to the WAL.
         *
         * Calling setReadOnly(true) is rejected when the transaction already
         * has writes in its write set (hasWrites() == true), because the
         * read-only commit fast-path would silently discard those writes.
         *
         * @param read_only  true to enable read-only mode, false to disable.
         * @return Status::OK() on success; error if the transaction is not
         *         active, or if read_only=true but writes already exist.
         */
        Status setReadOnly(bool read_only = true);

        /**
         * @brief Return true if this transaction is in read-only mode.
         *
         * Read-only mode may be set explicitly via setReadOnly(true), or
         * automatically detected after commit() when no writes were performed.
         */
        bool isReadOnly() const { return read_only_; }

        /**
         * @brief Return true if this transaction has accumulated any write
         *        operations since it started (or since the last savepoint
         *        rollback that undid all writes).
         *
         * Based on the write-set tracked for explain(); each putEntity,
         * eraseEntity, addEdge, deleteEdge, addVector, optimisticPut, etc.
         * contributes at least one entry.
         */
        bool hasWrites() const { return !write_set_.empty(); }

        // SAGA support
        Saga& getSaga() { return *saga_; }
        const Saga& getSaga() const { return *saga_; }

        // ── Transaction Explain ───────────────────────────────────────────────

        /**
         * @brief One lock entry in the explain report.
         */
        struct ExplainLockEntry {
            std::string key;       ///< Storage key that is locked
            std::string lock_type; ///< "SHARED", "EXCLUSIVE", "INTENT_SHARED", or "INTENT_EXCLUSIVE"
        };

        /**
         * @brief One write-set entry in the explain report.
         *
         * Each key written or deleted by this transaction is recorded here,
         * representing the MVCC version chain entries that will be created
         * on commit.
         */
        struct ExplainWriteEntry {
            std::string key;       ///< Storage key written or deleted
            std::string operation; ///< "put" or "delete"
        };

        /**
         * @brief Full explain report for a transaction.
         *
         * Returned by explain() and by TransactionManager::explainTransaction().
         * Contains the current lock set, write set (MVCC version chain entries),
         * isolation level, and elapsed duration.
         */
        struct ExplainResult {
            TransactionId txn_id{0};
            std::string   isolation_level;  ///< Human-readable isolation level name
            uint64_t      duration_ms{0};
            bool          is_finished{false};
            std::vector<ExplainLockEntry>  locks_held; ///< Locks currently held by this transaction
            std::vector<ExplainWriteEntry> write_set;  ///< Keys written/deleted (MVCC chain entries)
        };

        /**
         * @brief Produce an explain report for this transaction.
         *
         * Collects the locks currently held (via LockManager) and the write set
         * accumulated since the transaction started.  Safe to call on both active
         * and finished transactions.
         */
        ExplainResult explain() const;

    private:
        /// Track a key written or deleted by this transaction (for explain()).
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

        /// Non-owning pointer to the shared LockManager; used for predicate
        /// lock tracking by SERIALIZABLE transactions. May be nullptr for
        /// non-SERIALIZABLE transactions or legacy Transaction objects.
        LockManager* lock_manager_{nullptr};

        /// Optional non-owning pointers to history/conflict managers.
        /// Set by TransactionManager after construction when history tracking is enabled.
        HistoryManager*  history_mgr_{nullptr};
        ConflictManager* conflict_mgr_{nullptr};

        /// Per-key pre-write (base) values captured before each write.
        /// Used to populate ConflictRecord::base_value on commit failure.
        std::unordered_map<std::string, std::vector<uint8_t>> base_values_;

        /// Per-key values that this transaction is trying to write.
        /// Used to populate ConflictRecord::ours_value on commit failure.
        std::unordered_map<std::string, std::vector<uint8_t>> our_values_;

        /// Record the current wall-clock duration into finished_duration_ms_.
        /// Must be called while the caller holds exclusive ownership (i.e. after
        /// the finished_ CAS succeeds but before releasing the transaction).
        void captureDuration() noexcept;

        struct SavepointEntry {
            std::string name;
            size_t saga_step_count{0}; ///< SAGA step count at the time the savepoint was created
        };
        std::vector<SavepointEntry> savepoints_; ///< named savepoints in creation order

        std::vector<ExplainWriteEntry> write_set_; ///< write-set accumulated for explain()

        /// When true, write operations are rejected and commit() is a no-op.
        bool read_only_{false};

        /// Tenant namespace for this transaction.  Empty = global / default namespace.
        std::string tenant_id_;

        /**
         * @brief Apply the tenant namespace prefix to a table name.
         *
         * When this transaction is scoped to a tenant, returns
         * "tenant:{tenant_id}:{table}".  Otherwise returns @p table unchanged.
         * Used to route all secondary-index operations into the correct
         * per-tenant namespace.
         */
        std::string makeNamespacedTable(std::string_view table) const {
            if (tenant_id_.empty()) {
              return std::string(table);
            }
            return "tenant:" + tenant_id_ + ":" + std::string(table);
        }

        /**
         * @brief Apply the tenant namespace prefix to a raw storage key.
         *
         * When this transaction is scoped to a tenant, returns
         * "tenant:{tenant_id}:{key}".  Otherwise returns @p key unchanged.
         * Used to physically isolate MVCC keys between tenants.
         */
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

    /**
     * @brief Begin a transaction scoped to a specific tenant namespace.
     *
     * All entity, edge, and vector key operations performed inside the
     * returned transaction are physically isolated under a per-tenant key
     * prefix ("tenant:{tenant_id}:...").  Secondary-index operations use the
     * same prefix so that queries against one tenant never see data from
     * another tenant.
     *
     * Per-tenant statistics (begun, committed, aborted) are tracked
     * independently of the global statistics for the lifetime of the manager.
     *
     * @param tenant_id  Non-empty tenant identifier.  Passing an empty string
     *                   is equivalent to calling beginTransaction(isolation).
     * @param isolation  Desired isolation level (default: ReadCommitted).
     * @return           A globally unique transaction ID.
     */
    TransactionId beginTransaction(std::string_view tenant_id,
                                   IsolationLevel isolation = IsolationLevel::ReadCommitted);

    std::shared_ptr<Transaction> getTransaction(TransactionId id);
    Status commitTransaction(TransactionId id);
    /**
     * @brief Roll back an active session transaction.
     *
     * @return true  if the transaction was found in the active map and rolled back.
     * @return false if no active transaction with this ID exists (already completed).
     */
    bool rollbackTransaction(TransactionId id);

    /**
     * @brief Return an explain report for an active or recently completed transaction.
     *
     * The report includes the locks currently held by the transaction and the
     * write set (MVCC version chain entries) accumulated since begin.
     *
     * @param id  Transaction ID returned by beginTransaction().
     * @return    ExplainResult on success, std::nullopt if no transaction with
     *            @p id is found in the active or completed maps.
     */
    std::optional<Transaction::ExplainResult> explainTransaction(TransactionId id) const;

    // ── Per-tenant transaction namespace ─────────────────────────────────────

    /**
     * @brief Per-tenant transaction statistics.
     *
     * All counters represent the lifetime totals since the TransactionManager
     * was constructed (i.e. they are never reset).
     */
    struct TenantTransactionStats {
        std::string tenant_id;
        uint64_t total_begun{0};
        uint64_t total_committed{0};
        uint64_t total_aborted{0};
        uint64_t active_count{0};
    };

    /**
     * @brief Return the transaction statistics for a single tenant.
     *
     * Returns a zeroed-out TenantTransactionStats (with the given tenant_id)
     * when no transaction has ever been started for @p tenant_id.
     *
     * @param tenant_id  Tenant identifier to query.
     */
    TenantTransactionStats getTenantTransactionStats(std::string_view tenant_id) const;

    /**
     * @brief Return per-tenant statistics for all tenants that have ever
     *        started at least one transaction.
     */
    std::vector<TenantTransactionStats> getAllTenantTransactionStats() const;

    /**
     * @brief Count the number of currently active transactions for a tenant.
     *
     * @param tenant_id  Tenant identifier to query.
     * @return           Number of active (uncommitted/unrolled-back) transactions.
     */
    size_t getActiveTenantTransactionCount(std::string_view tenant_id) const;

    /**
     * @brief List all active transaction IDs for a given tenant.
     *
     * @param tenant_id  Tenant identifier to query.
     * @return           Vector of active transaction IDs in unspecified order.
     */
    std::vector<TransactionId> listTenantTransactionIds(std::string_view tenant_id) const;

    /**
     * @brief Abort all active transactions belonging to @p tenant_id.
     *
     * Useful when a tenant is being suspended or deleted.  Each transaction
     * is rolled back and moved to the completed map.
     *
     * @param tenant_id  Tenant whose transactions should be aborted.
     * @return           Number of transactions actually aborted.
     */
    size_t abortTenantTransactions(std::string_view tenant_id);
    
    // Direct transaction (legacy API)
    Transaction begin(IsolationLevel isolation = IsolationLevel::ReadCommitted);

    // ── Transaction timeout ───────────────────────────────────────────────────

    /**
     * @brief Set the default timeout applied to every new transaction.
     *
     * When a non-zero timeout is set, every transaction created via
     * beginTransaction() or begin() has its timeout pre-configured to this
     * value.  Existing active transactions are not affected.
     *
     * The background monitor in the TransactionManager automatically rolls
     * back any active transaction that has exceeded its timeout.
     *
     * @param timeout  Default timeout; pass 0 ms to disable (no timeout).
     */
    void setDefaultTransactionTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Return the currently configured default transaction timeout.
     *
     * Returns 0 ms when no default timeout is set.
     */
    std::chrono::milliseconds getDefaultTransactionTimeout() const;

    /**
     * @brief Return the number of transactions automatically rolled back
     *        due to timeout since the manager was created.
     */
    uint64_t getTimeoutCount() const {
        return total_timed_out_.load(std::memory_order_relaxed);
    }
    
    // Statistics
    struct Stats {
        uint64_t total_begun;
        uint64_t total_committed;
        uint64_t total_aborted;
        uint64_t total_timed_out;  ///< Transactions rolled back due to timeout
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

    // ── Transaction Timeout / Auto-Rollback ───────────────────────────────────

    /**
     * @brief Set a global timeout for all new and active transactions.
     *
     * Any transaction that has been active for longer than @p timeout_ms
     * without committing will be automatically rolled back by the background
     * detector thread.
     *
     * Set to 0 (default) to disable automatic rollback on timeout.
     *
     * @param timeout_ms  Maximum transaction lifetime in milliseconds; 0 = disabled.
     */
    void setTransactionTimeout(std::chrono::milliseconds timeout_ms);

    /**
     * @brief Return the currently configured transaction timeout.
     * @return Configured timeout in milliseconds (0 = disabled).
     */
    std::chrono::milliseconds getTransactionTimeout() const;

    /**
     * @brief Return the number of transactions rolled back due to timeout.
     */
    uint64_t getTimedOutCount() const;

    /**
     * @brief Abort any active transactions that have exceeded the configured timeout.
     *
     * Called automatically from the background detector thread.  Can also be
     * called manually (e.g. from a dedicated timeout-sweeper).
     *
     * @return Number of transactions aborted in this sweep.
     */
    size_t abortTimedOutTransactions();
    
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

    // ── Adaptive Deadlock Prevention (v1.9.0) ────────────────────────────────

    /**
     * @brief Attach an external DeadlockPredictor for adaptive prevention.
     *
     * When set, the TransactionManager will:
     *  - Call DeadlockPredictor::recordTransaction() each time a transaction
     *    commits or rolls back (so the predictor learns from history).
     *  - Call DeadlockPredictor::recordDeadlock() each time a deadlock cycle
     *    is resolved (so the predictor reinforces the cycle's key patterns).
     *
     * Ownership is *not* transferred; the caller must ensure the predictor
     * outlives this TransactionManager.
     *
     * Pass nullptr to detach a previously set predictor.
     */
    void setDeadlockPredictor(DeadlockPredictor* predictor);

    /**
     * @brief Return the currently attached DeadlockPredictor, or nullptr.
     */
    DeadlockPredictor* getDeadlockPredictor() const;

    /**
     * @brief Estimate the probability that acquiring @p proposed_locks will
     *        lead to a deadlock given the currently active transactions.
     *
     * Delegates to the attached DeadlockPredictor.  Returns 0.0 when no
     * predictor is attached or insufficient history exists.
     *
     * @param proposed_locks  Keys the caller intends to lock next.
     */
    double predictDeadlockProbability(
        const std::vector<std::string>& proposed_locks) const;

    /**
     * @brief Return the recommended lock-acquisition order for @p keys.
     *
     * Delegates to the attached DeadlockPredictor.  Returns @p keys sorted
     * lexicographically when no predictor is attached.
     *
     * @param keys  Keys that need to be locked.
     */
    std::vector<std::string> recommendLockOrder(
        const std::vector<std::string>& keys) const;

    /**
     * @brief Return the recommended transaction timeout for @p keys.
     *
     * Delegates to the attached DeadlockPredictor.  Returns the current
     * deadlock-detection timeout (as configured by setDeadlockTimeout()) when
     * no predictor is attached.
     *
     * @param keys  Keys that the transaction will lock.
     */
    std::chrono::milliseconds recommendTimeout(
        const std::vector<std::string>& keys) const;

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

    // ── History & Conflict tracking ──────────────────────────────────────────

    /**
     * @brief Enable atomic history tracking for all new transactions.
     *
     * Once set, every putEntity() and eraseEntity() call will also write an
     * immutable history record within the same RocksDB transaction.
     *
     * @param mgr Non-owning pointer to a HistoryManager backed by the same DB.
     *            Must outlive this TransactionManager.  Pass nullptr to disable.
     */
    void setHistoryManager(HistoryManager* mgr) { history_mgr_ = mgr; }

    /**
     * @brief Enable conflict artifact persistence for all new transactions.
     *
     * Once set, any transaction commit failure that is due to a write-write
     * conflict will result in a ConflictRecord being written and the returned
     * Status will carry a non-empty conflict_id.
     *
     * @param mgr Non-owning pointer to a ConflictManager backed by the same DB.
     *            Must outlive this TransactionManager.  Pass nullptr to disable.
     */
    void setConflictManager(ConflictManager* mgr) { conflict_mgr_ = mgr; }

    // ── Time-travel queries ───────────────────────────────────────────────────

    /**
     * @brief Result of a time-travel query against the entity history log.
     *
     * Wraps a raw HistoryRecord with convenience accessors suited for
     * higher-level callers who do not want to work with HistoryRecord directly.
     */
    struct TimeTravelRecord {
        std::string base_key;          ///< Live storage key (e.g. "entity:users:u1")
        HLCTimestamp timestamp;        ///< HLC timestamp of this version
        std::string op;                ///< "put" (value present) or "del" (tombstone)
        std::vector<uint8_t> value;    ///< Serialized entity bytes; empty for "del"
        uint64_t txn_id{0};           ///< Transaction that produced this version
    };

    /**
     * @brief Set the SnapshotManager used to resolve named snapshot tags to
     *        HLC timestamps for time-travel queries.
     *
     * Optional: when set, readEntityAtSnapshot() can look up the wall-clock
     * timestamp stored inside a named tag and use it to perform a time-travel
     * read.  The pointer must outlive this TransactionManager.  Pass nullptr
     * to disable snapshot-based time-travel.
     *
     * @param mgr Non-owning pointer to a SnapshotManager.  May be nullptr.
     */
    void setSnapshotManager(transaction::SnapshotManager* mgr) {
        snapshot_mgr_ = mgr;
    }

    /**
     * @brief Read the state of an entity as it existed at a given HLC timestamp.
     *
     * Queries the immutable history log written by putEntity() / eraseEntity()
     * and returns the most-recent version at or before @p ts.
     *
     * Requires that a HistoryManager has been set via setHistoryManager().
     *
     * @param table  Table name.
     * @param pk     Primary key.
     * @param ts     HLC timestamp upper bound (inclusive).
     * @return       TimeTravelRecord at or before @p ts, or std::nullopt when:
     *               - no HistoryManager is configured,
     *               - the entity has no history at or before @p ts, or
     *               - the entity has never been written.
     */
    std::optional<TimeTravelRecord> readEntityAtTimestamp(
        std::string_view table,
        std::string_view pk,
        HLCTimestamp ts) const;

    /**
     * @brief Read the state of an entity as it existed at a Unix wall-clock
     *        timestamp (milliseconds since epoch).
     *
     * Converts @p unix_ms to the corresponding HLC timestamp upper bound
     * (physical component = unix_ms, logical component = MAX_LOGICAL) and
     * delegates to readEntityAtTimestamp().
     *
     * @param table    Table name.
     * @param pk       Primary key.
     * @param unix_ms  Unix timestamp in milliseconds.
     * @return         TimeTravelRecord at or before the given wall-clock time,
     *                 or std::nullopt when no matching history entry is found.
     */
    std::optional<TimeTravelRecord> readEntityAtUnixMs(
        std::string_view table,
        std::string_view pk,
        int64_t unix_ms) const;

    /**
     * @brief Read the state of an entity as it existed at a named snapshot tag.
     *
     * Looks up the wall-clock timestamp stored in the named tag via the
     * configured SnapshotManager and then delegates to readEntityAtUnixMs().
     *
     * Requires both a HistoryManager (setHistoryManager()) and a
     * SnapshotManager (setSnapshotManager()) to be configured.
     *
     * @param table     Table name.
     * @param pk        Primary key.
     * @param tag_name  Named snapshot tag created via SnapshotManager::createTag().
     * @return          TimeTravelRecord as of the snapshot, or std::nullopt when:
     *                  - no SnapshotManager is configured,
     *                  - the tag does not exist, or
     *                  - no history entry exists at or before the tag timestamp.
     */
    std::optional<TimeTravelRecord> readEntityAtSnapshot(
        std::string_view table,
        std::string_view pk,
        const std::string& tag_name) const;

    /**
     * @brief List all historical versions of an entity, oldest first.
     *
     * Returns the complete version chain recorded in the immutable history log.
     * Each entry corresponds to one putEntity() or eraseEntity() call that was
     * committed.
     *
     * Requires that a HistoryManager has been set via setHistoryManager().
     *
     * @param table  Table name.
     * @param pk     Primary key.
     * @return       All TimeTravelRecords in chronological order (empty when
     *               no HistoryManager is set or the entity has no history).
     */
    std::vector<TimeTravelRecord> listEntityVersions(
        std::string_view table,
        std::string_view pk) const;

    // ── Serializable Snapshot Isolation (SSI) configuration ──────────────────

    /**
     * @brief Tuning parameters for Serializable Snapshot Isolation (SSI).
     *
     * These settings control the predicate-lock subsystem used by
     * SERIALIZABLE transactions.  Adjust them to balance memory usage, false-
     * positive abort rate, and conflict-detection latency.
     */
    struct SSIConfig {
        /// Enable or disable predicate lock tracking.  When false, SERIALIZABLE
        /// transactions behave identically to REPEATABLE_READ (snapshot isolation
        /// only; write-skew anomalies are not detected).
        bool enable_predicate_locking = true;

        /// Maximum total number of predicate locks that may be held
        /// simultaneously across all active transactions.  Once the limit is
        /// reached, new acquirePredicateLock() calls are silently dropped,
        /// which may increase the false-positive abort rate.
        size_t max_predicate_locks = 10000;

        /// How often the background conflict-detection sweep (if any) is
        /// triggered.  Currently informational; no background sweep is
        /// implemented – conflict detection is performed inline at write time.
        std::chrono::milliseconds conflict_detection_interval{100};
    };

    /**
     * @brief Update the SSI configuration.
     *
     * Thread-safe.  New values take effect immediately for all subsequent
     * predicate-lock operations; existing in-flight locks are unaffected.
     *
     * @param config  New SSI tuning parameters.
     */
    void setSSIConfig(const SSIConfig& config);

    /**
     * @brief Return the currently active SSI configuration.
     */
    SSIConfig getSSIConfig() const;

    /**
     * @brief Describes a single read-write or write-write serialization
     *        conflict detected for a SERIALIZABLE transaction.
     */
    struct SerializationConflict {
        /// The transaction ID of the other transaction involved in the conflict.
        TransactionId other_txn_id{0};

        /// The storage key that triggered the conflict.
        std::string key;

        /// Human-readable description of the conflict kind.
        ///  "read-write"  – this transaction's read range overlaps a write by
        ///                  @p other_txn_id (phantom / write-skew risk).
        ///  "write-write" – both transactions wrote the same key concurrently
        ///                  (lost-update risk).
        std::string conflict_type;

        /// Human-readable explanation.
        std::string message;
    };

    /**
     * @brief Enumerate predicate-lock conflicts for a SERIALIZABLE transaction.
     *
     * Scans every predicate lock held by @p txn_id against the predicate locks
     * held by all other active SERIALIZABLE transactions and returns one
     * SerializationConflict entry for each key range that would produce a
     * serialization failure.
     *
     * Returns an empty vector for non-SERIALIZABLE transactions or when
     * predicate locking is disabled.
     *
     * @param txn_id  Transaction to analyse.
     * @return        List of detected conflicts; empty when none exist.
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
    
    TransactionId generateTransactionId();
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

    /// Count active transactions for a tenant without acquiring sessions_mutex_.
    /// Must be called with sessions_mutex_ already held.
    size_t countActiveTenantTransactionsLocked(std::string_view tenant_id) const;

    // Transaction timeout
    std::atomic<uint64_t> default_transaction_timeout_ms_{0}; ///< 0 = no default timeout
    void timeoutExpiredTransactions(); ///< roll back active transactions that exceeded their timeout
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
    
    void deadlockDetectorLoop();
    bool detectDeadlockCycle(std::vector<TransactionId>& cycle);
    void resolveDeadlock(const std::vector<TransactionId>& cycle);
    
    // Lock tracking helpers called by transactions
    void trackLockAcquired(TransactionId txn_id, const std::string& key);
    void trackLockReleased(TransactionId txn_id, const std::string& key);
    void trackLockWaiting(TransactionId txn_id, const std::string& key);
    void clearWaiting(TransactionId txn_id);

    /// Optional non-owning pointers – set to enable history/conflict tracking.
    HistoryManager*  history_mgr_{nullptr};
    ConflictManager* conflict_mgr_{nullptr};
    /// Optional SnapshotManager for resolving named tags in time-travel queries.
    transaction::SnapshotManager* snapshot_mgr_{nullptr};
    /// Optional non-owning pointer to the adaptive deadlock predictor (v1.9.0).
    /// Stored atomically so setDeadlockPredictor() can be called concurrently
    /// with predict/recommend helpers without introducing a data race.
    std::atomic<DeadlockPredictor*> deadlock_predictor_{nullptr};

    // SSI configuration – protected by ssi_config_mutex_
    mutable std::mutex ssi_config_mutex_;
    SSIConfig ssi_config_;
};

} // namespace themis

