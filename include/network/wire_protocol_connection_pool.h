/**
 * @file wire_protocol_connection_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol Connection Pool
// Client-side connection pooling for Wire Protocol connections

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace network {

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// =============================================================================
// Pluggable Pool Sizing Strategy
// =============================================================================

/**
 * @brief Abstract interface for connection pool sizing strategies.
 *
 * Implementations decide when to grow or shrink the pool based on
 * real-time utilization metrics supplied by the pool itself.
 */
class IPoolingStrategy {
public:
    virtual ~IPoolingStrategy() = default;

    /**
     * @brief Compute the ideal total connection count for a target.
     *
     * @param current_count  Current total connections (active + idle)
     * @param active_count   Connections currently in use
     * @param load           Utilisation ratio (active / total), range [0, 1]
     * @return Desired total connection count
     */
    virtual size_t getIdealConnectionCount(
        size_t current_count,
        size_t active_count,
        double load) = 0;

    /**
     * @brief Return true if a new connection should be pre-created.
     *
     * @param current_count   Current total connections
     * @param max_count       Configured maximum
     * @param available_count Idle connections available for immediate use
     */
    virtual bool shouldCreateConnection(
        size_t current_count,
        size_t max_count,
        size_t available_count) = 0;

    /**
     * @brief Return true if the oldest idle connection should be removed.
     *
     * @param current_count   Current total connections
     * @param min_count       Configured minimum
     * @param available_count Idle connections available
     * @param idle_time       Time the oldest idle connection has been idle
     */
    virtual bool shouldRemoveConnection(
        size_t current_count,
        size_t min_count,
        size_t available_count,
        std::chrono::seconds idle_time) = 0;
};

/**
 * @brief Adaptive pool sizing strategy based on utilization thresholds.
 *
 * - Scales up when the fraction of idle connections drops below
 *   `(1 - scale_up_threshold)`.
 * - Scales down when idle connections have been idle longer than
 *   `min_idle_time` and the pool is above `min_count`.
 */
class AdaptivePoolingStrategy : public IPoolingStrategy {
public:
    struct Config {
        /// Target utilization (fraction of connections that should be active).
        double target_utilization  = 0.7;
        /// Trigger scale-up when utilization exceeds this value.
        double scale_up_threshold  = 0.8;
        /// Trigger scale-down when utilization drops below this value.
        double scale_down_threshold = 0.3;
        /// Multiplier applied when growing the pool.
        double scale_up_factor     = 1.5;
        /// Multiplier applied when shrinking the pool.
        double scale_down_factor   = 0.7;
        /// An idle connection must have been unused for at least this long
        /// before it is eligible for removal.
        std::chrono::seconds min_idle_time{300};
    };

    explicit AdaptivePoolingStrategy(const Config& config);
    AdaptivePoolingStrategy();

    size_t getIdealConnectionCount(
        size_t current_count,
        size_t active_count,
        double load) override;

    bool shouldCreateConnection(
        size_t current_count,
        size_t max_count,
        size_t available_count) override;

    bool shouldRemoveConnection(
        size_t current_count,
        size_t min_count,
        size_t available_count,
        std::chrono::seconds idle_time) override;

    const Config& strategyConfig() const { return config_; }

private:
    Config config_;
};

/**
 * @brief Socket wrapper supporting both plain and SSL sockets
 */
class SocketWrapper {
public:
    SocketWrapper(std::shared_ptr<tcp::socket> plain_socket);
    SocketWrapper(std::shared_ptr<ssl::stream<tcp::socket>> ssl_socket);
    
    bool is_open() const;
    void close(boost::system::error_code& ec);
    
    // Check if this is an SSL socket
    bool is_ssl() const { return ssl_socket_ != nullptr; }
    
    // Get the plain or SSL socket
    tcp::socket* plain_socket() { return plain_socket_.get(); }
    ssl::stream<tcp::socket>* ssl_socket() { return ssl_socket_.get(); }
    
private:
    std::shared_ptr<tcp::socket> plain_socket_;
    std::shared_ptr<ssl::stream<tcp::socket>> ssl_socket_;
};

/**
 * @brief Wire Protocol Connection Pool for client-side pooling
 * 
 * Provides:
 * - TCP connection pooling (reduce handshake overhead)
 * - TLS/mTLS support for secure connections
 * - Keep-Alive support
 * - Per-target connection pooling
 * - Automatic reconnection
 * - Connection health checks
 * - Thread-safe access
 * 
 * Performance Benefits:
 * - Reduces TCP handshake overhead (3-way handshake)
 * - Reuses authenticated sessions
 * - Better throughput under high concurrency
 * - Lower latency for subsequent requests
 * 
 * @see docs/wire-protocol.md for protocol documentation
 * @see docs/knowledge-base/PERFORMANCE_TIPS.md (Connection Pooling section)
 */
class WireProtocolConnectionPool {
public:
    struct Config {
        size_t min_connections_per_target = 2;        ///< Minimum connections per target
        size_t max_connections_per_target = 20;       ///< Maximum connections per target
        std::chrono::seconds idle_timeout{60};        ///< Connection idle timeout
        std::chrono::seconds connect_timeout{5};      ///< Connection timeout
        std::chrono::seconds acquire_timeout{10};     ///< Timeout for acquiring connection
        std::chrono::seconds keepalive_interval{30};  ///< Keepalive ping interval
        bool enable_ssl = false;                      ///< Enable SSL/TLS
        bool enable_mtls = false;                     ///< Enable mutual TLS
        std::string ssl_cert_path;                    ///< Client certificate path
        std::string ssl_key_path;                     ///< Client key path
        std::string ssl_ca_cert_path;                 ///< CA certificate path
        size_t max_retries = 3;                       ///< Max connection retry attempts
        bool enable_warmup = true;                    ///< Pre-create min connections on startup
        /// Enable adaptive pool sizing driven by real-time utilization metrics.
        bool enable_adaptive_sizing = false;
        /// Custom sizing strategy (default: AdaptivePoolingStrategy with default config).
        /// Only consulted when enable_adaptive_sizing is true.
        std::shared_ptr<IPoolingStrategy> adaptive_strategy;
    };
    
    explicit WireProtocolConnectionPool(const Config& config);
    WireProtocolConnectionPool();
    ~WireProtocolConnectionPool();
    
    // Disable copy and move (maintenance thread captures this)
    WireProtocolConnectionPool(const WireProtocolConnectionPool&) = delete;
    WireProtocolConnectionPool& operator=(const WireProtocolConnectionPool&) = delete;
    WireProtocolConnectionPool(WireProtocolConnectionPool&&) = delete;
    WireProtocolConnectionPool& operator=(WireProtocolConnectionPool&&) = delete;
    
    /**
     * @brief Pooled connection handle (RAII)
     * 
     * Automatically returns connection to pool when destroyed.
     */
    class ConnectionHandle {
    public:
        ConnectionHandle(
            std::shared_ptr<SocketWrapper> socket,
            WireProtocolConnectionPool* pool,
            const std::string& target
        );
        ~ConnectionHandle();
        
        // Disable copy, allow move
        ConnectionHandle(const ConnectionHandle&) = delete;
        ConnectionHandle& operator=(const ConnectionHandle&) = delete;
        ConnectionHandle(ConnectionHandle&& other) noexcept;
        ConnectionHandle& operator=(ConnectionHandle&& other) noexcept;
        
        // Backward-compatible accessor returning the underlying TCP socket
        // Only works for plain (non-SSL) sockets
        tcp::socket& socket() { 
            if (!socket_->is_ssl() && socket_->plain_socket()) {
                return *socket_->plain_socket();
            }
            throw std::runtime_error("socket() accessor not available for SSL connections; use socketWrapper() instead");
        }
        
        // Access to the SocketWrapper for advanced usage (SSL vs plain)
        SocketWrapper& socketWrapper() { return *socket_; }
        
        bool isValid() const { return socket_ && socket_->is_open(); }
        
    private:
        std::shared_ptr<SocketWrapper> socket_;
        WireProtocolConnectionPool* pool_;
        std::string target_;
    };
    
    /**
     * @brief Acquire a connection for the given target
     * @param target Target address (e.g., "localhost:8766")
     * @return Connection handle (RAII)
     * @throws std::runtime_error if unable to acquire connection
     */
    ConnectionHandle acquireConnection(const std::string& target);
    
    /**
     * @brief Get pool statistics
     */
    struct Stats {
        size_t total_connections = 0;
        size_t available_connections = 0;
        size_t in_use_connections = 0;
        size_t stale_connections_removed = 0;
        size_t failed_connections = 0;
        size_t acquire_timeouts = 0;
        size_t connections_created = 0;
        size_t connections_reused = 0;
        size_t keepalive_checks_sent = 0;
        /// Current utilization: active / (active + idle), averaged across all targets.
        double utilization = 0.0;
        /// Number of pool-size adaptations performed by the adaptive strategy.
        size_t pool_size_adaptations = 0;
        
        /**
         * @brief Calculate connection reuse rate (0.0 - 1.0)
         */
        double getReuseRate() const {
            size_t total = connections_created + connections_reused;
            if (total == 0) return 0.0;
            return static_cast<double>(connections_reused) / static_cast<double>(total);
        }
    };
    
    Stats getStats() const;
    
    /**
     * @brief Warm up pool for target
     * Creates minimum number of connections in advance
     */
    void warmup(const std::string& target);
    
    /**
     * @brief Clear all pooled connections
     */
    void clear();
    
    /**
     * @brief Prune stale connections from pool
     */
    void pruneStaleConnections();
    
private:
    /**
     * @brief Pooled connection with metadata
     */
    struct PooledConnection {
        std::shared_ptr<SocketWrapper> socket;
        std::chrono::steady_clock::time_point last_used;
        std::chrono::steady_clock::time_point created_at;
        bool in_use = false;
        bool authenticated = false;
        
        bool isStale(std::chrono::seconds timeout) const {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::seconds>(now - last_used) > timeout;
        }
        
        bool isHealthy() const {
            return socket && socket->is_open();
        }
    };
    
    /**
     * @brief Per-target connection pool
     */
    struct TargetPool {
        std::mutex mutex;
        std::condition_variable cv;
        std::queue<std::shared_ptr<PooledConnection>> available;
        std::unordered_map<void*, std::shared_ptr<PooledConnection>> all_connections;
        size_t active_count = 0;
    };
    
    /**
     * @brief Parse target string into host and port
     */
    std::pair<std::string, std::string> parseTarget(const std::string& target);
    
    /**
     * @brief Create new connection for target
     */
    std::shared_ptr<SocketWrapper> createConnection(const std::string& target);
    
    /**
     * @brief Release connection back to pool
     */
    void releaseConnection(const std::string& target, std::shared_ptr<SocketWrapper> socket);
    
    /**
     * @brief Get or create target pool
     */
    std::shared_ptr<TargetPool> getOrCreateTargetPool(const std::string& target);
    
    /**
     * @brief Perform connection health check
     */
    bool performHealthCheck(SocketWrapper& socket);
    
    /**
     * @brief Initialize SSL context for TLS/mTLS
     */
    void initializeSSLContext();
    
    /**
     * @brief Adapt pool size based on real-time utilization metrics.
     * Called from the maintenance thread when adaptive sizing is enabled.
     */
    void adaptPoolSize();

    Config config_;
    std::shared_ptr<net::io_context> io_context_;
    std::shared_ptr<ssl::context> ssl_context_;
    
    mutable std::mutex pools_mutex_;
    std::unordered_map<std::string, std::shared_ptr<TargetPool>> target_pools_;
    
    // Background thread for maintenance
    std::thread maintenance_thread_;
    std::atomic<bool> shutdown_{false};
    std::mutex shutdown_mutex_;
    std::condition_variable shutdown_cv_;
    
    // Statistics
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> connections_created_{0};
    std::atomic<size_t> connections_reused_{0};
    std::atomic<size_t> stale_removed_{0};
    std::atomic<size_t> failed_connections_{0};
    std::atomic<size_t> acquire_timeouts_{0};
    std::atomic<size_t> keepalive_checks_{0};
    std::atomic<size_t> pool_size_adaptations_{0};
};

} // namespace network
} // namespace themis
