/**
 * @file shard_rpc_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include "sharding/shard_rpc_client.h"

namespace themis::sharding {

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
     * @brief Configuration for the RPC server
     */
    /** @brief Runtime configuration for shard RPC server endpoint and TLS policy. */
    struct Config {
        std::string listen_address;     // Address to listen on (e.g., "0.0.0.0:50051")
        
        // mTLS Configuration (optional, required for production)
        bool enable_mtls = true;        // Enable mutual TLS authentication (default: on)
        std::string tls_cert_path;      // Path to server certificate (PEM format)
        std::string tls_key_path;       // Path to server private key (PEM format)
        std::string tls_ca_cert_path;   // Path to CA certificate for client verification (PEM format)
        bool tls_require_client_cert = true;  // Require client certificates (mutual TLS)
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
     * @brief Start the gRPC server
     * @return true if started successfully
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
