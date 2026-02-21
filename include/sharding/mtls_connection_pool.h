/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mtls_connection_pool.h                             ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     278                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <optional>
#include <chrono>
#include <atomic>
#include <queue>
#include <set>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>

// Forward declarations for OpenSSL SSL type
typedef struct ssl_st SSL;

namespace themis::sharding {

// Custom deleter for SSL pointers
struct SSLDeleter {
    void operator()(SSL* ptr) const;
};

/**
 * @brief Per-Endpoint Connection Pool with dynamic scaling
 * 
 * Features:
 * - Dynamic scaling between min and max connections
 * - Connection validation and health checks
 * - TTL-based connection expiration
 * - Thread-safe connection pooling
 */
class EndpointConnectionPool {
public:
    /**
     * @brief Configuration for endpoint connection pool
     */
    struct Config {
        // Connection pool sizing
        size_t min_connections = 2;              // Minimum pre-allocated connections
        size_t max_connections = 50;             // Maximum connections per endpoint
        std::chrono::seconds connection_ttl{300}; // Connection time-to-live
        std::chrono::seconds idle_timeout{60};   // Close idle connections after 60s
        
        // Health checks
        bool enable_health_checks = true;
        std::chrono::seconds health_check_interval{30};
    };
    
    /**
     * @brief Pool statistics
     */
    struct Statistics {
        size_t active_connections;      // Currently in use
        size_t idle_connections;        // Available in pool
        size_t total_created;           // Total created since start
        size_t connections_failed;      // Failed connections
        double utilization_percent;     // (active / max) * 100
    };
    
    /**
     * @brief Construct endpoint connection pool
     * @param endpoint Target endpoint (e.g., "localhost:50051")
     * @param config Pool configuration
     */
    explicit EndpointConnectionPool(
        const std::string& endpoint,
        const Config& config
    );
    
    /**
     * @brief Destructor - cleanup resources
     */
    ~EndpointConnectionPool();
    
    // Prevent copying and moving
    EndpointConnectionPool(const EndpointConnectionPool&) = delete;
    EndpointConnectionPool& operator=(const EndpointConnectionPool&) = delete;
    EndpointConnectionPool(EndpointConnectionPool&&) = delete;
    EndpointConnectionPool& operator=(EndpointConnectionPool&&) = delete;
    
    /**
     * @brief Get a connection from pool (blocking with timeout)
     * @param timeout Maximum time to wait for connection
     * @return SSL connection or nullopt if timeout/failure
     */
    std::optional<std::unique_ptr<SSL, SSLDeleter>> getConnection(
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );
    
    /**
     * @brief Return connection to pool for reuse
     * @param connection SSL connection to return
     */
    void releaseConnection(std::unique_ptr<SSL, SSLDeleter> connection);
    
    /**
     * @brief Mark connection as failed (will be replaced)
     * @param connection SSL connection pointer to invalidate
     */
    void invalidateConnection(SSL* connection);
    
    /**
     * @brief Get pool statistics
     * @return Current pool statistics
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Explicitly warm up pool to minimum connections
     * @return true if successful
     */
    bool warmUp();
    
    /**
     * @brief Close all connections
     */
    void closeAll();
    
private:
    /**
     * @brief Pooled connection with metadata
     */
    struct PooledConnection {
        std::unique_ptr<SSL, SSLDeleter> ssl;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_used;
        bool is_valid = true;
    };
    
    // Connection lifecycle management
    std::optional<std::unique_ptr<SSL, SSLDeleter>> createNewConnection();
    bool validateConnection(SSL* conn);
    bool isConnectionExpired(const PooledConnection& pooled);
    void cleanupExpiredConnections();
    void cleanupLoop();
    
    std::string endpoint_;
    Config config_;
    
    // Thread-safe queue for idle connections
    std::queue<PooledConnection> idle_pool_;
    std::set<SSL*> active_connections_;
    
    mutable std::shared_mutex pool_mutex_;
    std::condition_variable_any available_cv_;
    
    // Statistics
    std::atomic<uint64_t> total_created_{0};
    std::atomic<uint64_t> connections_failed_{0};
    
    // Background cleanup thread
    std::thread cleanup_thread_;
    std::atomic<bool> running_{false};
};

/**
 * @brief Global Connection Pool Manager for all endpoints
 * 
 * Manages per-endpoint connection pools with global resource limits
 */
class MTLSConnectionPoolManager {
public:
    /**
     * @brief Configuration for pool manager
     */
    struct Config {
        EndpointConnectionPool::Config endpoint_config;
        
        // Global limits
        size_t max_total_connections = 500;     // Across all endpoints
        
        // Eviction policy
        bool enable_endpoint_eviction = true;
        size_t max_endpoints = 100;             // Max cached endpoint pools
    };
    
    /**
     * @brief Global statistics
     */
    struct GlobalStatistics {
        size_t total_active_connections;
        size_t total_idle_connections;
        size_t cached_endpoint_pools;
        std::map<std::string, EndpointConnectionPool::Statistics> per_endpoint_stats;
    };
    
    /**
     * @brief Construct pool manager with configuration
     * @param config Manager configuration
     */
    explicit MTLSConnectionPoolManager(const Config& config);
    MTLSConnectionPoolManager();
    
    /**
     * @brief Destructor
     */
    ~MTLSConnectionPoolManager();
    
    // Prevent copying and moving
    MTLSConnectionPoolManager(const MTLSConnectionPoolManager&) = delete;
    MTLSConnectionPoolManager& operator=(const MTLSConnectionPoolManager&) = delete;
    MTLSConnectionPoolManager(MTLSConnectionPoolManager&&) = delete;
    MTLSConnectionPoolManager& operator=(MTLSConnectionPoolManager&&) = delete;
    
    /**
     * @brief Get pool for specific endpoint (creates if needed)
     * @param endpoint Target endpoint
     * @return Shared pointer to endpoint pool
     */
    std::shared_ptr<EndpointConnectionPool> getPool(const std::string& endpoint);
    
    /**
     * @brief Get connection directly (convenience method)
     * @param endpoint Target endpoint
     * @param timeout Maximum wait time
     * @return SSL connection or nullopt
     */
    std::optional<std::unique_ptr<SSL, SSLDeleter>> getConnection(
        const std::string& endpoint,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );
    
    /**
     * @brief Return connection to appropriate pool
     * @param endpoint Target endpoint
     * @param conn SSL connection to return
     */
    void releaseConnection(const std::string& endpoint, std::unique_ptr<SSL, SSLDeleter> conn);
    
    /**
     * @brief Get global statistics
     * @return Global pool statistics
     */
    GlobalStatistics getStatistics() const;
    
    /**
     * @brief Close all connections and clear pools
     */
    void shutdown();
    
private:
    std::map<std::string, std::shared_ptr<EndpointConnectionPool>> pools_;
    mutable std::shared_mutex pools_mutex_;
    
    Config config_;
    std::atomic<uint64_t> total_connections_{0};
};

} // namespace themis::sharding
