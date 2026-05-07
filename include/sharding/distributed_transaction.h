/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_transaction.h                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/truetime.h"
#include "sharding/wal_manager.h"
#include <string>
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
    ACTIVE,      // Transaction is active
    PREPARING,   // Preparing to commit (phase 1 of 2PC)
    PREPARED,    // All participants prepared (ready to commit)
    COMMITTING,  // Committing transaction (phase 2 of 2PC)
    COMMITTED,   // Transaction committed successfully
    ABORTING,    // Aborting transaction
    ABORTED      // Transaction aborted
};

/**
 * @brief Isolation level for distributed transactions
 *
 * Controls the read-visibility and locking behaviour applied by
 * TwoPhaseCommitParticipant::onPrepare() via the validate_and_lock callback.
 */
enum class DistributedIsolationLevel {
    SNAPSHOT_ISOLATION, // MVCC snapshot reads – default; no read locks needed
    SERIALIZABLE        // Full serializability; read-set validation on prepare
};

/**
 * @brief Participant in a distributed transaction
 */
struct TransactionParticipant {
    std::string shard_id;        // Shard identifier
    std::string endpoint;        // Network endpoint
    bool prepared;               // Has this participant prepared?
    bool committed;              // Has this participant committed?
    std::string error_msg;       // Error message if any
};

/**
 * @brief Distributed transaction metadata
 */
struct DistributedTransaction {
    std::string transaction_id;           // Unique transaction ID
    TransactionState state;               // Current state
    DistributedIsolationLevel isolation_level; // Isolation level
    std::chrono::nanoseconds start_time;  // Transaction start timestamp
    std::chrono::nanoseconds commit_time; // Commit timestamp (TrueTime)
    std::vector<TransactionParticipant> participants;  // Participating shards
    nlohmann::json operations;            // Operations to execute
    uint32_t commit_retry_count;          // Number of commit retries
    std::string error_detail;             // Detailed error message
    
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
class DistributedTransactionCoordinator {
public:
    /**
     * @brief Configuration for distributed transaction coordinator
     */
    struct Config {
        uint64_t prepare_timeout_ms = 10000;   // Timeout for prepare phase
        uint64_t commit_timeout_ms = 10000;    // Timeout for commit phase
        uint64_t max_concurrent_txns = 1000;   // Max concurrent transactions
        bool enable_read_only_opt = true;      // Enable read-only optimization
        uint64_t rpc_timeout_ms = 5000;        // RPC timeout per shard call
        uint32_t max_retries = 3;              // RPC retry attempts
        uint32_t max_commit_retries = 5;       // Max commit phase retries (higher for durability)
        uint64_t retry_backoff_base_ms = 100;  // Base delay for exponential backoff
        uint64_t max_backoff_ms = 5000;        // Maximum backoff delay
        bool enable_recovery_log = true;       // Enable transaction recovery logging
        std::string coordinator_id;            // Identifier for Prometheus labels (default: "default")

        /**
         * When true (default), transactions with isolation_level ==
         * SNAPSHOT_ISOLATION use the Percolator commit path instead of 2PC:
         *   - Skip the prepare / vote round.
         *   - Derive commit timestamp from TrueTime::now_with_uncertainty().
         *   - Apply commit-wait before sending COMMIT to participants.
         *
         * SERIALIZABLE transactions always use 2PC regardless of this flag.
         */
        bool use_percolator_for_snapshot = true;
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
    std::shared_ptr<TrueTime> truetime_;
    Config config_;

    // Shard-id to real network endpoint (populated via setShardEndpointMap())
    std::unordered_map<std::string, std::string> shard_endpoint_map_;
    
    // Write-Ahead Log for transaction recovery
    std::unique_ptr<WALManager> wal_manager_;
    
    mutable std::mutex mutex_;
    std::map<std::string, DistributedTransaction> transactions_;
    
    // Statistics
    std::atomic<uint64_t> total_transactions_{0};
    std::atomic<uint64_t> committed_transactions_{0};
    std::atomic<uint64_t> aborted_transactions_{0};
    std::atomic<uint64_t> readonly_transactions_{0};
    
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
    void logTransactionForRecovery(const DistributedTransaction& txn);
    
    /**
     * @brief Log prepared state for recovery (for in-doubt transaction handling)
     * @param txn Distributed transaction
     */
    void logPreparedStateForRecovery(const DistributedTransaction& txn);
    
    /**
     * @brief Recover in-doubt transactions from log
     */
    void recoverTransactions();

    /**
     * @brief Percolator-style commit for SNAPSHOT_ISOLATION transactions.
     *
     * Replaces the 2PC prepare phase with an optimistic, primary-lock
     * approach:
     *   1. Skip the global prepare / vote round.
     *   2. Derive commit timestamp via TrueTime::now_with_uncertainty().latest.
     *   3. Perform commit-wait: spin until TT.now().earliest >
     *      commit_ts + max_uncertainty.
     *   4. Send COMMIT directly to all participants.
     *
     * This path is chosen when Config::use_percolator_for_snapshot == true and
     * the transaction's isolation_level == SNAPSHOT_ISOLATION.
     *
     * @param txn Distributed transaction (state mutated in-place).
     * @return True if all participants committed successfully.
     */
    bool percolatorCommit(DistributedTransaction& txn);
};

} // namespace themis::sharding
