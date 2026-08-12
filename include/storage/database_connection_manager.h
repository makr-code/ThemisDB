/**
 * @file database_connection_manager.h
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
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <unordered_map>
#include <random>

namespace themis {
namespace storage {

/**
 * @brief Database Connection Resilience Manager
 * 
 * Implements database best-practice safe-fail mechanisms for database connections:
 * - Connection pooling with health checks
 * - Automatic reconnection with exponential backoff
 * - Connection timeout handling
 * - Connection keepalive mechanism
 * - Circuit breaker for failing connections
 * 
 * Best Practices:
 * - Connection pooling: Reuse connections to reduce overhead
 * - Health checks: Verify connections before use
 * - Automatic retry: Transparent reconnection on transient failures
 * - Exponential backoff: Prevent thundering herd on database restart
 * - Circuit breaker: Fail fast when database is down
 * - Timeout handling: Prevent hung operations
 */
class DatabaseConnectionManager {
public:
    enum class ConnectionState {
        HEALTHY,        // Connection is operational
        DEGRADED,       // Connection experiencing issues
        FAILED,         // Connection has failed
        RECONNECTING,   // Attempting to reconnect
        CIRCUIT_OPEN    // Circuit breaker open
    };
    
    struct ConnectionConfig {
        // Connection pool settings
        size_t min_connections = 2;
        size_t max_connections = 10;
        std::chrono::seconds idle_timeout{300};  // 5 minutes
        std::chrono::seconds max_connection_age{3600};  // 1 hour
        
        // Health check settings
        bool enable_health_checks = true;
        std::chrono::seconds health_check_interval{30};
        std::chrono::seconds connection_timeout{10};
        
        // Retry settings
        size_t max_retry_attempts = 5;
        std::chrono::milliseconds initial_retry_delay{100};
        std::chrono::milliseconds max_retry_delay{30000};  // 30 seconds
        double backoff_multiplier = 2.0;
        
        // Circuit breaker settings
        size_t failure_threshold = 5;
        size_t success_threshold = 2;
        std::chrono::seconds circuit_reset_timeout{60};
        
        // Keepalive settings
        bool enable_keepalive = true;
        std::chrono::seconds keepalive_interval{30};
    };
    
    struct ConnectionHealth {
        ConnectionState state;
        size_t total_operations = 0;
        size_t failed_operations = 0;
        size_t reconnect_attempts = 0;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_used_at;
        std::chrono::system_clock::time_point last_health_check;
        std::string last_error;
        float error_rate = 0.0f;
        bool is_stale = false;
    };
    
    struct ConnectionStats {
        size_t total_connections = 0;
        size_t active_connections = 0;
        size_t idle_connections = 0;
        size_t failed_connections = 0;
        size_t total_reconnects = 0;
        size_t circuit_breaker_trips = 0;
        float average_error_rate = 0.0f;
    };
    
    // Connection handle interface
    /** @brief Connection handle interface. */
    class Connection {
    public:
        virtual ~Connection() = default;
        
        [[nodiscard]] virtual bool isValid() const = 0;
        [[nodiscard]] virtual bool ping() = 0;
        [[nodiscard]] virtual std::string getError() const = 0;
        virtual void close() = 0;
        
        // Connection metadata
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_used_at;
        size_t operation_count = 0;
        size_t error_count = 0;
    };
    
    explicit DatabaseConnectionManager(const ConnectionConfig& config);
    ~DatabaseConnectionManager();
    
    /**
     * @brief Acquire a connection from the pool
     * 
     * If all connections are in use, will either:
     * - Wait for one to become available (if blocking)
     * - Create a new connection (if pool not at max)
     * - Return nullptr (if non-blocking and none available)
     * 
     * @param blocking Whether to wait for connection
     * @param timeout Maximum time to wait
     * @return Shared pointer to connection, or nullptr if failed
     */
    std::shared_ptr<Connection> acquireConnection(
        bool blocking = true,
        std::chrono::seconds timeout = std::chrono::seconds(10)
    );
    
    /**
     * @brief Release a connection back to the pool
     * 
     * @param conn Connection to release
     * @param error_occurred Whether an error occurred during use
     */
    void releaseConnection(
        std::shared_ptr<Connection> conn,
        bool error_occurred = false
    );
    
    /**
     * @brief Execute an operation with automatic retry
     * 
     * Wraps a database operation with connection acquisition, retry logic,
     * and automatic reconnection on failure.
     * 
     * @param operation Function to execute with connection
     * @return true if operation succeeded (possibly after retries)
     */
    template<typename ResultType>
    bool executeWithRetry(
        std::function<ResultType(std::shared_ptr<Connection>)> operation,
        ResultType& result
    );
    
    /**
     * @brief Check health of all connections
     * 
     * Pings all connections and removes stale/failed ones
     */
    void performHealthCheck();
    
    /**
     * @brief Get connection statistics
     */
    ConnectionStats getStats() const;
    
    /**
     * @brief Get health info for all connections
     */
    std::vector<ConnectionHealth> getConnectionHealth() const;
    
    /**
     * @brief Check if manager is healthy
     */
    bool isHealthy() const;
    
    /**
     * @brief Force reconnect all connections
     */
    void reconnectAll();
    
    /**
     * @brief Close all connections
     */
    void closeAll();
    
protected:
    // Factory method - override in subclass for specific connection type
    [[nodiscard]] virtual std::shared_ptr<Connection> createConnection() = 0;
    
    // Reconnection helper
    std::shared_ptr<Connection> reconnect(
        std::shared_ptr<Connection> old_conn
    );
    
private:
    ConnectionConfig config_;
    mutable std::mutex mutex_;
    
    // Connection pools
    std::queue<std::shared_ptr<Connection>> idle_connections_;
    std::unordered_map<Connection*, std::shared_ptr<Connection>> active_connections_;
    
    // Health tracking
    std::unordered_map<Connection*, ConnectionHealth> connection_health_;
    
    // Circuit breaker state
    std::atomic<ConnectionState> circuit_state_{ConnectionState::HEALTHY};
    std::atomic<size_t> consecutive_failures_{0};
    std::atomic<size_t> consecutive_successes_{0};
    std::chrono::system_clock::time_point circuit_opened_at_;
    
    // Statistics
    std::atomic<size_t> total_reconnects_{0};
    std::atomic<size_t> circuit_trips_{0};
    
    // Helper methods
    bool isConnectionStale(const Connection* conn) const;
    bool shouldRemoveConnection(const Connection* conn) const;
    void updateCircuitBreaker(bool success);
    bool canAttemptConnection() const;
    std::chrono::milliseconds calculateBackoffDelay(size_t attempt) const;
};

/**
 * @brief Exponential Backoff Calculator
 * 
 * Calculates retry delays with exponential backoff and jitter
 */
class ExponentialBackoff {
public:
    struct Config {
        std::chrono::milliseconds initial_delay{100};
        std::chrono::milliseconds max_delay{30000};
        double multiplier = 2.0;
        double jitter_factor = 0.2;  // 20% random jitter
    };
    
    explicit ExponentialBackoff(const Config& config);
    
    /**
     * @brief Calculate delay for given attempt number
     * 
     * @param attempt Attempt number (0-indexed)
     * @return Delay with jitter applied
     */
    std::chrono::milliseconds calculateDelay(size_t attempt) const;
    
    /**
     * @brief Reset backoff state
     */
    void reset();
    
private:
    Config config_;
    mutable std::mt19937 rng_;
};

/**
 * @brief Connection Keepalive Manager
 * 
 * Periodically sends keepalive messages to maintain connections
 */
class ConnectionKeepalive {
public:
    explicit ConnectionKeepalive(
        std::chrono::seconds interval,
        std::function<bool()> keepalive_fn
    );
    
    ~ConnectionKeepalive();
    
    /**
     * @brief Start keepalive thread
     */
    void start();
    
    /**
     * @brief Stop keepalive thread
     */
    void stop();
    
    /**
     * @brief Check if keepalive is running
     */
    bool isRunning() const;
    
    /**
     * @brief Get number of keepalive attempts
     */
    size_t getKeepaliveCount() const;
    
    /**
     * @brief Get number of failed keepalives
     */
    size_t getFailureCount() const;
    
private:
    std::chrono::seconds interval_;
    std::function<bool()> keepalive_fn_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_stop_{false};
    std::thread keepalive_thread_;
    mutable std::mutex stop_mutex_;
    std::condition_variable stop_cv_;
    std::atomic<size_t> keepalive_count_{0};
    std::atomic<size_t> failure_count_{0};
    
    void keepaliveLoop();
};

/**
 * @brief Connection Timeout Guard
 * 
 * RAII-style timeout enforcement for database operations
 */
class ConnectionTimeoutGuard {
public:
    ConnectionTimeoutGuard(
        std::chrono::seconds timeout,
        const std::string& operation_name
    );
    
    ~ConnectionTimeoutGuard();
    
    /**
     * @brief Check if operation has timed out
     */
    bool hasTimedOut() const;
    
    /**
     * @brief Cancel timeout (call when operation completes)
     */
    void cancel();
    
    /**
     * @brief Get elapsed time
     */
    std::chrono::milliseconds getElapsedTime() const;
    
private:
    std::chrono::seconds timeout_;
    std::string operation_name_;
    std::chrono::system_clock::time_point start_time_;
    std::atomic<bool> cancelled_{false};
    mutable std::atomic<bool> timed_out_{false};
};

} // namespace storage
} // namespace themis
