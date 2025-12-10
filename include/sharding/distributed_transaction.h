/**
 * ThemisDB Distributed Transaction Coordinator with TrueTime
 * 
 * Integrates TrueTime clock synchronization with ACID transactions
 * and SAGA pattern for cross-shard distributed transactions.
 * 
 * Features:
 * - External consistency guarantees using TrueTime
 * - Cross-shard 2PC (Two-Phase Commit) with timestamps
 * - SAGA pattern with compensating transactions
 * - Deadlock detection using timestamp ordering
 * - Network-aware commit-wait optimization
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/truetime_clock.h"
#include "sharding/shard_latency_monitor.h"
#include "sharding/shard_router.h"
#include "transaction/transaction_manager.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>

namespace themis::sharding {

/**
 * Transaction isolation level
 */
enum class IsolationLevel {
    READ_UNCOMMITTED,
    READ_COMMITTED,
    REPEATABLE_READ,
    SERIALIZABLE,
    SNAPSHOT_ISOLATION  // Using TrueTime timestamps
};

/**
 * Transaction state
 */
enum class TransactionState {
    ACTIVE,         // Transaction in progress
    PREPARING,      // 2PC: Prepare phase
    PREPARED,       // 2PC: All participants prepared
    COMMITTING,     // 2PC: Commit phase
    COMMITTED,      // Transaction committed
    ABORTING,       // Transaction aborting
    ABORTED,        // Transaction aborted
    COMPENSATING    // SAGA: Executing compensations
};

/**
 * Distributed transaction participant info
 */
struct TransactionParticipant {
    std::string shard_id;
    std::string endpoint;
    bool is_coordinator;
    bool prepared;
    bool committed;
    TrueTimeStamp prepare_timestamp;
    TrueTimeStamp commit_timestamp;
};

/**
 * SAGA step definition
 */
struct SagaStep {
    std::string step_id;
    std::string shard_id;
    std::string operation;              // JSON operation to execute
    std::string compensation;           // JSON compensation to execute on rollback
    int64_t timeout_ms;                 // Timeout for this step
    TrueTimeStamp executed_at;          // When step was executed
    bool completed;
    bool compensated;
};

/**
 * Distributed transaction metadata
 */
struct DistributedTransaction {
    std::string txn_id;                     // Unique transaction ID
    std::string coordinator_shard;          // Coordinator shard ID
    IsolationLevel isolation_level;
    TransactionState state;
    
    // TrueTime timestamps for ordering
    TrueTimeStamp start_timestamp;          // When transaction started
    TrueTimeStamp commit_timestamp;         // When transaction committed
    TrueTimeStamp snapshot_timestamp;       // For snapshot isolation
    
    // Participants
    std::vector<TransactionParticipant> participants;
    
    // SAGA-specific
    bool is_saga;
    std::vector<SagaStep> saga_steps;
    int current_step;
    
    // Timeout
    std::chrono::system_clock::time_point deadline;
    
    // Metadata
    std::map<std::string, std::string> metadata;
};

/**
 * Transaction coordinator result
 */
struct TransactionResult {
    bool success;
    std::string txn_id;
    TrueTimeStamp commit_timestamp;
    std::string error_msg;
    std::vector<std::string> failed_participants;
};

/**
 * Distributed Transaction Coordinator
 * 
 * Coordinates distributed transactions across shards using
 * TrueTime for global ordering and consistency.
 */
class DistributedTransactionCoordinator {
public:
    /**
     * Configuration
     */
    struct Config {
        std::string local_shard_id;
        
        // Transaction settings
        IsolationLevel default_isolation = IsolationLevel::SNAPSHOT_ISOLATION;
        uint32_t default_timeout_ms = 30000;        // 30 seconds
        uint32_t prepare_timeout_ms = 10000;        // 10 seconds
        uint32_t commit_timeout_ms = 10000;         // 10 seconds
        
        // 2PC settings
        bool enable_2pc = true;
        uint32_t max_prepare_retries = 3;
        
        // SAGA settings
        bool enable_saga = true;
        uint32_t max_compensation_retries = 5;
        uint32_t compensation_retry_delay_ms = 1000;
        
        // TrueTime integration
        bool use_truetime = true;
        bool enforce_external_consistency = true;   // Use commit-wait
    };
    
    /**
     * Construct coordinator
     */
    DistributedTransactionCoordinator(
        const Config& config,
        std::shared_ptr<TrueTimeClock> truetime_clock,
        std::shared_ptr<ShardLatencyMonitor> latency_monitor,
        std::shared_ptr<ShardRouter> shard_router
    );
    
    /**
     * Begin distributed transaction
     * @param isolation Isolation level
     * @param participant_shards List of participating shards
     * @return Transaction ID
     */
    std::string beginTransaction(
        IsolationLevel isolation,
        const std::vector<std::string>& participant_shards
    );
    
    /**
     * Execute operation within transaction
     * @param txn_id Transaction ID
     * @param shard_id Target shard
     * @param operation Operation to execute (JSON)
     * @return Success
     */
    bool executeOperation(
        const std::string& txn_id,
        const std::string& shard_id,
        const std::string& operation
    );
    
    /**
     * Commit transaction (2PC protocol)
     * @param txn_id Transaction ID
     * @return Transaction result with commit timestamp
     */
    TransactionResult commit(const std::string& txn_id);
    
    /**
     * Abort transaction
     * @param txn_id Transaction ID
     * @return Success
     */
    bool abort(const std::string& txn_id);
    
    /**
     * Begin SAGA transaction
     * @param steps SAGA steps to execute
     * @return Transaction ID
     */
    std::string beginSaga(const std::vector<SagaStep>& steps);
    
    /**
     * Execute SAGA (runs all steps, compensates on failure)
     * @param txn_id SAGA transaction ID
     * @return Transaction result
     */
    TransactionResult executeSaga(const std::string& txn_id);
    
    /**
     * Get transaction state
     */
    std::optional<DistributedTransaction> getTransaction(const std::string& txn_id) const;
    
    /**
     * Wait for transaction to reach specific state
     * @param txn_id Transaction ID
     * @param state Target state
     * @param timeout_ms Maximum wait time
     * @return true if reached state, false if timeout
     */
    bool waitForState(
        const std::string& txn_id,
        TransactionState state,
        uint32_t timeout_ms
    );
    
    /**
     * Check if transaction can proceed (deadlock detection)
     * Uses timestamp ordering (wait-die scheme)
     * 
     * @param txn_id Requesting transaction
     * @param resource Resource being accessed
     * @param holder_txn_id Transaction holding resource
     * @return true if can wait, false if should abort
     */
    bool canWaitForResource(
        const std::string& txn_id,
        const std::string& resource,
        const std::string& holder_txn_id
    );
    
    /**
     * Export Prometheus metrics
     */
    std::string exportPrometheusMetrics() const;
    
private:
    Config config_;
    std::shared_ptr<TrueTimeClock> truetime_clock_;
    std::shared_ptr<ShardLatencyMonitor> latency_monitor_;
    std::shared_ptr<ShardRouter> shard_router_;
    
    // Active transactions
    std::map<std::string, DistributedTransaction> transactions_;
    mutable std::mutex txn_mutex_;
    
    // Statistics
    std::atomic<uint64_t> stats_txn_started_{0};
    std::atomic<uint64_t> stats_txn_committed_{0};
    std::atomic<uint64_t> stats_txn_aborted_{0};
    std::atomic<uint64_t> stats_saga_started_{0};
    std::atomic<uint64_t> stats_saga_completed_{0};
    std::atomic<uint64_t> stats_saga_compensated_{0};
    std::atomic<uint64_t> stats_2pc_failures_{0};
    
    // Internal methods - 2PC
    bool prepare(DistributedTransaction& txn);
    bool commitPhase2(DistributedTransaction& txn);
    void abortAll(DistributedTransaction& txn);
    
    bool sendPrepare(
        const std::string& shard_id,
        const std::string& txn_id,
        const TrueTimeStamp& snapshot_ts
    );
    
    bool sendCommit(
        const std::string& shard_id,
        const std::string& txn_id,
        const TrueTimeStamp& commit_ts
    );
    
    bool sendAbort(
        const std::string& shard_id,
        const std::string& txn_id
    );
    
    // Internal methods - SAGA
    bool executeSagaStep(DistributedTransaction& txn, SagaStep& step);
    bool compensateSagaStep(DistributedTransaction& txn, const SagaStep& step);
    void compensateAll(DistributedTransaction& txn);
    
    // TrueTime integration
    TrueTimeStamp getCommitTimestamp(const DistributedTransaction& txn);
    bool waitForExternalConsistency(const TrueTimeStamp& commit_ts);
    
    // Utilities
    std::string generateTxnId();
    void cleanupTransaction(const std::string& txn_id);
};

/**
 * Transaction-aware operation with timestamp
 */
struct TimestampedOperation {
    std::string txn_id;
    std::string operation_type;         // INSERT, UPDATE, DELETE
    std::string collection;
    std::string document_id;
    std::string data;                   // JSON payload
    TrueTimeStamp timestamp;            // Operation timestamp
    
    // For snapshot isolation
    TrueTimeStamp read_timestamp;       // Snapshot read point
    
    // Serialize
    std::string toJson() const;
    static std::optional<TimestampedOperation> fromJson(const std::string& json);
};

/**
 * Optimistic Concurrency Control using TrueTime
 * 
 * Uses timestamp ordering for conflict detection without locking.
 */
class OptimisticConcurrencyControl {
public:
    OptimisticConcurrencyControl(
        std::shared_ptr<TrueTimeClock> truetime_clock
    );
    
    /**
     * Begin read phase
     * @return Read timestamp for snapshot
     */
    TrueTimeStamp beginRead();
    
    /**
     * Validate transaction before commit
     * Checks if any conflicting writes occurred after read timestamp
     * 
     * @param read_ts Transaction's read timestamp
     * @param write_set Set of documents written
     * @return true if validation passes
     */
    bool validate(
        const TrueTimeStamp& read_ts,
        const std::vector<std::string>& write_set
    );
    
    /**
     * Record write timestamp for a document
     */
    void recordWrite(
        const std::string& document_id,
        const TrueTimeStamp& write_ts
    );
    
    /**
     * Get last write timestamp for document
     */
    std::optional<TrueTimeStamp> getLastWriteTimestamp(
        const std::string& document_id
    ) const;
    
private:
    std::shared_ptr<TrueTimeClock> truetime_clock_;
    
    // Document -> Last write timestamp
    std::map<std::string, TrueTimeStamp> write_timestamps_;
    mutable std::mutex mutex_;
};

/**
 * Helper functions for transaction coordination
 */
namespace txn_utils {

/**
 * Check if transaction T1 happened-before T2
 * Uses TrueTime timestamps for definite ordering
 */
bool happenedBefore(
    const TrueTimeStamp& ts1,
    const TrueTimeStamp& ts2
);

/**
 * Check if two transactions are concurrent
 * (neither definitely before the other)
 */
bool areConcurrent(
    const TrueTimeStamp& ts1,
    const TrueTimeStamp& ts2
);

/**
 * Calculate maximum network-aware commit-wait duration
 * Takes into account network latency to all participants
 */
uint64_t calculateCommitWaitDuration(
    const std::vector<std::string>& participant_shards,
    const ShardLatencyMonitor& latency_monitor,
    const TrueTimeStamp& commit_ts
);

/**
 * Generate globally unique transaction ID
 * Format: {shard_id}_{timestamp}_{sequence}
 */
std::string generateTransactionId(
    const std::string& shard_id,
    const TrueTimeClock& clock
);

} // namespace txn_utils

} // namespace themis::sharding
