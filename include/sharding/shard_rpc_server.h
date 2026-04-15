/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_rpc_server.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     155                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <cstdint>

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
    struct Config {
        std::string listen_address;     // Address to listen on (e.g., "0.0.0.0:50051")
        
        // mTLS Configuration (optional, required for production)
        bool enable_mtls = false;       // Enable mutual TLS authentication
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
    };
    
    /**
     * @brief Create a new ShardRPCServer
     * @param listen_address Address to listen on (e.g., "0.0.0.0:50051")
     */
    explicit ShardRPCServer(const std::string& listen_address);
    
    /**
     * @brief Create a new ShardRPCServer with configuration
     * @param config Server configuration including mTLS settings
     */
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
