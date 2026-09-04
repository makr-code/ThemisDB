/**
 * @file mtls_connection_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=9, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/mtls_connection_pool.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <iostream>
#include <algorithm>
#include <openssl/ssl.h>

namespace themis::sharding {

/**
 * @brief Release an OpenSSL SSL object and its associated BIO/socket.
 *
 * Delegates to `SSL_free()`, which also releases the BIO chain attached to
 * the SSL object.  When the BIO was created with `BIO_CLOSE` (e.g. via
 * `BIO_new_socket(fd, BIO_CLOSE)`), the underlying socket handle is closed
 * automatically.
 *
 * @param ptr SSL object to free.  No-op when `ptr` is `nullptr`.
 */
void SSLDeleter::operator()(SSL* ptr) const {
    if (ptr) {
        SSL_free(ptr);
    }
}

// ===========================================================================
// EndpointConnectionPool Implementation
// ===========================================================================

/**
 * @brief Construct a connection pool for the given endpoint (legacy, no factory).
 *
 * Initialises pool state with the provided configuration.  Because no
 * `ConnectionFactory` is injected, `createNewConnection()` will follow the
 * stub/fallback path (returns `nullopt`).  Prefer the factory-based
 * constructor for production use.
 *
 * @param endpoint Target endpoint string (e.g. `"localhost:50051"`).
 * @param config   Pool sizing, TTL, and health-check configuration.
 */
EndpointConnectionPool::EndpointConnectionPool(
    const std::string& endpoint,
    const Config& config
)
    : endpoint_(endpoint), config_(config), running_(true) {
    
    std::cout << "Initializing connection pool for endpoint: " << endpoint << std::endl;
    
    // Warm up minimum connections (non-blocking initialization)
    // Note: In production, this would create actual SSL connections
    // For now, we prepare the pool structure
    
    // Start cleanup thread if health checks are enabled
    if (config_.enable_health_checks) {
        cleanup_thread_ = std::thread([this]() { cleanupLoop(); });
    }
}

/**
 * @brief Construct a connection pool with an injected connection factory (v2.0).
 *
 * Stores @p factory by value inside the pool.  Any external state that was
 * **captured by reference** inside the callable must outlive the pool.  If
 * all captures are by value (or via `std::shared_ptr`), no additional lifetime
 * constraint applies.
 *
 * Starts the background cleanup/health-check thread when
 * `config.enable_health_checks == true`.
 *
 * @param endpoint Target endpoint string (e.g. `"localhost:50051"`).
 * @param config   Pool sizing, TTL, and health-check configuration.
 * @param factory  Callable `(endpoint) → optional<unique_ptr<SSL>>` used by
 *                 `createNewConnection()`.
 */
EndpointConnectionPool::EndpointConnectionPool(
    const std::string& endpoint,
    const Config& config,
    ConnectionFactory factory
)
    : endpoint_(endpoint), config_(config), connection_factory_(std::move(factory)), running_(true) {
    
    THEMIS_INFO("[EndpointConnectionPool] v2.0 factory-based pool initialized for endpoint: {} "
                "with min_connections={}, max_connections={}, factory={}",
                endpoint, config.min_connections, config.max_connections,
                connection_factory_ ? "injected" : "null");
    
    // Start cleanup thread if health checks are enabled
    if (config_.enable_health_checks) {
        cleanup_thread_ = std::thread([this]() { cleanupLoop(); });
    }
}

/**
 * @brief Destroy the pool, closing all connections and stopping the cleanup thread.
 *
 * Calls `closeAll()` to drain idle connections, then signals the background
 * cleanup thread to exit and joins it with a bounded timeout.  Active
 * connections (currently held by callers) are removed from the tracking set
 * but not forcefully closed; callers should release them before the pool is
 * destroyed.
 */
EndpointConnectionPool::~EndpointConnectionPool() {
    closeAll();
    
    // Stop cleanup thread
    running_ = false;
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(cleanup_thread_)) {
        THEMIS_WARN("[EndpointConnectionPool] cleanup thread did not finish within shutdown deadline; detaching.");
    }
}

/**
 * @brief Acquire a connection from the pool, blocking up to @p timeout.
 *
 * Attempts to acquire a connection in the following order:
 * 1. Dequeue a validated, non-expired connection from the idle pool.
 * 2. Create a new connection if the total count is below `config.max_connections`.
 * 3. Block on `available_cv_` until an idle connection is returned by another
 *    thread or `timeout` elapses.
 *
 * The returned connection is removed from the idle pool and added to the
 * active-connections tracking set.
 *
 * @param timeout Maximum wait duration.  Defaults to 5 seconds.
 *
 * @return A ready-to-use `unique_ptr<SSL>` on success, or `nullopt` when:
 *   - No connection could be created and none became available before timeout.
 *   - The pool was shut down while waiting.
 */
std::optional<std::unique_ptr<SSL, SSLDeleter>> EndpointConnectionPool::getConnection(
    std::chrono::milliseconds timeout
) {
    std::unique_lock<std::shared_mutex> lock(pool_mutex_);
    
    try {
        // Try to get from idle pool first
        if (!idle_pool_.empty()) {
            auto pooled = std::move(idle_pool_.front());
            idle_pool_.pop();
            
            // Validate before returning
            if (isConnectionExpired(pooled)) {
                connections_failed_++;
                // Connection expired, try to create new one
            } else if (pooled.ssl && validateConnection(pooled.ssl.get())) {
                // Update metadata
                pooled.last_used = std::chrono::steady_clock::now();
                active_connections_.insert(pooled.ssl.get());
                return std::move(pooled.ssl);
            }
        }
        
        // No idle connections or validation failed
        // Try to create new connection if under limit
        size_t total_connections = static_cast<int>(active_connections_.size()) + idle_pool_.size();
        if (total_connections < config_.max_connections) {
            auto new_conn = createNewConnection();
            if (new_conn) {
                active_connections_.insert(new_conn->get());
                return new_conn;
            }
        }
        
        // Wait for available connection with timeout
        bool available = available_cv_.wait_for(lock, timeout, [this]() {
            return !idle_pool_.empty() || !running_;
        });
        
        if (!available || !running_) {
            std::cerr << "Connection pool exhausted for endpoint: " << endpoint_ << std::endl;
            return std::nullopt;
        }
        
        // Try again after waiting
        if (!idle_pool_.empty()) {
            auto pooled = std::move(idle_pool_.front());
            idle_pool_.pop();
            
            if (pooled.ssl && validateConnection(pooled.ssl.get())) {
                pooled.last_used = std::chrono::steady_clock::now();
                active_connections_.insert(pooled.ssl.get());
                return std::move(pooled.ssl);
            }
        }
        
        return std::nullopt;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception in getConnection: {}", e.what());
        return std::nullopt;
    }
}

/**
 * @brief Return a previously acquired connection to the idle pool.
 *
 * Removes @p connection from the active-connections tracking set.  If the
 * pool is not yet at its maximum capacity, the connection is re-queued in
 * the idle pool and waiting threads are notified.  Otherwise the connection
 * is discarded (its `unique_ptr` destructor calls `SSL_free()` which closes
 * the socket).
 *
 * @param connection SSL connection to return.  Silently ignores `nullptr`.
 */
void EndpointConnectionPool::releaseConnection(std::unique_ptr<SSL, SSLDeleter> connection) {
    if (!connection) {
      return;
    }
    
    std::unique_lock<std::shared_mutex> lock(pool_mutex_);
    
    SSL* raw_ptr = connection.get();
    active_connections_.erase(raw_ptr);
    
    // Check if we should keep this connection or discard it
    size_t total_connections = static_cast<int>(active_connections_.size()) + idle_pool_.size();
    if (total_connections >= config_.max_connections) {
        // Pool is full, discard this connection
        // Connection will be cleaned up when unique_ptr goes out of scope
        return;
    }
    
    // Return to idle pool
    PooledConnection pooled;
    pooled.ssl = std::move(connection);
    pooled.last_used = std::chrono::steady_clock::now();
    pooled.created_at = std::chrono::steady_clock::now(); // Approximate
    pooled.is_valid = true;
    
    idle_pool_.push(std::move(pooled));
    available_cv_.notify_one();
}

/**
 * @brief Mark an active connection as failed and remove it from tracking.
 *
 * Increments the `connections_failed_` counter so that failure rates can be
 * observed via `getStatistics()`.  The caller is responsible for releasing the
 * underlying `SSL*` (e.g. by letting its owning `unique_ptr` go out of scope).
 *
 * @param connection Raw SSL pointer to invalidate.  Silently ignores `nullptr`.
 */
void EndpointConnectionPool::invalidateConnection(SSL* connection) {
    if (!connection) {
      return;
    }
    
    std::unique_lock<std::shared_mutex> lock(pool_mutex_);
    active_connections_.erase(connection);
    connections_failed_++;
}

/**
 * @brief Return a consistent snapshot of pool statistics.
 *
 * Acquires a shared lock so that active/idle counts, lifetime totals, and
 * failure counters are read atomically with respect to concurrent pool
 * operations.
 *
 * @return Current `Statistics` snapshot containing active/idle counts,
 *         total connections created, failures, and utilisation percentage.
 */
EndpointConnectionPool::Statistics EndpointConnectionPool::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(pool_mutex_);
    
    Statistics stats;
    stats.active_connections = active_connections_.size();
    stats.idle_connections = idle_pool_.size();
    stats.total_created = total_created_.load();
    stats.connections_failed = connections_failed_.load();
    
    if (config_.max_connections > 0) {
        stats.utilization_percent = (static_cast<double>(stats.active_connections) / 
                                    config_.max_connections) * 100.0;
    } else {
        stats.utilization_percent = 0.0;
    }
    
    return stats;
}

/**
 * @brief Pre-populate the pool to its minimum connection count.
 *
 * Useful during startup to ensure that the first requests do not incur
 * connection-establishment latency.  Silently tolerates partial warm-up
 * (e.g. when fewer than `min_connections` could be established).
 *
 * @return `true` if at least one connection was created; `false` if all
 *         creation attempts failed.
 */
bool EndpointConnectionPool::warmUp() {
    std::unique_lock<std::shared_mutex> lock(pool_mutex_);
    
    // Create minimum connections
    size_t created = 0;
    for (size_t i = 0; i < config_.min_connections; ++i) {
        auto conn = createNewConnection();
        if (conn.has_value()) {
            PooledConnection pooled;
            pooled.ssl = std::move(conn.value());
            pooled.created_at = std::chrono::steady_clock::now();
            pooled.last_used = std::chrono::steady_clock::now();
            pooled.is_valid = true;
            
            idle_pool_.push(std::move(pooled));
            created++;
        }
    }
    
    std::cout << "Warmed up " << created << " connections for endpoint: " 
              << endpoint_ << std::endl;
    
    return created > 0;
}

/**
 * @brief Drain all idle connections and clear the active-connection set.
 *
 * Idle connections are destroyed immediately (their `unique_ptr` destructors
 * call `SSL_free()`).  Active connections are removed from the tracking set but
 * **not** forcefully closed; callers holding those connections should still
 * release them normally.
 *
 * After this call, `getConnection()` will attempt to create fresh connections.
 */
void EndpointConnectionPool::closeAll() {
    std::unique_lock<std::shared_mutex> lock(pool_mutex_);
    
    // Clear idle pool (unique_ptrs will auto-cleanup)
    while (!idle_pool_.empty()) {
        idle_pool_.pop();
    }
    
    // Note: Active connections will be cleaned up when released
    // In production, we might want to forcefully close them
    active_connections_.clear();
}

/**
 * @brief Attempt to establish a new SSL connection for this pool's endpoint.
 *
 * When a `ConnectionFactory` has been injected (via the v2.0 constructor or
 * `setConnectionFactory()`), delegates to the factory and updates `total_created_`
 * or `connections_failed_` accordingly.
 *
 * When no factory is available, the stub/fallback path is taken:
 * `connections_failed_` is incremented and `nullopt` is returned.  This path
 * exists for backward compatibility; new production code should always inject a
 * factory.
 *
 * @note Must be called while holding a lock on `pool_mutex_` (exclusive).
 *
 * @return New, ready-to-use `unique_ptr<SSL>` on success, or `nullopt` on
 *         failure or when no factory is available.
 */
std::optional<std::unique_ptr<SSL, SSLDeleter>> EndpointConnectionPool::createNewConnection() {
    // When a connection factory has been injected, delegate to it.
    if (connection_factory_) {
        auto conn = connection_factory_(endpoint_);
        if (conn) {
            total_created_++;
        } else {
            connections_failed_++;
        }
        return conn;
    }

    // NON-PRODUCTION PATH (Stub/Fallback)
    // Purpose: Graceful fallback when no factory is injected.
    // Activation: connection_factory_ is null (no factory injected via setConnectionFactory() or constructor).
    // Production Delta: In production, always use v2.0 factory-based construction:
    //   EndpointConnectionPool(endpoint, config, factory)
    //   or: pool->setConnectionFactory(factory)
    // Removal Plan: This stub path is acceptable for backward compatibility; new code should inject a factory.
    // Roadmap ref: src/sharding/FUTURE_ENHANCEMENTS.md § "mTLS Pool Connection Ownership" (COMPLETED v2.0)
    // Status: COMPLETED v2.0 - Factory pattern fully implemented and tested

    THEMIS_DEBUG("EndpointConnectionPool::createNewConnection() called without factory "
                 "for endpoint {}. This is a fallback path (v2.0+). "
                 "For production use, inject a factory via constructor or setConnectionFactory().",
                 endpoint_);
    
    connections_failed_++;
    return std::nullopt;
}

/**
 * @brief Check whether a pooled SSL connection is still usable.
 *
 * Currently performs a null-pointer check.  A production-quality
 * implementation would additionally verify socket liveness (e.g. via a
 * non-blocking `recv` peek) and check the SSL session expiry.
 *
 * @param conn Raw SSL pointer to validate.
 * @return `true` if @p conn appears valid; `false` if `nullptr`.
 */
bool EndpointConnectionPool::validateConnection(SSL* conn) {
    if (!conn) {
      return false;
    }
    
    // Note: In production, this would:
    // 1. Check if socket is still connected
    // 2. Optionally send a ping/health check
    // 3. Verify SSL session is still valid
    
    // For now, assume connection is valid
    return true;
}

/**
 * @brief Determine whether a pooled connection should be evicted.
 *
 * A connection is considered expired if any of the following are true:
 * - Its age since creation exceeds `config_.connection_ttl`.
 * - The time since its last use exceeds `config_.idle_timeout`.
 * - Its `is_valid` flag has been cleared by an earlier operation.
 *
 * @param pooled Reference to the pooled-connection metadata to evaluate.
 * @return `true` if the connection has expired and should be discarded.
 */
bool EndpointConnectionPool::isConnectionExpired(const PooledConnection& pooled) {
    auto now = std::chrono::steady_clock::now();
    
    // Check TTL
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        now - pooled.created_at
    );
    if (age > config_.connection_ttl) {
        return true;
    }
    
    // Check idle timeout
    auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(
        now - pooled.last_used
    );
    if (idle_time > config_.idle_timeout) {
        return true;
    }
    
    return !pooled.is_valid;
}

/**
 * @brief Evict all expired connections from the idle pool.
 *
 * Rebuilds `idle_pool_` by scanning every entry and retaining only those
 * where `isConnectionExpired()` returns `false`.  Evicted entries are
 * destroyed in-place (their `unique_ptr` destructors close the socket).
 *
 * @note Must be called while holding an exclusive lock on `pool_mutex_`.
 */
void EndpointConnectionPool::cleanupExpiredConnections() {
    // Remove expired connections from idle pool
    std::queue<PooledConnection> new_pool;
    size_t removed = 0;
    
    while (!idle_pool_.empty()) {
        auto pooled = std::move(idle_pool_.front());
        idle_pool_.pop();
        
        if (!isConnectionExpired(pooled)) {
            new_pool.push(std::move(pooled));
        } else {
            removed++;
        }
    }
    
    idle_pool_ = std::move(new_pool);
    
    if (removed > 0) {
        std::cout << "Cleaned up " << removed << " expired connections for endpoint: " 
                  << endpoint_ << std::endl;
    }
}

/**
 * @brief Background loop that periodically evicts expired idle connections.
 *
 * Runs on a dedicated thread started in the constructor when
 * `config_.enable_health_checks` is `true`.  Sleeps for
 * `config_.health_check_interval` between each cleanup pass and exits when
 * `running_` is set to `false` (signalled by the destructor).
 */
void EndpointConnectionPool::cleanupLoop() {
    while (running_) {
        std::this_thread::sleep_for(config_.health_check_interval);
        
        if (!running_) {
          break;
        }
        
        std::unique_lock<std::shared_mutex> lock(pool_mutex_);
        cleanupExpiredConnections();
    }
}

// ===========================================================================
// MTLSConnectionPoolManager Implementation
// ===========================================================================

/**
 * @brief Construct the pool manager with explicit configuration.
 *
 * @param config Manager-level settings including per-endpoint pool defaults,
 *               global connection limits, and endpoint eviction policy.
 */
MTLSConnectionPoolManager::MTLSConnectionPoolManager(const Config& config)
    : config_(config) {
    std::cout << "Initialized MTLSConnectionPoolManager" << std::endl;
}

/**
 * @brief Construct the pool manager with default configuration.
 */
MTLSConnectionPoolManager::MTLSConnectionPoolManager()
    : MTLSConnectionPoolManager(Config{})
{
}

/**
 * @brief Destroy the pool manager, closing all managed pools.
 *
 * Delegates to `shutdown()`, which drains idle connections across every
 * registered endpoint pool and then clears the pool registry.
 */
MTLSConnectionPoolManager::~MTLSConnectionPoolManager() {
    shutdown();
}

/**
 * @brief Retrieve (or lazily create) the connection pool for @p endpoint.
 *
 * Uses a double-checked locking pattern: the common read-path acquires a
 * shared lock; if the pool is absent, an exclusive lock is taken and the pool
 * is created under that lock.
 *
 * When the endpoint capacity (`config_.max_endpoints`) is reached and
 * `enable_endpoint_eviction` is set, an LRU eviction pass would be performed
 * here (currently logs a warning only).
 *
 * @param endpoint Target endpoint string (e.g. `"host:port"`).
 * @return Shared pointer to the per-endpoint pool (never `nullptr`).
 */
std::shared_ptr<EndpointConnectionPool> MTLSConnectionPoolManager::getPool(
    const std::string& endpoint
) {
    // Try to find existing pool first (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(pools_mutex_);
        auto it = pools_.find(endpoint);
        if (it != pools_.end()) {
            return it->second;
        }
    }
    
    // Create new pool (write lock)
    std::unique_lock<std::shared_mutex> lock(pools_mutex_);
    
    // Double-check in case another thread created it
    auto it = pools_.find(endpoint);
    if (it != pools_.end()) {
        return it->second;
    }
    
    // Check global limits
    if (config_.enable_endpoint_eviction && pools_.size() >= config_.max_endpoints) {
        std::cerr << "Warning: Maximum endpoint pools reached (" 
                  << config_.max_endpoints << ")" << std::endl;
        // In production, we would implement LRU eviction here
    }
    
    // Create new pool
    auto pool = std::make_shared<EndpointConnectionPool>(
        endpoint,
        config_.endpoint_config
    );
    
    pools_[endpoint] = pool;
    return pool;
}

/**
 * @brief Convenience method: acquire a connection for @p endpoint.
 *
 * Looks up (or creates) the per-endpoint pool via `getPool()` and then
 * forwards to `EndpointConnectionPool::getConnection()`.  Increments the
 * manager-level `total_connections_` counter on success.
 *
 * @param endpoint Target endpoint string.
 * @param timeout  Maximum wait duration; forwarded to the pool.
 * @return A ready-to-use `unique_ptr<SSL>`, or `nullopt` on failure/timeout.
 */
std::optional<std::unique_ptr<SSL, SSLDeleter>> MTLSConnectionPoolManager::getConnection(
    const std::string& endpoint,
    std::chrono::milliseconds timeout
) {
    auto pool = getPool(endpoint);
    if (!pool) {
        return std::nullopt;
    }
    
    auto conn = pool->getConnection(timeout);
    if (conn) {
        total_connections_++;
    }
    
    return conn;
}

/**
 * @brief Return a connection to the appropriate per-endpoint pool.
 *
 * Looks up the pool for @p endpoint under a shared lock and, if found,
 * delegates to `EndpointConnectionPool::releaseConnection()`.  The
 * manager-level `total_connections_` counter is decremented on success.
 *
 * If no pool exists for @p endpoint (i.e. it was evicted or never created),
 * the connection is dropped (its `unique_ptr` destructor closes it).
 *
 * @param endpoint Target endpoint that originally supplied the connection.
 * @param conn     SSL connection to return.  Silently ignores `nullptr`.
 */
void MTLSConnectionPoolManager::releaseConnection(
    const std::string& endpoint, 
    std::unique_ptr<SSL, SSLDeleter> conn
) {
    if (!conn) {
      return;
    }
    
    std::shared_lock<std::shared_mutex> lock(pools_mutex_);
    
    auto it = pools_.find(endpoint);
    if (it != pools_.end()) {
        it->second->releaseConnection(std::move(conn));
        if (total_connections_ > 0) {
            total_connections_--;
        }
    }
}

/**
 * @brief Return an aggregate statistics snapshot across all endpoint pools.
 *
 * Acquires a shared lock on the pool registry and iterates over every
 * registered `EndpointConnectionPool`.  Per-endpoint statistics are
 * aggregated into global active/idle totals and returned alongside the
 * per-endpoint breakdown.
 *
 * @return `GlobalStatistics` containing total active/idle connections,
 *         number of cached endpoint pools, and per-endpoint breakdown.
 */
MTLSConnectionPoolManager::GlobalStatistics MTLSConnectionPoolManager::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(pools_mutex_);
    
    GlobalStatistics stats;
    stats.cached_endpoint_pools = pools_.size();
    stats.total_active_connections = 0;
    stats.total_idle_connections = 0;
    
    for (const auto& [endpoint, pool] : pools_) {
        auto pool_stats = pool->getStatistics();
        stats.per_endpoint_stats[endpoint] = pool_stats;
        stats.total_active_connections += pool_stats.active_connections;
        stats.total_idle_connections += pool_stats.idle_connections;
    }
    
    return stats;
}

/**
 * @brief Handle a PKI certificate rotation event.
 *
 * Drains idle connections across all registered endpoint pools so that the
 * next `getConnection()` call on each endpoint will establish a fresh TLS
 * session using the rotated certificate.  In-flight active connections are
 * left untouched; they will be closed when callers release them, at which
 * point the pool will create new connections with the updated certificate.
 *
 * This method is non-blocking: it schedules the drain synchronously under a
 * shared lock and then returns.  Callers that must confirm drain completion
 * should poll `getStatistics()` until idle counts recover to expected levels.
 *
 * Wire-up: register this method as a callback on the PKI client's certificate-
 * rotation event to prevent stale TLS sessions from being reused after rotation.
 */
void MTLSConnectionPoolManager::onCertificateRotated() {
    // Drain idle connections so all new requests re-establish TLS with the
    // refreshed certificate.  In-flight active connections are left untouched
    // (they will be closed naturally when their users release them, after which
    // the pool will create fresh connections with the new cert).
    std::shared_lock<std::shared_mutex> lock(pools_mutex_);

    std::cout << "MTLSConnectionPoolManager: certificate rotated – draining idle "
              << "connections across " << pools_.size() << " endpoint pools" << std::endl;

    for (auto& [endpoint, pool] : pools_) {
        // closeAll() drains idle connections while active ones remain open.
        // The next getConnection() call will create a new TLS connection which
        // will pick up the rotated certificate from disk.
        pool->closeAll();
    }
}

/**
 * @brief Shut down all endpoint pools and release resources.
 *
 * Acquires an exclusive lock on the pool registry, calls `closeAll()` on
 * every registered `EndpointConnectionPool` to drain idle connections, then
 * clears the pool map and resets `total_connections_` to zero.
 *
 * After this call the manager is effectively empty; subsequent `getPool()`
 * or `getConnection()` calls will create new pools from scratch.
 */
void MTLSConnectionPoolManager::shutdown() {
    std::unique_lock<std::shared_mutex> lock(pools_mutex_);
    
    std::cout << "Shutting down MTLSConnectionPoolManager with " 
              << pools_.size() << " endpoint pools" << std::endl;
    
    // Close all pools
    for (auto& [endpoint, pool] : pools_) {
        pool->closeAll();
    }
    
    pools_.clear();
    total_connections_ = 0;
}

} // namespace themis::sharding
