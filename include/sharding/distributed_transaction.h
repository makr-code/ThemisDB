/**
 * @file distributed_transaction.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/truetime.h"
#include "sharding/wal_manager.h"
#include "transaction/recoverable_two_phase_coordinator.h"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * @brief Transaction state
 */
enum class TransactionState {
    ACTIVE,      ///< Transaction is active.
    PREPARING,   ///< Phase 1 of 2PC in progress.
    PREPARED,    ///< All participants prepared and ready for decision.
    COMMITTING,  ///< Phase 2 of 2PC commit in progress.
    COMMITTED,   ///< Transaction committed successfully.
    ABORTING,    ///< Abort propagation in progress.
    ABORTED      ///< Transaction aborted.
};

/**
 * @brief Isolation level for distributed transactions
 *
 * Controls the read-visibility and locking behaviour applied by
 * TwoPhaseCommitParticipant::onPrepare() via the validate_and_lock callback.
 */
enum class DistributedIsolationLevel {
    SNAPSHOT_ISOLATION, ///< MVCC snapshot reads; may allow write-skew/phantoms.
    SERIALIZABLE        ///< Full serializability with prepare-time validation.
};

/**
 * @brief Participant in a distributed transaction
 */
struct TransactionParticipant {
    std::string shard_id;   ///< Target shard identifier.
    std::string endpoint;   ///< RPC endpoint for shard participant.
    bool prepared;          ///< True when participant voted/prepared.
    bool committed;         ///< True when participant applied commit.
    std::string error_msg;  ///< Participant-specific failure detail.
};

/**
 * @brief Distributed transaction metadata
 */
struct DistributedTransaction {
    std::string transaction_id;                      ///< Unique transaction ID.
    TransactionState state;                          ///< Current lifecycle state.
    DistributedIsolationLevel isolation_level;       ///< Isolation level selected for transaction.
    std::chrono::nanoseconds start_time;             ///< Transaction start timestamp.
    std::chrono::nanoseconds commit_time;            ///< Assigned commit timestamp.
    std::vector<TransactionParticipant> participants; ///< Participating shards.
    nlohmann::json operations;                       ///< Grouped operations per shard.
    uint32_t commit_retry_count;                     ///< Number of commit retry attempts.
    std::string error_detail;                        ///< Extended error detail for diagnostics.
    
    DistributedTransaction()
        : state(TransactionState::ACTIVE)
        , isolation_level(DistributedIsolationLevel::SNAPSHOT_ISOLATION)
        , start_time(0)
        , commit_time(0)
        , commit_retry_count(0) {}
};

/**
 * @brief Manages distributed transactions across shards using TrueTime
 * 
 * This coordinator implements a two-phase commit protocol enhanced with
 * TrueTime for:
 * - Snapshot isolation across shards
 * - Strict serializability guarantees
 * - Wait-free read-only transactions
 */
class DistributedTransactionCoordinator
    : public themis::transaction::IRecoverableTwoPhaseCoordinator {
public:
    /**
     * @brief Configuration for distributed transaction coordinator
     */
    struct Config {
        uint64_t prepare_timeout_ms = 10000;    ///< Timeout for prepare phase.
        uint64_t commit_timeout_ms = 10000;     ///< Timeout for commit phase.
        uint64_t max_concurrent_txns = 1000;    ///< Maximum concurrent transactions.
        bool enable_read_only_opt = true;       ///< Enable read-only fast-path optimization.
        uint64_t rpc_timeout_ms = 5000;         ///< RPC timeout per shard call.
        uint32_t max_retries = 3;               ///< Per-RPC retry attempts.
        uint32_t max_commit_retries = 5;        ///< Maximum retries for commit phase.
        uint64_t retry_backoff_base_ms = 100;   ///< Base delay for exponential retry backoff.
        uint64_t max_backoff_ms = 5000;         ///< Maximum retry backoff delay.
        bool enable_recovery_log = true;        ///< Enable WAL-based transaction recovery logging.
        std::string wal_directory = "./wal/coordinator"; ///< WAL directory used for decision/recovery persistence.
        std::string coordinator_id;             ///< Prometheus label value for coordinator metrics.

        /**
         * When true, transactions with isolation_level == SNAPSHOT_ISOLATION use
         * the Percolator commit path instead of full 2PC:
         *   - Run a cross-shard prepare phase for write-write conflict detection.
         *   - Derive commit timestamp from TrueTime::now_with_uncertainty().
         *   - Apply commit-wait before sending COMMIT to participants.
         *
         * SERIALIZABLE transactions always use full 2PC regardless of this flag.
         *
         * @warning Percolator is designed for read-heavy, non-safety-critical
         * workloads.  For safety-critical applications that require full ACID
         * guarantees and zero tolerance for anomalies, leave this flag at its
         * default of false so that all transactions use the standard 2PC path.
         * Enable it only after explicitly accepting the trade-offs documented in
         * docs/DISTRIBUTED_TRANSACTIONS.md §Percolator Mode.
         */
        bool use_percolator_for_snapshot = false;
    };

    /**
     * @brief Construct coordinator
     * @param truetime TrueTime instance for timestamping
     * @param config Coordinator configuration
     */
    explicit DistributedTransactionCoordinator(
        std::shared_ptr<TrueTime> truetime,
        const Config& config
    );

    /** @brief Default destructor. */
    ~DistributedTransactionCoordinator() = default;
    
    /**
     * @brief Begin a new distributed transaction
     * @param shard_ids       List of participating shards
     * @param isolation_level Desired isolation level (default: SNAPSHOT_ISOLATION)
     * @return Transaction ID
     */
    std::string beginTransaction(
        const std::vector<std::string>& shard_ids,
        DistributedIsolationLevel isolation_level = DistributedIsolationLevel::SNAPSHOT_ISOLATION
    );
    
    /**
     * @brief Add an operation to the transaction
     * @param txn_id Transaction ID
     * @param shard_id Target shard
     * @param operation Operation to perform
     * @return True if added successfully
     */
    bool addOperation(
        const std::string& txn_id,
        const std::string& shard_id,
        const nlohmann::json& operation
    );
    
    /**
     * @brief Commit a distributed transaction (two-phase commit)
     * 
     * Phase 1: Prepare
     * - Send prepare requests to all participants
     * - Wait for all to respond with prepared or timeout
     * 
     * Phase 2: Commit
     * - Assign commit timestamp using TrueTime
     * - Wait until commit_timestamp is definitely in the past
     * - Send commit requests to all participants
     * 
     * @param txn_id Transaction ID
     * @return True if committed successfully
     */
    bool commit(const std::string& txn_id);
    
    /**
     * @brief Abort a distributed transaction
     * @param txn_id Transaction ID
     * @return True if aborted successfully
     */
    bool abort(const std::string& txn_id);
    
    /**
     * @brief Execute a read-only transaction (wait-free with TrueTime)
     * 
     * Read-only transactions don't need 2PC:
     * 1. Get timestamp t = TT.now().latest
     * 2. Read from snapshot at timestamp t
     * 3. No locks needed, no waiting
     * 
     * @param shard_ids Shards to read from
     * @param operations Read operations
     * @return Results from all shards
     */
    nlohmann::json executeReadOnly(
        const std::vector<std::string>& shard_ids,
        const nlohmann::json& operations
    );
    
    /**
     * @brief Get transaction status
     * @param txn_id Transaction ID
     * @return Transaction state or nullopt if not found
     */
    std::optional<TransactionState> getTransactionState(const std::string& txn_id) const;
    
    /**
     * @brief Get statistics
     * @return JSON with coordinator stats
     */
    nlohmann::json getStatistics() const;

    /**
     * @brief Recover and re-drive in-doubt transactions from the WAL.
     *
     * Transactions with a durable COMMIT decision are re-driven with COMMIT.
     * Transactions that reached PREPARED without a durable final decision are
     * conservatively re-driven with ABORT.
     *
     * @return Number of transactions resolved to a final state by this pass.
     */
    [[nodiscard]] size_t recoverInDoubtTransactions() override;

    /**
     * @brief Return the canonical coordinator name for global recovery reports.
     * @return Stable coordinator type name.
     */
    [[nodiscard]] std::string recoveryCoordinatorName() const override;

    /**
     * @brief Return the durable backend used by this coordinator.
     * @return "WAL" when recovery logging is enabled, otherwise "disabled".
     */
    [[nodiscard]] std::string recoveryBackendName() const override;

    /**
     * @brief Snapshot current in-doubt transactions using the shared state model.
     * @return Normalized non-final transaction list for global recovery orchestration.
     */
    [[nodiscard]] std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;

    /**
     * @brief Register real gRPC endpoints for shard IDs.
     *
     * Injects a shard_id → network-address mapping so that
     * beginTransaction() can populate participant endpoints with real
     * addresses instead of the placeholder "shard://<id>" URIs.
     *
     * @param map Map of shard_id to gRPC address (e.g. "host:port")
     */
    void setShardEndpointMap(std::unordered_map<std::string, std::string> map);

private:
    std::shared_ptr<TrueTime> truetime_;  ///< TrueTime source used for timestamp assignment and waits.
    Config config_;                       ///< Runtime coordinator configuration.

    // Shard-id to real network endpoint (populated via setShardEndpointMap())
    std::unordered_map<std::string, std::string> shard_endpoint_map_; ///< Registered shard endpoint map.
    
    // Write-Ahead Log for transaction recovery
    std::unique_ptr<WALManager> wal_manager_; ///< Optional WAL manager for in-doubt recovery.
    
    // ======================================================================
    // DEADLOCK PREVENTION: Canonical Lock Acquisition Order
    // ======================================================================
    // To prevent circular wait deadlocks, all code MUST acquire locks in
    // this strict order when multiple locks are needed:
    //   1. mutex_ (primary transaction state lock)
    //   2. error_mutex (used only within phase operations, never held across calls)
    // 
    // CRITICAL RULES:
    // - NEVER hold both locks across a function call or async operation
    // - NEVER acquire error_mutex while holding mutex_
    // - ALWAYS unlock all locks before waiting on futures
    // - Use lock.unlock() explicitly before async operations
    // ======================================================================
    mutable std::timed_mutex mutex_;                         ///< Protects transactions and endpoint map.
    std::map<std::string, DistributedTransaction> transactions_; ///< Active and recent transaction registry.
    
    // Statistics
    std::atomic<uint64_t> total_transactions_{0};      ///< Total begun transactions.
    std::atomic<uint64_t> committed_transactions_{0};  ///< Successfully committed transactions.
    std::atomic<uint64_t> aborted_transactions_{0};    ///< Aborted transactions.
    std::atomic<uint64_t> readonly_transactions_{0};   ///< Executed read-only transactions.
    
    /**
     * @brief Phase 1: Send prepare requests to all participants
     * @param txn Distributed transaction
     * @return True if all participants prepared successfully
     */
    bool preparePhase(DistributedTransaction& txn);
    
    /**
     * @brief Phase 2: Send commit requests to all participants
     * @param txn Distributed transaction
     * @return True if all participants committed successfully
     */
    bool commitPhase(DistributedTransaction& txn);
    
    /**
     * @brief Send prepare request to a participant
     * @param participant Participant to prepare
     * @param txn_id Transaction ID
     * @return True if prepared successfully
     */
    bool sendPrepare(TransactionParticipant& participant, const std::string& txn_id);
    
    /**
     * @brief Send commit request to a participant
     * @param participant Participant to commit
     * @param txn_id Transaction ID
     * @param commit_timestamp Commit timestamp
     * @return True if committed successfully
     */
    bool sendCommit(
        TransactionParticipant& participant,
        const std::string& txn_id,
        std::chrono::nanoseconds commit_timestamp
    );
    
    /**
     * @brief Send abort request to a participant
     * @param participant Participant to abort
     * @param txn_id Transaction ID
     * @return True if aborted successfully
     */
    bool sendAbort(TransactionParticipant& participant, const std::string& txn_id);
    
    /**
     * @brief Generate unique transaction ID
     * @return Transaction ID
     */
    std::string generateTransactionId();
    
    /**
     * @brief Clean up old transactions
     */
    void cleanupOldTransactions();
    
    /**
     * @brief Calculate backoff delay for retries using exponential backoff
     * @param retry_count Current retry attempt count
     * @return Delay in milliseconds
     */
    uint64_t calculateBackoffDelay(uint32_t retry_count) const;
    
    /**
     * @brief Retry commit operation with exponential backoff
     * @param txn Distributed transaction
     * @return True if committed successfully after retries
     */
    bool retryCommitPhase(DistributedTransaction& txn);
    
    /**
     * @brief Log transaction state for recovery
     * @param txn Distributed transaction
     */
    void logBeginStateForRecovery(const DistributedTransaction& txn);

    /**
     * @brief Log a durable 2PC decision or completion marker for recovery.
     * @param txn Distributed transaction
     * @param commit True for COMMIT, false for ABORT.
     * @param phase Either "decision" or "complete".
     * @param reason Optional diagnostic reason for ABORT/error paths.
     */
    [[nodiscard]] bool logDecisionStateForRecovery(
        const DistributedTransaction& txn,
        bool commit,
        std::string_view phase,
        std::string_view reason = {}
    );

    /**
     * @brief Log terminal COMMIT/ABORT completion state for recovery.
     * @param txn Distributed transaction
     */
    void logTransactionForRecovery(const DistributedTransaction& txn);
    
    /**
     * @brief Log prepared state for recovery (for in-doubt transaction handling)
     * @param txn Distributed transaction
     */
    [[nodiscard]] bool logPreparedStateForRecovery(const DistributedTransaction& txn);
    
    /**
     * @brief Recover in-doubt transactions from log
     * @return Number of transactions resolved to a final state.
     */
    size_t recoverTransactions();

    /**
     * @brief Percolator-style commit for SNAPSHOT_ISOLATION transactions.
     *
     * Executes cross-shard conflict detection and TrueTime-anchored commit for
     * snapshot-isolated transactions.  The full sequence is:
     *   0. Prepare phase — all participants vote COMMIT or ABORT (conflict check).
     *      Any ABORT vote triggers abort of all prepared participants and returns false.
     *   1. Derive commit timestamp via TrueTime::now_with_uncertainty().latest.
     *   2. Perform commit-wait: spin until TT.now().earliest > commit_ts.
     *   3. Send COMMIT to all participants with the agreed timestamp.
     *
     * This path is chosen when Config::use_percolator_for_snapshot == true and
     * the transaction's isolation_level == SNAPSHOT_ISOLATION.
     *
     * @param txn Distributed transaction (state mutated in-place).
     * @return True if all participants prepared and committed successfully.
     */
    bool percolatorCommit(DistributedTransaction& txn);
};

} // namespace themis::sharding
