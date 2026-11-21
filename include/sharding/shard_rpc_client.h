/**
 * @file shard_rpc_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <functional>
#include <utility>
#include <nlohmann/json.hpp>
#include "sharding/epoch_fencing.h"
#include "sharding/consensus_module.h"
#include "sharding/raft_log.h"

#ifdef THEMIS_ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#endif

namespace themis::sharding {

// Forward declarations
class PrometheusMetrics;
class MTLSConnectionPoolManager;
struct ShardTransferManifest;

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
        
        // mTLS is mandatory in production. The insecure override is honored only outside production.
        bool enable_mtls = true;
        bool allow_insecure = false;
        std::string tls_cert_path;      // Path to client certificate (PEM format)
        std::string tls_key_path;       // Path to client private key (PEM format)
        std::string tls_ca_cert_path;   // Path to CA certificate for server verification (PEM format)
        bool tls_verify_server = true;  // Verify server certificate against CA
        std::string local_node_id;      ///< Raft member ID that must match each vote/leader request.
        std::string expected_peer_node_id;  ///< Raft member ID required in each RPC response.
        std::string expected_peer_identity_property;  ///< Must be `x509_subject_alternative_name` for Raft TLS.
        std::string expected_peer_identity_value;     ///< DNS SAN used for TLS server-name verification.

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
    
    /**
     * @brief Constructs an RPC client using the provided transport/runtime configuration.
     * @param config Endpoint, retry, and TLS settings; production requires mTLS and server verification.
     * @throws std::runtime_error if production transport requirements are not met or secure
     *         credential loading fails without a non-production insecure override.
     */
    explicit ShardRPCClient(const Config& config);
    /** @brief Destroy client and release internal connection/circuit-breaker state. */
    ~ShardRPCClient();
    
    // Disable copy, allow move
    ShardRPCClient(const ShardRPCClient&) = delete;
    ShardRPCClient& operator=(const ShardRPCClient&) = delete;
    ShardRPCClient(ShardRPCClient&&) noexcept = default;
    ShardRPCClient& operator=(ShardRPCClient&&) noexcept = default;
    
    /**
     * @brief Send PREPARE request (2PC Phase 1)
     * @param txn_id Transaction ID
     * @param operations Operations to prepare
     * @param fencing_token Current credential scoped to this target shard
     * @return true if shard votes to commit
     */
    bool prepare(
        const std::string& txn_id,
        const nlohmann::json& operations,
        const EpochToken& fencing_token
    );

    /**
     * @brief Reject an unfenced PREPARE request.
     * @param txn_id Transaction ID
     * @param operations Operations that would have been prepared
     * @return Always false because no fencing credential was supplied
     */
    bool prepare(const std::string& txn_id, const nlohmann::json& operations);
    
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
     * @brief Send an authenticated Raft vote request to one configured member.
     * @param raft_group_id Logical group routed to the remote state machine.
     * @param expected_peer_node_id Expected remote Raft member; must match this
     *        client's explicit TLS identity binding.
     * @param request Vote request; candidate_id must match local_node_id.
     * @return Remote vote response after the RPC, peer certificate identity,
     *         and non-stale response term are verified. A higher term is returned
     *         unchanged so Raft can durably step down.
     * @throws std::runtime_error for missing gRPC, unverified TLS, identity
     *         mismatch, timeout, or RPC failure.
     * @note The unary call is not retried and uses a positive deadline capped at
     *       five seconds; a higher response term is preserved for Raft step-down.
     */
    [[nodiscard]] themisdb::sharding::VoteResponse requestRaftVote(
        const std::string& raft_group_id,
        const std::string& expected_peer_node_id,
        const themisdb::sharding::VoteRequest& request);

    /**
     * @brief Send authenticated Raft AppendEntries to one configured member.
     * @param raft_group_id Logical group routed to the remote state machine.
     * @param expected_peer_node_id Expected remote Raft member; must match this
     *        client's explicit TLS identity binding.
     * @param request AppendEntries request; leader_id must match local_node_id,
     *        with at most 256 entries, 1 MiB per command, and 3 MiB aggregate
     *        command payload.
     * @return Follower response. `success` is meaningful only after transport,
     *         peer identity, response term, and complete matched prefix are
     *         validated. A higher term is returned unchanged so Raft can durably
     *         step down.
     * @throws std::runtime_error for missing gRPC, unverified TLS, identity
     *         mismatch, timeout, or RPC failure.
     * @note The unary call is not retried and uses a positive deadline capped at
     *       five seconds; a higher response term is preserved for Raft step-down.
     */
    [[nodiscard]] themisdb::sharding::AppendEntriesResponse appendRaftEntries(
        const std::string& raft_group_id,
        const std::string& expected_peer_node_id,
        const themisdb::sharding::AppendEntriesRequest& request);

    /**
     * @brief Stage and verify an immutable logical range export over mTLS.
     * @param manifest Exact, externally authorized transfer assignment and export digest.
     * @param read_chunk Supplies payload and lowercase SHA-256 for a requested zero-based
     *        chunk index; called synchronously, starting at the receiver's durable checkpoint.
     * @param on_admitted Optional synchronous notification after durable PREPARE.
     * @return True only after the receiver explicitly confirms verification of persisted
     *         contents. This does not change shard ownership.
     * @throws std::runtime_error on transport/status failure; a lost response may follow
     *         durable progress. Retry with the identical immutable export to resume.
     * @note Each RPC uses the configured positive deadline, capped at five minutes.
     *       The callbacks must themselves be bounded and must not block on this client.
     */
    [[nodiscard]] bool transferShard(
        const ShardTransferManifest& manifest,
        const std::function<std::pair<std::string, std::string>(uint64_t)>& read_chunk,
        const std::function<void()>& on_admitted = {});

    /**
     * @brief Request terminal cancellation of a staged transfer over verified mTLS.
     * @param migration_id Identifier from the original durable assignment.
     * @return True only when the target acknowledges durable abort.
     * @throws std::runtime_error on RPC failure; a lost response is not proof of abort.
     */
    [[nodiscard]] bool abortShardTransfer(const std::string& migration_id);

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
     * The in-process transport is compiled for test-mode builds only; production
     * and ordinary development builds require a real gRPC endpoint.
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
