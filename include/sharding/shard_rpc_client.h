/**
 * @file shard_rpc_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=9; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=3, Debt=2, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <functional>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#endif

namespace themis::sharding {

// Forward declarations
class PrometheusMetrics;
class MTLSConnectionPoolManager;

} // namespace themis::sharding

namespace themisdb::sharding {
class OperationalMetrics;
}

namespace themis::sharding {

/**
 * @brief Exception thrown for non-retryable RPC errors.
 *
 * Thrown when a gRPC status code is classified as non-retryable
 * (e.g., INVALID_ARGUMENT, ALREADY_EXISTS, PERMISSION_DENIED).
 * The retry loop will rethrow this immediately without further attempts.
 */
class NonRetryableRpcError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Retry policy for cross-shard RPC calls.
 *
 * Classifies gRPC status codes into retryable and non-retryable categories
 * and integrates with the CircuitBreaker to prevent retry storms.
 *
 * Retryable status codes:
 *   UNAVAILABLE, DEADLINE_EXCEEDED, RESOURCE_EXHAUSTED, ABORTED, INTERNAL
 *
 * Non-retryable status codes (raise NonRetryableRpcError immediately):
 *   INVALID_ARGUMENT, NOT_FOUND, ALREADY_EXISTS, PERMISSION_DENIED,
 *   UNAUTHENTICATED, FAILED_PRECONDITION, OUT_OF_RANGE, UNIMPLEMENTED
 */
struct ShardRpcRetryPolicy {
    int max_attempts         = 3;     ///< Maximum total attempts (1 + retries).
    int initial_backoff_ms   = 100;   ///< Initial delay before first retry.
    int max_backoff_ms       = 5000;  ///< Upper cap for exponential back-off.
    bool use_circuit_breaker = true;  ///< Gate retries through the circuit breaker.
};

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
    /** @brief Runtime configuration for shard RPC transport, retries and TLS. */
    struct Config {
        std::string endpoint;           // Shard endpoint (e.g., "shard1:50051" for gRPC)
        std::string shard_id;           // Local shard identifier used for metric labels
        int timeout_ms = 5000;          // RPC timeout in milliseconds
        int max_retries = 3;            // Maximum retry attempts
        int retry_delay_ms = 100;       // Initial delay between retries (exponential backoff)
        
        // mTLS Configuration (optional, required for production)
        bool enable_mtls = true;        // Enable mutual TLS authentication (default: on)
        std::string tls_cert_path;      // Path to client certificate (PEM format)
        std::string tls_key_path;       // Path to client private key (PEM format)
        std::string tls_ca_cert_path;   // Path to CA certificate for server verification (PEM format)
        bool tls_verify_server = true;  // Verify server certificate against CA

        // Connection pool (optional, shared across ShardRPCClient instances)
        MTLSConnectionPoolManager* connection_pool = nullptr; // Non-owning; nullptr disables pooling
        int max_pool_connections = 50;  // Per-endpoint pool size; propagated via GossipConfigManager

        // Circuit Breaker Configuration
        bool enable_circuit_breaker = true;          // Enable circuit breaker protection
        int circuit_breaker_failure_threshold = 5;   // Failures before opening circuit
        int circuit_breaker_recovery_ms = 5000;      // Milliseconds before half-open probe (default 5 s)

        // Metrics (optional, non-owning pointers; nullptr disables the respective metric sink)
        themisdb::sharding::OperationalMetrics* operational_metrics = nullptr;
        PrometheusMetrics*  prometheus_metrics  = nullptr;
    };
    
    /** @brief Construct RPC client using provided transport/runtime configuration. */
    explicit ShardRPCClient(const Config& config);
    /** @brief Destroy client and release internal connection/circuit-breaker state. */
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
     * @brief Send COMPENSATE request (SAGA compensation)
     * @param txn_id Transaction ID
     * @param operation Compensation operation JSON payload
     * @return true if compensation executed successfully
     */
    bool compensate(
        const std::string& txn_id,
        const nlohmann::json& operation
    );
    
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
     * @brief Replicate (write) a single entity to this shard.
     *
     * Uses the gRPC ReplicateData RPC in multi-node deployments and a
     * lightweight in-process acknowledgement for loopback/test endpoints.
     *
     * @param collection  Collection / namespace name
     * @param uuid        Entity UUID
     * @param data        Entity payload (JSON object)
     * @param timestamp_ns Write timestamp (nanoseconds since epoch); 0 = now
     * @return true if the entity was accepted by the remote shard
     */
    bool writeEntity(
        const std::string& collection,
        const std::string& uuid,
        const nlohmann::json& data,
        uint64_t timestamp_ns = 0
    );
    
    /**
     * @brief Check if shard is available
     */
    bool ping();

    /**
     * @brief A single directed wait-for edge returned by collectWaitForEdges().
     *
     * waiting_transaction_id is blocked by blocking_transaction_id on this shard.
     */
    struct WaitForEdge {
        std::string waiting_transaction_id;
        std::string blocking_transaction_id;
    };

    /**
     * @brief Pull all active local wait-for edges from the remote shard.
     *
     * Used by the coordinator's deadlock-detection thread to build a
     * cluster-wide wait-for graph that supplements edges explicitly reported
     * via CrossShardTransactionCoordinator::reportDistributedWait().
     *
     * @return List of wait-for edges currently active on the remote shard.
     *         Returns an empty list on RPC failure (fail-open: callers should
     *         not abort transactions solely on the basis of a missing response).
     */
    std::vector<WaitForEdge> collectWaitForEdges();

    /**
     * @brief Inject a custom response handler for the in-process simulation path.
     *
     * When set, @p handler is called by sendRequestInProcess() instead of the
     * built-in hardcoded responses.  This enables test and integration code to:
     *   - simulate NACK/abort responses ("vote": "abort")
     *   - inject network-timeout exceptions
     *   - exercise partial-failure and circuit-breaker code paths
     *
     * The handler receives the RPC method name (e.g. "prepare", "commit") and
     * the full @p params JSON and must return a well-formed response JSON.
     * Pass @c nullptr to restore the built-in hardcoded fallback behaviour.
     *
     * Thread safety: the handler is stored under the impl mutex; it is safe to
     * call this method from any thread before or during RPC use.
     *
     * @param handler  Callable matching @c nlohmann::json(std::string, nlohmann::json),
     *                 or @c nullptr to clear.
     */
    using InProcessResponseHandler =
        std::function<nlohmann::json(std::string /*method*/,
                                     nlohmann::json /*params*/)>;

    void setInProcessResponseHandler(InProcessResponseHandler handler);

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
     * @brief Handle gRPC write-entity request (ReplicateData RPC)
     */
    nlohmann::json handleWriteEntityGrpc(
        grpc::ClientContext& context,
        const nlohmann::json& params
    );
    
    /**
     * @brief Handle gRPC healthcheck request
     */
    nlohmann::json handleHealthCheckGrpc(
        grpc::ClientContext& context
    );

    nlohmann::json handleCollectWaitForEdgesGrpc(
        grpc::ClientContext& context
    );
    
    /**
     * @brief Determine if a gRPC error is retryable
     */
    bool isRetryableError(grpc::StatusCode code);
#endif
};

} // namespace themis::sharding
