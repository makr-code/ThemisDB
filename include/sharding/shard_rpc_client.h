/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_rpc_client.h                                 ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#endif

namespace themis::sharding {

/**
 * @brief RPC Client for shard-to-shard communication
 * 
 * v1.3.0: Basic RPC implementation for distributed transactions
 * Supports 2PC (Two-Phase Commit) protocol messages
 * 
 * v1.3.4: Added gRPC support for multi-node cluster deployments
 * - Real gRPC connections for horizontal scaling
 * - Automatic retry with exponential backoff
 * - Connection keepalive and pooling
 * - Healthcheck endpoint
 * - Error categorization (retryable vs non-retryable)
 */
class ShardRPCClient {
public:
    struct Config {
        std::string endpoint;           // Shard endpoint (e.g., "shard1:50051" for gRPC)
        int timeout_ms = 5000;          // RPC timeout in milliseconds
        int max_retries = 3;            // Maximum retry attempts
        int retry_delay_ms = 100;       // Initial delay between retries (exponential backoff)
        
        // mTLS Configuration (optional, required for production)
        bool enable_mtls = false;       // Enable mutual TLS authentication
        std::string tls_cert_path;      // Path to client certificate (PEM format)
        std::string tls_key_path;       // Path to client private key (PEM format)
        std::string tls_ca_cert_path;   // Path to CA certificate for server verification (PEM format)
        bool tls_verify_server = true;  // Verify server certificate against CA
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
     * Routes to either gRPC or in-process implementation
     */
    nlohmann::json sendRequest(
        const std::string& method,
        const nlohmann::json& params
    );
    
    /**
     * @brief Send request using in-process simulation (for single-node)
     */
    nlohmann::json sendRequestInProcess(
        const std::string& method,
        const nlohmann::json& params
    );
    
#ifdef THEMIS_ENABLE_GRPC
    /**
     * @brief Send request using gRPC (for multi-node)
     */
    nlohmann::json sendRequestGrpc(
        const std::string& method,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC prepare transaction request
     */
    nlohmann::json handlePrepareGrpc(
        grpc::ClientContext& context,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC commit transaction request
     */
    nlohmann::json handleCommitGrpc(
        grpc::ClientContext& context,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC abort transaction request
     */
    nlohmann::json handleAbortGrpc(
        grpc::ClientContext& context,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC snapshot read request
     */
    nlohmann::json handleSnapshotReadGrpc(
        grpc::ClientContext& context,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC healthcheck request
     */
    nlohmann::json handleHealthCheckGrpc(
        grpc::ClientContext& context
    );
    
    /**
     * @brief Determine if a gRPC error is retryable
     */
    bool isRetryableError(grpc::StatusCode code);
#endif
};

} // namespace themis::sharding
