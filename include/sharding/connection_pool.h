// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_CONNECTION_POOL_H
#define THEMISDB_SHARDING_CONNECTION_POOL_H

#include <queue>
#include <set>
#include <map>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <atomic>
#include <functional>

namespace themisdb {
namespace sharding {

/**
 * @brief Generic connection handle (can be SSL*, socket fd, etc.)
 */
struct Connection {
    void* handle{nullptr};
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_used;
    
    bool operator<(const Connection& other) const {
        return handle < other.handle;
    }
    
    bool operator==(const Connection& other) const {
        return handle == other.handle;
    }
};

/**
 * @brief Connection Pool with Dynamic Scaling
 * 
 * Thread-safe connection pool that:
 * - Dynamically scales from initial_size to max_size
 * - Closes idle connections after timeout
 * - Tracks connection lifecycle metrics
 * - Supports custom connection creation/destruction
 */
class ConnectionPool {
public:
    struct Config {
        size_t initial_size{10};
        size_t max_size{100};
        std::chrono::milliseconds idle_timeout{300000};  // 5 minutes
        std::chrono::milliseconds connection_timeout{5000};
        std::function<Connection()> create_connection;
        std::function<void(Connection)> destroy_connection;
    };
    
    struct Stats {
        size_t total_size;
        size_t available;
        size_t in_use;
        size_t created_total;
        size_t closed_total;
        size_t timeout_errors;
    };
    
    explicit ConnectionPool(const Config& config);
    ~ConnectionPool();
    
    /**
     * @brief Acquire connection from pool
     * @param timeout Maximum time to wait for available connection
     * @return Connection if available, nullopt on timeout
     */
    std::optional<Connection> acquire(std::chrono::milliseconds timeout);
    
    /**
     * @brief Return connection to pool
     * @param conn Connection to return
     */
    void release(Connection conn);
    
    /**
     * @brief Close idle connections
     * @return Number of connections closed
     */
    size_t closeIdleConnections();
    
    /**
     * @brief Get pool statistics
     * @return Current stats
     */
    Stats getStats() const;
    
    /**
     * @brief Shutdown pool and close all connections
     */
    void shutdown();

private:
    Config config_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Connection> available_;
    std::set<Connection> in_use_;
    std::map<Connection, std::chrono::system_clock::time_point> idle_times_;
    
    std::atomic<size_t> created_total_{0};
    std::atomic<size_t> closed_total_{0};
    std::atomic<size_t> timeout_errors_{0};
    std::atomic<bool> shutdown_{false};
    
    /**
     * @brief Create new connection
     */
    Connection createConnection();
    
    /**
     * @brief Destroy connection
     */
    void destroyConnection(Connection conn);
    
    /**
     * @brief Check if we can create more connections
     */
    bool canCreateMore() const;
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_CONNECTION_POOL_H
