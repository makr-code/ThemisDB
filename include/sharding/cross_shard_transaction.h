/**
 * @file cross_shard_transaction.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/cross_shard_fk_validator.h"
#include "sharding/cross_shard_ssi_manager.h"
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include "sharding/wal_manager.h"  // For LSN type
#include "transaction/recoverable_two_phase_coordinator.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace sharding {
enum class TransactionProtocol;
enum class TransactionWALEntryType;
struct TransactionWALEntry;
struct TransactionWALConfig;
class TransactionWAL;
class TransactionSnapshotManager;
}

namespace themisdb {
namespace sharding {

// Forward declarations for Phase 2.3.3 integration
using LSN = themis::sharding::LSN;

using TransactionWAL = ::sharding::TransactionWAL;
using TransactionWALConfig = ::sharding::TransactionWALConfig;
using TransactionWALEntryType = ::sharding::TransactionWALEntryType;
using TransactionWALEntry = ::sharding::TransactionWALEntry;
using TransactionSnapshotManager = ::sharding::TransactionSnapshotManager;

/**
 * @brief Transaction protocol type
 */
enum class TransactionProtocol {
    TWO_PHASE_COMMIT,   ///< 2PC: blocking but strongly consistent.
    THREE_PHASE_COMMIT, ///< 3PC: non-blocking variant with pre-commit stage.
    SAGA,               ///< SAGA with compensating actions.
    PERCOLATOR,         ///< Percolator optimistic protocol for distributed writes.
    CALVIN              ///< Calvin deterministic protocol via pre-ordering.
};

/**
 * @brief Transaction isolation level
 */
enum class IsolationLevel {
    READ_UNCOMMITTED,   ///< Dirty reads allowed.
    READ_COMMITTED,     ///< Reads only committed data.
    REPEATABLE_READ,    ///< Repeatable reads within a transaction.
    SNAPSHOT_ISOLATION, ///< MVCC snapshot isolation.
    SERIALIZABLE        ///< Fully serializable execution.
};

/**
 * @brief Transaction state
 */
enum class TransactionState {
    ACTIVE,      ///< Transaction is active.
    PREPARING,   ///< Phase 1 prepare in progress.
    PREPARED,    ///< All participants prepared.
    COMMITTING,  ///< Commit decision being applied.
    COMMITTED,   ///< Commit completed successfully.
    ABORTING,    ///< Abort decision being applied.
    ABORTED,     ///< Abort completed.
    UNKNOWN      ///< State unknown, usually after coordinator failure.
};

enum class DeadlockVictimPolicy {
    YOUNGEST,  ///< Abort most recently started transaction in cycle.
    OLDEST,    ///< Abort longest-running transaction in cycle.
    RANDOM     ///< Abort random transaction in cycle.
};

/**
 * @brief Shard participant in a transaction
 */
struct ShardParticipant {
    std::string shard_id;                 ///< Shard identifier.
    std::string endpoint;                 ///< Participant RPC endpoint.
    std::vector<std::string> operations;  ///< Serialized operations for shard.
    bool prepared = false;                ///< True when prepare acknowledged.
    bool precommitted = false;            ///< True when 3PC pre-commit acknowledged.
    bool committed = false;               ///< True when commit acknowledged.
    bool aborted = false;                 ///< True when abort acknowledged.
    std::string error_message;            ///< Participant-specific failure detail.
};

/**
 * @brief Cross-shard transaction metadata
 */
struct CrossShardTransaction {
    std::string transaction_id;              ///< Global transaction ID.
    TransactionProtocol protocol;            ///< Selected transaction protocol.
    IsolationLevel isolation_level;          ///< Requested isolation level.
    TransactionState state;                  ///< Current transaction state.
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::map<std::string, ShardParticipant> participants;  ///< Participants keyed by shard ID.
    nlohmann::json metadata;                 ///< Additional protocol metadata.
    
    // MVCC timestamps for snapshot isolation
    int64_t snapshot_timestamp = 0;          ///< Snapshot/read timestamp at begin.
    int64_t commit_timestamp = 0;            ///< Commit timestamp assigned on decision.
    
    // Compensation data (for SAGA)
    std::map<std::string, nlohmann::json> compensations; ///< Compensation payloads for SAGA.
    
    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"transaction_id", transaction_id},
            {"protocol", static_cast<int>(protocol)},
            {"isolation_level", static_cast<int>(isolation_level)},
            {"state", static_cast<int>(state)},
            {"start_time", std::chrono::duration_cast<std::chrono::milliseconds>(
                start_time.time_since_epoch()).count()},
            {"metadata", metadata}
        };
        
        if (state == TransactionState::COMMITTED || state == TransactionState::ABORTED) {
            j["end_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time.time_since_epoch()).count();
        }
        
        nlohmann::json participants_json = nlohmann::json::array();
        for (const auto& [shard_id, participant] : participants) {
            participants_json.push_back({
                {"shard_id", participant.shard_id},
                {"prepared", participant.prepared},
                {"committed", participant.committed},
                {"aborted", participant.aborted}
            });
        }
        j["participants"] = participants_json;
        
        return j;
    }
};

/**
 * @brief Configuration for cross-shard transactions
 */
struct CrossShardTransactionConfig {
    struct PolledWaitForEdge {
        std::string waiting_transaction_id;
        std::string blocking_transaction_id;
    };

    TransactionProtocol default_protocol = TransactionProtocol::TWO_PHASE_COMMIT; ///< Default protocol.
    IsolationLevel default_isolation = IsolationLevel::SNAPSHOT_ISOLATION;        ///< Default isolation.
    
    // 2PC/3PC settings
    std::chrono::milliseconds prepare_timeout{5000}; ///< Prepare-phase timeout.
    std::chrono::milliseconds commit_timeout{5000};  ///< Commit-phase timeout.
    std::chrono::milliseconds abort_timeout{5000};   ///< Abort-phase timeout.
    
    // SAGA settings
    bool saga_enable_compensation = true;             ///< Enable SAGA compensation flow.
    std::chrono::milliseconds saga_step_timeout{10000}; ///< Timeout per SAGA step.
    
    // Percolator settings
    std::chrono::milliseconds percolator_lock_timeout{1000}; ///< Percolator lock timeout.
    uint32_t percolator_max_retries = 3;                     ///< Percolator retry count.
    
    // Generic lock timeout (used by multiple protocols)
    std::chrono::milliseconds lock_timeout{5000}; ///< Generic coordinator mutex timeout.
    
    // Calvin settings
    std::chrono::milliseconds calvin_epoch_duration{10}; ///< Calvin epoch batch window.
    bool calvin_enable_deterministic_lock_order = true; ///< Sort locks by key for deadlock-free acquisition.
    
    // Deadlock detection
    bool enable_deadlock_detection = true;                           ///< Enable deadlock detector thread.
    std::chrono::milliseconds deadlock_detection_interval{1000};     ///< Deadlock scan interval.
    DeadlockVictimPolicy deadlock_victim_policy = DeadlockVictimPolicy::YOUNGEST; ///< Victim choice policy.
    
    // Transaction timeout
    std::chrono::milliseconds transaction_timeout{30000}; ///< Global transaction timeout.
    
    // Transaction log path (defaults to /var/lib/themisdb/transaction_log.jsonl)
    std::string transaction_log_path = "/var/lib/themisdb/transaction_log.jsonl"; ///< Absolute transaction log path.
    
    // Persistence settings (Phase 2.3.3 - Transaction Durability)
    bool enable_persistence = false; ///< Enable WAL/snapshot persistence backend.
    std::string data_dir;            ///< Base directory for WAL and snapshots.
    uint64_t snapshot_interval = 1000; ///< Snapshot cadence in operation count.
    size_t max_snapshots = 10;       ///< Maximum retained snapshots.

    // Coordinator identity — set to DistributedCoordinator::getLocalShardId()
    // before constructing CrossShardTransactionCoordinator so that audit records
    // and snapshots carry the real node ID instead of a placeholder.
    std::string coordinator_id; ///< Actual coordinator node identifier.

    // Cluster-wide deadlock detection — shard endpoints to poll.
    // Map of shard_id -> gRPC endpoint (e.g. "shard1" -> "shard1:50051").
    // When non-empty, deadlockDetectionThread polls every endpoint once per
    // deadlock_detection_interval to collect their local wait-for edges and
    // merge them into the cluster-wide graph alongside any edges explicitly
    // reported via reportDistributedWait().
    std::map<std::string, std::string> shard_endpoints; ///< Shard ID to endpoint map for wait-edge polling.

    // Optional override used to collect per-shard wait-for edges during
    // deadlock detection. Primarily intended for deterministic tests and
    // custom deployments that do not use ShardRPCClient polling.
    std::function<std::vector<PolledWaitForEdge>(
        const std::string& shard_id,
        const std::string& endpoint
    )> polled_wait_for_edge_collector;

    // ── Distributed Serializable Snapshot Isolation (SSI) ────────────────────

    /// Configuration for cross-shard SSI predicate-lock tracking and
    /// conflict detection.  Applies only to transactions with isolation level
    /// SERIALIZABLE.  Inactive (no-op) for all other isolation levels.
    CrossShardSSIManager::Config ssi_config;
};

struct BackendRecoveryStats {
    uint64_t pending_transactions = 0;
    uint64_t snapshot_transactions_restored = 0;
    uint64_t wal_entries_replayed = 0;
    uint64_t stale_transactions_detected = 0;
    uint64_t resume_candidates = 0;
    uint64_t failed_operations = 0;
    uint64_t in_doubt_transactions = 0;
};

/**
 * @brief Enhanced Cross-Shard Transaction Coordinator
 * 
 * Provides pluggable transaction protocols for distributed transactions
 * across multiple shards. Supports:
 * - 2PC/3PC for strong consistency
 * - SAGA for long-running transactions with compensation
 * - Percolator for optimistic concurrency
 * - Distributed deadlock detection
 * - Snapshot isolation across shards
 */
class CrossShardTransactionCoordinator
    : public themis::transaction::IRecoverableTwoPhaseCoordinator {
public:
    /**
     * @brief Construct cross-shard coordinator with optional TrueTime source.
     * @param config Coordinator behavior and timeout settings.
     * @param consensus Consensus module for replicated coordinator decisions.
     * @param truetime Optional TrueTime provider; created lazily when null.
     */
    explicit CrossShardTransactionCoordinator(
        const CrossShardTransactionConfig& config,
        std::shared_ptr<ConsensusModule> consensus,
        std::shared_ptr<themis::sharding::TrueTime> truetime = nullptr
    );
    
    /** @brief Stop background worker threads and release resources. */
    ~CrossShardTransactionCoordinator();
    
    /**
     * @brief Initialize the coordinator
     */
    bool initialize();
    
    /**
     * @brief Start the coordinator
     */
    bool start();
    
    /**
     * @brief Stop the coordinator
     */
    void stop();
    
    /**
     * @brief Begin a new cross-shard transaction
     * @param transaction_id Unique transaction identifier
     * @param protocol Transaction protocol to use
     * @param isolation_level Isolation level
     * @return true if transaction started successfully
     */
    bool beginTransaction(
        const std::string& transaction_id,
        TransactionProtocol protocol = TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel isolation_level = IsolationLevel::SNAPSHOT_ISOLATION
    );
    
    /**
     * @brief Add a shard participant to the transaction
     * @param transaction_id Transaction ID
     * @param shard_id Shard to add
     * @param endpoint Shard endpoint
     * @param operations Operations to perform on this shard
     * @return true if added successfully
     */
    bool addParticipant(
        const std::string& transaction_id,
        const std::string& shard_id,
        const std::string& endpoint,
        const std::vector<std::string>& operations
    );
    
    /**
     * @brief Prepare transaction (2PC/3PC phase 1)
     * @param transaction_id Transaction ID
     * @return true if all participants prepared successfully
     */
    bool prepare(const std::string& transaction_id);
    
    /**
     * @brief Commit transaction
     * @param transaction_id Transaction ID
     * @return true if committed successfully
     */
    bool commit(const std::string& transaction_id);
    
    /**
     * @brief Abort transaction
     * @param transaction_id Transaction ID
     * @return true if aborted successfully
     */
    bool abort(const std::string& transaction_id);

    /**
     * @brief Re-drive in-doubt transactions using the configured recovery backend.
     *
      * Uses WAL+snapshot recovery when the backend is available.
     *
     * This method is intended for startup recovery before new transactions are
     * accepted. If called during live traffic, the returned value is best-effort.
     *
     * @return Number of in-doubt transactions resolved by this invocation.
     */
    [[nodiscard]] size_t recoverInDoubtTransactions() override;

    /**
     * @brief Return the canonical coordinator name for global recovery reports.
     * @return Stable coordinator type name.
     */
    [[nodiscard]] std::string recoveryCoordinatorName() const override;

    /**
     * @brief Return the durable backend used by this coordinator.
     * @return "WAL/snapshot" when persistence is enabled, otherwise "disabled".
     */
    [[nodiscard]] std::string recoveryBackendName() const override;

    /**
     * @brief Snapshot current in-doubt transactions using the shared state model.
     * @return Normalized non-final transaction list for global recovery orchestration.
     */
    [[nodiscard]] std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;
    
    /**
     * @brief Execute a SAGA transaction
     * @param transaction_id Transaction ID
     * @param steps SAGA steps to execute
     * @param compensations Compensation actions for each step
     * @return true if SAGA completed successfully
     */
    bool executeSaga(
        const std::string& transaction_id,
        const std::vector<nlohmann::json>& steps,
        const std::vector<nlohmann::json>& compensations
    );
    
    /**
     * @brief Get transaction state
     * @param transaction_id Transaction ID
     * @return Transaction state or nullopt if not found
     */
    std::optional<TransactionState> getTransactionState(
        const std::string& transaction_id
    ) const;
    
    /**
     * @brief Get transaction details
     * @param transaction_id Transaction ID
     * @return Transaction details or nullopt if not found
     */
    std::optional<CrossShardTransaction> getTransaction(
        const std::string& transaction_id
    ) const;
    
    /**
     * @brief Check if transaction is deadlocked
     * @param transaction_id Transaction ID
     * @return true if deadlocked
     */
    bool isDeadlocked(const std::string& transaction_id) const;

    /**
     * @brief Report a distributed wait edge observed on a shard.
     *
     * Records that @p waiting_transaction_id is waiting for
     * @p blocking_transaction_id on @p shard_id so the coordinator can build
     * a cross-shard wait-for graph for deadlock detection.
     */
    void reportDistributedWait(
        const std::string& waiting_transaction_id,
        const std::string& blocking_transaction_id,
        const std::string& shard_id
    );

    /**
     * @brief Clear all distributed wait edges for a transaction.
     */
    void clearDistributedWaits(const std::string& transaction_id);

    // ── Distributed SSI API ──────────────────────────────────────────────────

    /**
     * @brief Register predicate (range) locks observed on @p shard_id for a
     *        SERIALIZABLE transaction during its read phase.
     *
     * Must be called by the shard participant (or its proxy) before the
     * transaction enters the prepare phase so that the coordinator has the
     * full cross-shard read-set available for conflict detection.
     *
     * No-op when SSI is disabled or when @p transaction_id does not denote
     * an active SERIALIZABLE transaction.
     *
     * Thread-safe.
     *
     * @param transaction_id  Global transaction identifier.
     * @param shard_id        Shard that performed the range scan.
     * @param predicates      Key ranges observed on @p shard_id.
     */
    void registerShardReadSet(
        const std::string& transaction_id,
        const std::string& shard_id,
        const std::vector<CrossShardPredicateLock>& predicates);

    /**
     * @brief Register the keys written on @p shard_id by a SERIALIZABLE
     *        transaction.
     *
     * Must be called by the shard participant (or its proxy) before the
     * transaction enters the prepare phase.
     *
     * No-op when SSI is disabled.
     *
     * Thread-safe.
     *
     * @param transaction_id  Global transaction identifier.
     * @param shard_id        Shard that performed the writes.
     * @param keys            Keys written on @p shard_id.
     */
    void registerShardWriteSet(
        const std::string& transaction_id,
        const std::string& shard_id,
        const std::vector<std::string>& keys);

    /**
     * @brief Perform cross-shard SSI conflict detection for @p transaction_id.
     *
     * Called internally by prepare() for SERIALIZABLE transactions.  May also
     * be called explicitly by callers that manage prepare/commit life-cycles
     * outside the coordinator (e.g. testing harnesses).
     *
     * Returns an empty vector when no conflict is found.  A non-empty result
     * means the transaction must be aborted.
     *
     * Thread-safe.
     *
     * @param transaction_id  Transaction to validate.
     * @return                All detected serialization conflicts.
     */
    [[nodiscard]] std::vector<CrossShardSSIConflict>
    validateCrossShardSSI(const std::string& transaction_id) const;

    /**
     * @brief Update the SSI configuration applied to all future transactions.
     *
     * Thread-safe.  New values take effect immediately for subsequent
     * registerShardReadSet() / validateCrossShardSSI() calls.
     *
     * @param config  New SSI tuning parameters.
     */
    void setSSIConfig(const CrossShardSSIManager::Config& config);

    /**
     * @brief Return the currently active SSI configuration.
     * Thread-safe.
     */
    [[nodiscard]] CrossShardSSIManager::Config getSSIConfig() const;
    
    /**
     * @brief Get active transactions
     */
    std::vector<CrossShardTransaction> getActiveTransactions() const;
    
    /**
     * @brief Get transaction statistics
     */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Register callback for transaction state changes
     */
    void onTransactionStateChange(
        std::function<void(const std::string&, TransactionState, TransactionState)> callback
    );

    /**
     * @brief Inject a PreCommit RPC callback for the 3PC protocol.
     *
     * In the 3PC protocol, Phase 2 must instruct every participant to durably
     * persist its prepared state (PreCommit) before the coordinator proceeds to
      * Phase 3 (Commit). Without this RPC, 3PC execution fails closed.
     *
     * When @p fn is non-null, execute3PC() calls it for each participant in
     * Phase 2 and aborts the transaction if any participant rejects.  This
     * activates the full 3PC non-blocking guarantee.
     *
      * Pass @c nullptr to remove the callback. Any later attempt to execute
      * 3PC without a callback fails closed and aborts the transaction.
     *
     * **Exception safety**: The callback must not throw.  If it does, the
     * exception is caught by execute3PC(), treated as a NACK (i.e. the
     * participant's PreCommit is counted as failed), and the transaction is
     * aborted.
     *
     * @param fn  Callable @c bool(shard_id, txn_id) that sends the PreCommit
     *            RPC to the given shard; returns true on acknowledgement,
     *            false on NACK; must not throw.
     */
    using PreCommitRpcFn =
        std::function<bool(const std::string& /*shard_id*/,
                           const std::string& /*txn_id*/)>;

    using DeferredPreCommitFn =
        std::function<void(const std::string& /*txn_id*/,
                           const std::vector<std::string>& /*failed_shards*/)>;

    void setPreCommitCallback(PreCommitRpcFn fn);

    /**
     * @brief Set callback for deferred PreCommit retry (3PC non-blocking mode).
     *
     * When set, failed PreCommit operations do not immediately abort the
     * transaction. Instead, execute3PC() hands the failed shard list to this
     * callback for deferred retry orchestration.
     *
     * **Exception safety / contract**:
     * - The callback must not throw.
     * - If it throws, execute3PC() treats this as a fatal orchestration failure
     *   and fails closed (abort path) instead of silently downgrading behavior.
     *
     * @param fn  Callable @c void(txn_id, failed_shards) that schedules retry
     *            handling for failed PreCommit participants; must not throw.
     */
    void setDeferredPreCommitCallback(DeferredPreCommitFn fn);

    /**
     * @brief Inject a cross-shard foreign-key validator (issue #5392).
     *
     * When a non-null validator is set, prepare() invokes
     * CrossShardForeignKeyValidator::validate() **before** dispatching the
     * prepare RPCs to participants.  Any non-deferrable FK violation causes
     * prepare() to return false (transaction aborted).
     *
     * Passing @c nullptr removes a previously installed validator.  The
     * coordinator takes shared ownership of the validator so it can be shared
     * across coordinator instances in tests.
     *
     * ### Thread safety
     * The method is protected by the callbacks_mutex_ and is safe to call
     * concurrently with ongoing transactions; the validator pointer is
     * snapshotted at the start of each prepare() call.
     *
     * @param validator  Fully configured CrossShardForeignKeyValidator, or
     *                   nullptr to disable cross-shard FK checking.
     */
    void setForeignKeyValidator(
        std::shared_ptr<CrossShardForeignKeyValidator> validator
    );

private:
    /**
     * @brief Execute 2PC protocol
     */
    bool execute2PC(CrossShardTransaction& txn);
    
    /**
     * @brief Execute 3PC protocol
     */
    bool execute3PC(CrossShardTransaction& txn);
    
    /**
     * @brief Execute Percolator protocol
     */
    bool executePercolator(CrossShardTransaction& txn);
    
    /**
     * @brief Execute Calvin deterministic protocol
     * 
     * Implements the Calvin deterministic database protocol:
     * - Phase 1 (Sequencing): Assign a deterministic global sequence number
     * - Phase 2 (Lock Acquisition): Pre-acquire all locks in a canonical order
     * - Phase 3 (Execution): Execute and commit in the pre-determined order
     * 
     * Unlike 2PC, Calvin does not require a voting round; all participants
     * execute the same pre-ordered transaction log, guaranteeing determinism.
     */
    bool executeCalvin(CrossShardTransaction& txn);
    
    /**
     * @brief Send prepare request to shard
     */
    bool sendPrepare(const std::string& shard_id, const std::string& transaction_id);
    
    /**
     * @brief Send commit request to shard
     */
    bool sendCommit(const std::string& shard_id, const std::string& transaction_id);
    
    /**
     * @brief Send abort request to shard
     */
    bool sendAbort(const std::string& shard_id, const std::string& transaction_id);
    
    /**
     * @brief Deadlock detection thread
     */
    void deadlockDetectionThread();

    /**
     * @brief Background worker that retries deferred 3PC pre-commit RPCs.
     */
    void preCommitRetryThread();
    
    /**
     * @brief Build wait-for graph for deadlock detection
     */
    std::map<std::string, std::vector<std::string>> buildWaitForGraph() const;
    
    /**
     * @brief Detect cycles in wait-for graph
     */
    bool detectCycle(
        const std::map<std::string, std::vector<std::string>>& graph,
        const std::string& start_node,
        std::set<std::string>& visited,
        std::set<std::string>& rec_stack
    ) const;

    /**
     * @brief Remove all outgoing and incoming distributed wait edges
     * for a finished transaction.
     *
     * Caller must hold transactions_mutex_.
     */
    void clearDistributedWaitEdgesLocked(const std::string& transaction_id);
    
    /**
     * @brief Execute compensations for SAGA transaction
     */
    void executeCompensations(
        const std::string& transaction_id,
        const std::vector<nlohmann::json>& executed_steps,
        const std::vector<nlohmann::json>& compensations
    );
    
    /**
     * @brief Generate MVCC commit timestamp ensuring external consistency
     * @param txn Transaction to generate timestamp for
     * @return Commit timestamp that is definitely after snapshot timestamp
     */
    int64_t generateCommitTimestamp(const CrossShardTransaction& txn);
    
    /**
     * @brief Persist transaction state to durable storage
     */
    bool persistTransactionState(
        const std::string& transaction_id,
        TransactionState state
    );

    /**
     * @brief Acquire/release the exclusive terminal-decision guard for a transaction.
     *
     * Prevents concurrent commit()/abort() callers from driving conflicting final
     * decisions for the same transaction at the same time.
     */
    bool tryStartTerminalDecision(const std::string& transaction_id);
    void finishTerminalDecision(const std::string& transaction_id);
    
    /**
     * @brief Recover from WAL and snapshot (Phase 2.3.3)
     */
    bool recoverFromWAL(BackendRecoveryStats* stats = nullptr);

    struct RecoveryRunResult {
        bool ok = false;
        bool backend_available = false;
        const char* backend = "WAL/snapshot";
        uint64_t elapsed_ms = 0;
        BackendRecoveryStats details;
    };

    /**
     * @brief Run WAL/snapshot recovery and emit consistent telemetry.
     * @param context Caller context used in diagnostics (e.g. "initialize").
     * @return Structured recovery result for caller-side handling.
     */
    RecoveryRunResult runRecoveryBackend(const char* context);
    
    /**
     * @brief Create periodic snapshot of active transactions (Phase 2.3.3)
     */
    void createPeriodicSnapshot();
    
    CrossShardTransactionConfig config_; ///< Runtime coordinator configuration.
    std::shared_ptr<ConsensusModule> consensus_; ///< Consensus dependency for replicated decisions.
    std::shared_ptr<themis::sharding::TrueTime> truetime_; ///< Time source for MVCC/external consistency.

    /// Distributed SSI manager for cross-shard predicate-lock tracking and
    /// conflict detection.  Active only for SERIALIZABLE transactions.
    CrossShardSSIManager ssi_manager_;
    
    // Transaction log file
    std::string transaction_log_path_; ///< Absolute transaction log file path.
    
    // Phase 2.3.3: WAL and Snapshot infrastructure
    std::unique_ptr<TransactionWAL> transaction_wal_; ///< WAL backend for durability/recovery.
    std::unique_ptr<TransactionSnapshotManager> snapshot_manager_; ///< Snapshot backend for recovery acceleration.
    std::atomic<uint64_t> operations_since_snapshot_{0}; ///< Operation count since last snapshot.
    LSN last_applied_lsn_{0, 0}; ///< Last WAL LSN applied during recovery.
    
    // State
    mutable std::timed_mutex transactions_mutex_; ///< Protects transaction map and wait-edge graph.
    std::map<std::string, CrossShardTransaction> transactions_; ///< Active transaction registry.
    std::map<std::string, std::set<std::string>> distributed_wait_for_edges_; ///< Cross-shard wait-for edges.
    mutable std::mutex decision_mutex_; ///< Protects terminal decision guard set.
    std::set<std::string> terminal_decisions_in_progress_; ///< Transactions currently in commit/abort finalization.
    
    // Callbacks
    // ======================================================================
    // DEADLOCK PREVENTION: Canonical Lock Acquisition Order
    // ======================================================================
    // To prevent circular wait deadlocks, all code MUST acquire locks in
    // this strict order when multiple locks are needed:
    //   1. transactions_mutex_ (transaction state and wait-edge graph)
    //   2. decision_mutex_ (terminal decision guard set)
    //   3. callbacks_mutex_ (state change and RPC callbacks)
    //   4. deferred_mutex_ (deferred PreCommit tracking)
    // 
    // CRITICAL RULES:
    // - NEVER acquire in reverse order
    // - NEVER hold multiple locks across function calls or async operations
    // - ALWAYS unlock transactions_mutex_ before waiting on futures/futures
    // - Use lock.unlock() explicitly before RPC calls or thread operations
    // - If you need multiple locks, comment the reason and the order used
    // ======================================================================
    mutable std::mutex callbacks_mutex_;
    std::function<void(const std::string&, TransactionState, TransactionState)> 
        on_state_change_callback_; ///< Optional state transition callback.

    /// Injected 3PC PreCommit RPC callback (CST-6).
    /// Protected by callbacks_mutex_.
    /// Contract: must not throw; missing callback causes execute3PC() to fail closed.
    /// → See docs/architecture/transaction_coordinators.md §4.2 for the full callback contract.
    PreCommitRpcFn precommit_callback_; ///< Optional injected PreCommit RPC callback.

    DeferredPreCommitFn deferred_precommit_callback_;///< Optional callback for deferred PreCommit retry.

    /// Injected cross-shard FK validator (issue #5392).
    /// Protected by callbacks_mutex_.
    std::shared_ptr<CrossShardForeignKeyValidator> fk_validator_; ///< Optional FK constraint validator.
    
    // Deferred PreCommit tracking
    std::map<std::string, std::vector<std::string>> deferred_precommits_; ///< txn_id -> failed shards
    std::mutex deferred_mutex_;
    
    // Background thread
    std::atomic<bool> running_;         ///< Lifecycle flag for background workers.
    std::thread deadlock_detection_thread_; ///< Distributed deadlock detector thread.
    std::thread precommit_retry_thread_;   ///< Thread for retrying deferred PreCommits.
    
    // Statistics
    std::atomic<uint64_t> total_transactions_;     ///< Total started transactions.
    std::atomic<uint64_t> committed_transactions_; ///< Successfully committed transactions.
    std::atomic<uint64_t> aborted_transactions_;   ///< Aborted transactions.
    std::atomic<uint64_t> deadlocked_transactions_; ///< Deadlock victim count.
};

// ============================================================================
// PercolatorCoordinator
// ============================================================================

/**
 * @brief Standalone Percolator-style MVCC transaction coordinator.
 *
 * Implements the Google Percolator two-phase optimistic protocol for
 * cross-shard transactions that favour snapshot-isolated, read-heavy
 * workloads.  Key properties:
 *
 *  1. Primary-lock model: one row/shard acts as the transaction's "primary";
 *     secondary rows reference the primary lock.
 *  2. TrueTime commit-wait: commit timestamp is drawn from
 *     TrueTime::now_with_uncertainty().latest; the coordinator then waits
 *     until TT.now().earliest > commit_ts + max_uncertainty before sending
 *     the final COMMIT to participants.
 *  3. Coordinator-state durability: every phase transition is logged to the
 *     supplied TransactionWAL so that a replacement coordinator can resume
 *     in-flight transactions after a crash.
 *  4. Stale-lock cleanup: cleanStaleLocks() may be called by OrphanDetector
 *     to abort Percolator transactions that left locks behind after a
 *     coordinator failure.
 *
 * The class uses callbacks for the low-level shard operations so it can be
 * used both standalone (e.g. in tests) and wired into
 * CrossShardTransactionCoordinator::executePercolator().
 *
 * Usage example:
 * @code
 *   PercolatorCoordinator::Config cfg;
 *   cfg.lock_timeout   = std::chrono::milliseconds(500);
 *   cfg.max_retries    = 3;
 *
 *   PercolatorCoordinator perc(cfg, truetime_ptr, std::move(wal_ptr));
 *   bool ok = perc.execute(txn, prepare_fn, commit_fn, abort_fn);
 * @endcode
 */
class PercolatorCoordinator {
public:
    /**
     * @brief Runtime configuration.
     */
    struct Config {
        /// Per-lock acquisition timeout (milliseconds).
        std::chrono::milliseconds lock_timeout{1000};

        /// Maximum number of lock-acquisition retries per shard.
        uint32_t max_retries = 3;

        /// How long a lock can be held before it is considered stale.
        std::chrono::seconds stale_lock_threshold{30};
    };

    /// Callback type: send a PREPARE (lock) to one shard.
    using SendPrepareFn = std::function<bool(const std::string& shard_id,
                                             const std::string& txn_id)>;
    /// Callback type: send a COMMIT to one shard.
    using SendCommitFn  = std::function<bool(const std::string& shard_id,
                                             const std::string& txn_id)>;
    /// Callback type: send an ABORT to one shard.
    using SendAbortFn   = std::function<bool(const std::string& shard_id,
                                             const std::string& txn_id)>;

    /**
     * @brief Construct a PercolatorCoordinator.
     *
     * @param config     Runtime parameters.
     * @param truetime   TrueTime clock used for commit-timestamp generation
     *                   and commit-wait (may be nullptr; falls back to wall clock).
     * @param wal        Optional non-owning pointer to a WAL for coordinator-state
     *                   durability.  The WAL must outlive this object.
     */
    explicit PercolatorCoordinator(
        const Config& config,
        std::shared_ptr<themis::sharding::TrueTime> truetime = nullptr,
        TransactionWAL* wal = nullptr
    );

    ~PercolatorCoordinator() = default;

    // Non-copyable, movable.
    PercolatorCoordinator(const PercolatorCoordinator&) = delete;
    PercolatorCoordinator& operator=(const PercolatorCoordinator&) = delete;

    /**
     * @brief Execute the Percolator protocol for @p txn.
     *
     * Drives the full Percolator commit sequence:
     *  Phase 1 – PreWrite: lock secondaries first, then lock primary.
     *  Phase 2 – Assign TrueTime commit timestamp, perform commit-wait.
     *  Phase 3 – Commit primary, then commit secondaries (lock release).
     *
     * The shard-level operations are performed via the supplied callbacks
     * so this class is independent of the RPC transport layer and can be
     * used without a live CrossShardTransactionCoordinator.
     *
     * @param txn         Transaction to commit (state is mutated in-place).
     * @param prepare_fn  Callback: lock (prepare) one shard.
     * @param commit_fn   Callback: commit one shard.
     * @param abort_fn    Callback: abort / unlock one shard.
     * @return            true if the transaction committed successfully.
     */
    bool execute(
        CrossShardTransaction& txn,
        SendPrepareFn prepare_fn,
        SendCommitFn  commit_fn,
        SendAbortFn   abort_fn
    );

    /**
     * @brief Reclaim stale Percolator locks left by a failed coordinator.
     *
     * Iterates over @p stale_txn_ids and issues abort RPCs for every shard
     * whose Percolator lock has exceeded the stale-lock threshold.  Intended
     * to be called from OrphanDetector on its cleanup interval.
     *
     * @param stale_txn_ids  Transaction IDs whose locks should be cleaned up.
     * @param coordinator    Host coordinator used for getTransaction() / abort().
     * @return               Number of locks successfully released.
     */
    size_t cleanStaleLocks(
        const std::vector<std::string>& stale_txn_ids,
        CrossShardTransactionCoordinator& coordinator
    );

private:
    Config config_;
    std::shared_ptr<themis::sharding::TrueTime> truetime_;
    TransactionWAL* wal_;  ///< Non-owning pointer; lifetime managed by caller.

    /// Compute a commit timestamp using TrueTime if available, else wall clock.
    int64_t computeCommitTimestamp() const;

    /// Perform the TrueTime commit-wait: spin until now().earliest > deadline.
    void commitWait(int64_t commit_ts_ns) const;
};

} // namespace sharding
} // namespace themisdb
