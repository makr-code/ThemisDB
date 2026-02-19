// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_CROSS_SHARD_TRANSACTION_H
#define THEMISDB_SHARDING_CROSS_SHARD_TRANSACTION_H

#include "sharding/consensus_module.h"
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include "storage/wal_manager.h"  // For LSN type
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

// Forward declarations for Phase 2.3.3 integration
class TransactionWAL;
class TransactionSnapshotManager;

/**
 * @brief Transaction protocol type
 */
enum class TransactionProtocol {
    TWO_PHASE_COMMIT,      // 2PC - blocking but strongly consistent
    THREE_PHASE_COMMIT,    // 3PC - non-blocking variant
    SAGA,                  // SAGA - compensating transactions for long-running txns
    PERCOLATOR             // Percolator - optimistic concurrency for distributed writes
};

/**
 * @brief Transaction isolation level
 */
enum class IsolationLevel {
    READ_UNCOMMITTED,      // Dirty reads allowed
    READ_COMMITTED,        // Read only committed data
    REPEATABLE_READ,       // Consistent reads within transaction
    SNAPSHOT_ISOLATION,    // MVCC snapshot isolation
    SERIALIZABLE          // Fully serializable
};

/**
 * @brief Transaction state
 */
enum class TransactionState {
    ACTIVE,                // Transaction is active
    PREPARING,             // Preparing to commit (2PC phase 1)
    PREPARED,              // All participants ready to commit
    COMMITTING,            // Committing changes
    COMMITTED,             // Transaction committed successfully
    ABORTING,              // Aborting transaction
    ABORTED,               // Transaction aborted
    UNKNOWN                // State unknown (coordinator failure)
};

/**
 * @brief Shard participant in a transaction
 */
struct ShardParticipant {
    std::string shard_id;                    // Shard identifier
    std::string endpoint;                    // Shard endpoint
    std::vector<std::string> operations;     // Operations on this shard
    bool prepared = false;                   // Prepared for commit
    bool committed = false;                  // Committed
    bool aborted = false;                    // Aborted
    std::string error_message;               // Error message if failed
};

/**
 * @brief Cross-shard transaction metadata
 */
struct CrossShardTransaction {
    std::string transaction_id;              // Global transaction ID
    TransactionProtocol protocol;            // Transaction protocol
    IsolationLevel isolation_level;          // Isolation level
    TransactionState state;                  // Current state
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::map<std::string, ShardParticipant> participants;  // Shard ID -> participant
    nlohmann::json metadata;                 // Additional metadata
    
    // MVCC timestamps for snapshot isolation
    int64_t snapshot_timestamp = 0;          // Read timestamp (start of transaction)
    int64_t commit_timestamp = 0;            // Commit timestamp (end of transaction)
    
    // Compensation data (for SAGA)
    std::map<std::string, nlohmann::json> compensations;
    
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
    TransactionProtocol default_protocol = TransactionProtocol::TWO_PHASE_COMMIT;
    IsolationLevel default_isolation = IsolationLevel::SNAPSHOT_ISOLATION;
    
    // 2PC/3PC settings
    std::chrono::milliseconds prepare_timeout{5000};
    std::chrono::milliseconds commit_timeout{5000};
    std::chrono::milliseconds abort_timeout{5000};
    
    // SAGA settings
    bool saga_enable_compensation = true;
    std::chrono::milliseconds saga_step_timeout{10000};
    
    // Percolator settings
    std::chrono::milliseconds percolator_lock_timeout{1000};
    uint32_t percolator_max_retries = 3;
    
    // Deadlock detection
    bool enable_deadlock_detection = true;
    std::chrono::milliseconds deadlock_detection_interval{1000};
    
    // Transaction timeout
    std::chrono::milliseconds transaction_timeout{30000};
    
    // Transaction log path (defaults to /var/lib/themisdb/transaction_log.jsonl)
    std::string transaction_log_path = "/var/lib/themisdb/transaction_log.jsonl";
    
    // Persistence settings (Phase 2.3.3 - Transaction Durability)
    bool enable_persistence = false;                // Enable WAL-based persistence
    std::string data_dir;                           // Base directory for WAL and snapshots
    uint64_t snapshot_interval = 1000;              // Create snapshot every N operations
    size_t max_snapshots = 10;                      // Maximum snapshots to retain
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
class CrossShardTransactionCoordinator {
public:
    explicit CrossShardTransactionCoordinator(
        const CrossShardTransactionConfig& config,
        std::shared_ptr<ConsensusModule> consensus,
        std::shared_ptr<themis::sharding::TrueTime> truetime = nullptr
    );
    
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
     * @brief Load pending transactions from durable storage
     */
    std::vector<CrossShardTransaction> loadPendingTransactions();
    
    /**
     * @brief Recover coordinator state after failure
     */
    bool recoverFromFailure();
    
    /**
     * @brief Recover from WAL and snapshot (Phase 2.3.3)
     */
    bool recoverFromWAL();
    
    /**
     * @brief Create periodic snapshot of active transactions (Phase 2.3.3)
     */
    void createPeriodicSnapshot();
    
    CrossShardTransactionConfig config_;
    std::shared_ptr<ConsensusModule> consensus_;
    std::shared_ptr<themis::sharding::TrueTime> truetime_;
    
    // Transaction log file
    std::string transaction_log_path_;
    
    // Phase 2.3.3: WAL and Snapshot infrastructure
    std::unique_ptr<TransactionWAL> transaction_wal_;
    std::unique_ptr<TransactionSnapshotManager> snapshot_manager_;
    std::atomic<uint64_t> operations_since_snapshot_{0};
    LSN last_applied_lsn_{0};
    
    // State
    mutable std::mutex transactions_mutex_;
    std::map<std::string, CrossShardTransaction> transactions_;
    
    // Callbacks
    mutable std::mutex callbacks_mutex_;
    std::function<void(const std::string&, TransactionState, TransactionState)> 
        on_state_change_callback_;
    
    // Background thread
    std::atomic<bool> running_;
    std::thread deadlock_detection_thread_;
    
    // Statistics
    std::atomic<uint64_t> total_transactions_;
    std::atomic<uint64_t> committed_transactions_;
    std::atomic<uint64_t> aborted_transactions_;
    std::atomic<uint64_t> deadlocked_transactions_;
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_CROSS_SHARD_TRANSACTION_H
