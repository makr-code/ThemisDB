// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_TRANSACTION_LIFECYCLE_MANAGER_H
#define THEMISDB_SHARDING_TRANSACTION_LIFECYCLE_MANAGER_H

#include "sharding/cross_shard_transaction.h"
#include <string>
#include <map>
#include <queue>
#include <mutex>
#include <functional>
#include <chrono>

namespace themisdb {
namespace sharding {

/**
 * @brief Transaction metadata for lifecycle management
 */
struct TransactionMetadata {
    std::string transaction_id;
    TransactionState state;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;
    std::chrono::system_clock::time_point timeout_at;
};

/**
 * @brief Transaction timeout entry for priority queue
 */
struct TransactionTimeout {
    std::string transaction_id;
    std::chrono::system_clock::time_point timeout_at;
    
    bool operator<(const TransactionTimeout& other) const {
        // Min-heap: earlier timeouts have higher priority
        return timeout_at > other.timeout_at;
    }
};

/**
 * @brief Transaction Lifecycle Manager
 * 
 * Manages transaction lifecycle with:
 * - Bounded pending transaction limit
 * - TTL-based timeout and cleanup
 * - State transition tracking
 * - Automatic reaping of timed-out transactions
 */
class TransactionLifecycleManager {
public:
    struct Config {
        size_t max_pending{10000};
        std::chrono::milliseconds ttl{3600000};  // 1 hour
        std::function<void(const std::string&)> on_timeout;
    };
    
    struct Stats {
        size_t total_pending;
        std::map<TransactionState, size_t> by_state;
        size_t timed_out;
        size_t completed;
    };
    
    explicit TransactionLifecycleManager(const Config& config);
    
    /**
     * @brief Register new transaction
     * @param txn_id Transaction ID
     * @return true if registered, false if limit exceeded
     */
    bool registerTransaction(const std::string& txn_id);
    
    /**
     * @brief Transition transaction state
     * @param txn_id Transaction ID
     * @param new_state New state
     * @return true if transitioned successfully
     */
    bool transitionState(const std::string& txn_id, TransactionState new_state);
    
    /**
     * @brief Clean up completed transactions
     * @return Number of transactions cleaned up
     */
    size_t cleanupCompletedTransactions();
    
    /**
     * @brief Reap timed-out transactions
     * @return Number of transactions reaped
     */
    size_t reapTimedOutTransactions();
    
    /**
     * @brief Get transaction metadata
     * @param txn_id Transaction ID
     * @return Metadata if found
     */
    std::optional<TransactionMetadata> getMetadata(const std::string& txn_id) const;
    
    /**
     * @brief Get statistics
     * @return Current stats
     */
    Stats getStats() const;

private:
    Config config_;
    mutable std::mutex mutex_;
    std::map<std::string, TransactionMetadata> transactions_;
    std::priority_queue<TransactionTimeout> timeouts_;
    
    size_t timed_out_count_{0};
    size_t completed_count_{0};
    
    /**
     * @brief Check if transaction is terminal state
     */
    bool isTerminalState(TransactionState state) const;
    
    /**
     * @brief Remove transaction from tracking
     */
    void removeTransaction(const std::string& txn_id);
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_TRANSACTION_LIFECYCLE_MANAGER_H
