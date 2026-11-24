/**
 * @file shard_rpc_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include "sharding/shard_rpc_client.h"

namespace themisdb::sharding {
class RaftConsensus;
}

namespace themis::sharding {

class ShardTransferStore;

/**
 * @brief gRPC Server for handling inter-shard RPC requests
 * 
 * v1.3.4: Added for multi-node cluster deployments
 * Handles incoming RPC requests from other shards:
 * - Distributed transactions (2PC: prepare, commit, abort)
 * - Data replication
 * - Health checks
 */
class ShardRPCServer {
public:
    /**
     * @brief Health information returned by health check
     */
    struct HealthInfo {
        bool is_healthy = true;
        std::string version;
        uint64_t uptime_seconds = 0;
    };

    /**
     * @brief Maps one authenticated gRPC X.509 identity value to a shard ID.
     * @param property_name Exact auth-context property name, such as
     *        `x509_common_name` or `x509_subject_alternative_name`.
     * @param property_value Exact value returned by gRPC for that property.
     * @param coordinator_shard_id Canonical shard ID allowed to issue PREPARE.
     * @note Bindings are explicit; subject/SAN values are not normalized or
     *       treated as shard IDs unless configured here.
     */
    struct CoordinatorIdentityBinding {
        std::string property_name;
        std::string property_value;
        std::string coordinator_shard_id;
    };

    /**
     * @brief Maps an exact authenticated X.509 identity to a Raft member node ID.
     * @note The claimed candidate/leader ID in each Raft RPC must equal this
     *       mapping and be a member of the resolved Raft group's configuration.
     */
    struct RaftIdentityBinding {
        std::string property_name;   ///< Exact authenticated gRPC X.509 property name.
        std::string property_value;  ///< Exact certificate identity value for that property.
        std::string node_id;         ///< Configured Raft member ID represented by the certificate.
    };
    
    /**
     * @brief Configuration for the RPC server
     */
    /** @brief Runtime configuration for shard RPC server endpoint and TLS policy. */
    struct Config {
        std::string listen_address;     // Address to listen on (e.g., "0.0.0.0:50051")
        
        // mTLS is mandatory in production. Server certificate and key are always
        // required when enabled; a CA file is additionally required for client verification.
        bool enable_mtls = true;
        std::string tls_cert_path;      // Path to server certificate (PEM format)
        std::string tls_key_path;       // Path to server private key (PEM format)
        std::string tls_ca_cert_path;   // Path to CA certificate for client verification (PEM format)
        bool tls_require_client_cert = true;
        /**
         * @brief Trusted X.509 identity-to-shard mappings for PREPARE RPCs.
         * @note Multiple identity values may map to the same shard to support
         *       certificate rotation. Unmapped or ambiguously mapped identities
         *       are rejected before the request handler runs.
         */
        std::vector<CoordinatorIdentityBinding> coordinator_identity_bindings;

        /// Explicit X.509 identities allowed to act as transfer principals;
        /// the binding's ID is checked against the durable assignment.
        std::vector<CoordinatorIdentityBinding> transfer_identity_bindings;

        /// Optional shared staging store. Without it, transfer RPCs fail closed.
        std::shared_ptr<ShardTransferStore> transfer_store;

        /**
         * @brief Maps exact authenticated X.509 identities to storage principals
         *        allowed to request tensor fragments.
         */
        struct FragmentFetchIdentityBinding {
            std::string property_name;   ///< Exact authenticated X.509 property name.
            std::string property_value;  ///< Exact value provided by gRPC authentication.
            std::string principal_id;    ///< Storage authorization principal for this identity.
        };

        /**
         * @brief Reads immutable bytes from the authoritative local payload store.
         * @param principal_id Authenticated principal resolved from mTLS identity.
         * @param shard_id Local shard ID configured on this server.
         * @param artifact_id Opaque artifact ID; never interpreted as a path.
         * @param max_bytes Request's enforced response-size ceiling.
         * @return Payload and its lowercase SHA-256, or empty when not found.
         * @note Called concurrently by gRPC workers. Implementations must be
         *       thread-safe, enforce tenant/authorization rules not represented
         *       by the principal, honor max_bytes, and never return partial data.
         */
        using TensorFragmentReader = std::function<std::optional<TensorFragmentPayload>(
            const std::string& principal_id,
            const std::string& shard_id,
            const std::string& artifact_id,
            uint64_t max_bytes)>;

        /// Explicit Raft-group resolver; called concurrently and must be thread-safe.
        /// Returning null rejects the RPC; resolver exceptions fail the RPC.
        std::function<std::shared_ptr<themisdb::sharding::RaftConsensus>(
            const std::string& raft_group_id)> raft_group_resolver;

        /// Exact X.509 identity-to-member mappings required when Raft is enabled.
        std::vector<RaftIdentityBinding> raft_identity_bindings;

        /// Resolve the durable snapshot directory for the requested Raft group.
        std::function<std::shared_ptr<themisdb::sharding::RaftSnapshotManager>(
            const std::string& raft_group_id)> raft_snapshot_manager_resolver;

        /**
         * @brief Restore one verified snapshot before the server acknowledges installation.
         * @param raft Authenticated group's local consensus instance.
         * @param snapshots Manager where the verified snapshot file is installed.
         * @param leader_id Authenticated Raft member that sent the stream.
         * @param leader_term Sender's current Raft term.
         * @param snapshot Fully parsed snapshot with validated checksums;
         *        `data` remains Zstandard-compressed and must be decoded by the
         *        application restore implementation.
         * @return True only after application state, the durable Raft journal
         *         boundary/hard state, and any live 2PC decision store are
         *         restored and consistent; false rejects the install.
         * @note Implementations must make activation crash-recoverable and must
         *       not expose partial state. The server verifies the resulting Raft
         *       boundary before returning a successful RPC response.
         */
        using RaftSnapshotInstallHandler = std::function<bool(
            themisdb::sharding::RaftConsensus& raft,
            themisdb::sharding::RaftSnapshotManager& snapshots,
            const std::string& leader_id,
            uint64_t leader_term,
            const themisdb::sharding::RaftSnapshot& snapshot)>;
        RaftSnapshotInstallHandler raft_snapshot_install_handler;

        /// Hard limit for received Raft snapshot stream bytes.
        uint64_t max_raft_snapshot_bytes = 512ULL * 1024ULL * 1024ULL;
        /// Maximum deadline accepted for snapshot installation RPCs.
        uint32_t raft_snapshot_max_deadline_ms = 300000;

        /// Local shard identity served by the fragment reader.
        std::string tensor_fragment_shard_id;
        /// Storage-owner callback. If absent, the fragment RPC is unavailable.
        TensorFragmentReader tensor_fragment_reader;
        /// Explicit client certificate identities allowed to invoke the reader.
        std::vector<FragmentFetchIdentityBinding> tensor_fragment_identity_bindings;
        /// Hard response limit enforced before streaming.
        uint64_t max_tensor_fragment_bytes = 64ULL * 1024ULL * 1024ULL;
        /// Maximum bytes sent in one gRPC response frame.
        uint32_t tensor_fragment_chunk_bytes = 1024U * 1024U;
        /// Hard server-side RPC deadline limit.
        uint32_t tensor_fragment_max_deadline_ms = 30000;
    };
    
    /**
     * @brief Handler interface for processing incoming RPC requests
     * 
     * Implement this interface to handle RPC requests in your application
     */
    class RequestHandler {
    public:
        virtual ~RequestHandler() = default;
        
        /**
         * @brief Handle prepare phase of 2PC
         * @param transaction_id Transaction ID
         * @param coordinator_shard_id ID of the coordinator shard
         * @param transaction_data Serialized transaction data
         * @return true to vote commit, false to vote abort
         * @note For gRPC requests, the server invokes this only after the authenticated
         *       an explicitly configured X.509 property/value binding resolves to
         *       @p coordinator_shard_id; requests without a verified, configured,
         *       unambiguous identity or with a mismatch are rejected.
         */
        virtual bool onPrepare(
            const std::string& transaction_id,
            const std::string& coordinator_shard_id,
            const std::string& transaction_data
        ) = 0;
        
        /**
         * @brief Handle commit phase of 2PC
         * @param transaction_id Transaction ID
         * @return true if committed successfully
         */
        virtual bool onCommit(const std::string& transaction_id) = 0;
        
        /**
         * @brief Handle abort request
         * @param transaction_id Transaction ID
         * @return true if aborted successfully
         */
        virtual bool onAbort(const std::string& transaction_id) = 0;
        
        /**
         * @brief Handle health check request
         * @return Health information
         */
        virtual HealthInfo onHealthCheck() = 0;

        /**
         * @brief Collect all active local wait-for edges for distributed deadlock detection.
         *
         * The coordinator calls this on every shard during each deadlock-detection
         * cycle to build a cluster-wide wait-for graph.  Each entry in the returned
         * vector represents a directed edge: @p waiting_transaction_id is blocked by
         * @p blocking_transaction_id on this shard.
         *
         * Default implementation returns an empty list (no local lock waits known).
         * Override in shards that maintain a local lock-wait state.
         */
        virtual std::vector<ShardRPCClient::WaitForEdge> onCollectWaitForEdges() {
            return {};
        }
    };
    
    /**
     * @brief Create a new ShardRPCServer
     * @param listen_address Address to listen on (e.g., "0.0.0.0:50051")
     */
    /** @brief Construct server using listen address only (default TLS config). */
    explicit ShardRPCServer(const std::string& listen_address);
    
    /**
     * @brief Create a new ShardRPCServer with configuration
     * @param config Server configuration including mTLS settings
     */
    /** @brief Construct server using explicit runtime configuration. */
    explicit ShardRPCServer(const Config& config);
    
    ~ShardRPCServer();
    
    // Disable copy and move
    ShardRPCServer(const ShardRPCServer&) = delete;
    ShardRPCServer& operator=(const ShardRPCServer&) = delete;
    ShardRPCServer(ShardRPCServer&&) = delete;
    ShardRPCServer& operator=(ShardRPCServer&&) = delete;
    
    /**
     * @brief Set the request handler for processing incoming RPCs
     * @param handler Pointer to handler (must remain valid while server is running)
     */
    void setRequestHandler(RequestHandler* handler);
    
    /**
     * @brief Starts the gRPC server after validating its transport credentials.
     * @return True when the listener starts; false when gRPC is unavailable, the
     *         configuration is unsafe, credential loading fails, or the listener fails.
     * @note Production mode requires mTLS and client-certificate verification. Invalid or
     *       missing credentials never fall back to an insecure listener.
     */
    bool start();
    
    /**
     * @brief Stop the gRPC server
     */
    void stop();
    
    /**
     * @brief Wait for the server to shutdown (blocks)
     */
    void wait();
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::sharding
