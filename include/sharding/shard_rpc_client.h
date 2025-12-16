#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "storage/status.h"

namespace themis::sharding {

/**
 * @brief RPC Client for shard-to-shard communication
 * 
 * v1.3.0: Basic RPC implementation for distributed transactions
 * Supports 2PC (Two-Phase Commit) protocol messages
 */
class ShardRPCClient {
public:
    struct Config {
        std::string endpoint;           // Shard endpoint (e.g., "http://shard1:8529")
        int timeout_ms = 5000;          // RPC timeout in milliseconds
        int max_retries = 3;            // Maximum retry attempts
        int retry_delay_ms = 100;       // Delay between retries
    };
    
    explicit ShardRPCClient(const Config& config);
    ~ShardRPCClient();
    
    // Disable copy, allow move
    ShardRPCClient(const ShardRPCClient&) = delete;
    ShardRPCClient& operator=(const ShardRPCClient&) = delete;
    ShardRPCClient(ShardRPCClient&&) = default;
    ShardRPCClient& operator=(ShardRPCClient&&) = default;
    
    /**
     * @brief Send PREPARE request (2PC Phase 1)
     * @param txn_id Transaction ID
     * @param operations Operations to prepare
     * @return true if shard votes to commit
     */
    bool prepare(
        const std::string& txn_id,
        const nlohmann::json& operations
    );
    
    /**
     * @brief Send COMMIT request (2PC Phase 2)
     * @param txn_id Transaction ID
     * @param commit_timestamp Commit timestamp for MVCC
     * @return true if committed successfully
     */
    bool commit(
        const std::string& txn_id,
        int64_t commit_timestamp
    );
    
    /**
     * @brief Send ABORT request
     * @param txn_id Transaction ID
     * @return true if aborted successfully
     */
    bool abort(const std::string& txn_id);
    
    /**
     * @brief Execute snapshot read at specific timestamp
     * @param snapshot_ts Snapshot timestamp for consistent read
     * @param query Query to execute
     * @return Query results
     */
    nlohmann::json snapshotRead(
        int64_t snapshot_ts,
        const nlohmann::json& query
    );
    
    /**
     * @brief Check if shard is available
     */
    bool ping();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    /**
     * @brief Send RPC request with retry logic
     */
    nlohmann::json sendRequest(
        const std::string& method,
        const nlohmann::json& params
    );
};

} // namespace themis::sharding
