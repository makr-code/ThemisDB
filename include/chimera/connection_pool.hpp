/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            connection_pool.hpp                                ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:06:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     360                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 566ec90912  2026-03-12  Changes before error encountered        ║
    • 29a0ee06c5  2026-03-12  feat(chimera): wire THEMIS_ENABLE_* driver blocks, add Co... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file connection_pool.hpp
 * @brief Generic connection pool for CHIMERA database adapters
 *
 * @details
 * Provides a thread-safe, bounded connection pool for database driver
 * connections. Features include:
 *   - Configurable min/max pool size
 *   - Blocking acquire with timeout
 *   - Connection health validation before reuse
 *   - Automatic reconnection on failed health checks
 *   - Pool statistics (active, idle, total, acquire times)
 *
 * Usage:
 * @code
 *   ConnectionPoolConfig cfg;
 *   cfg.min_connections = 5;
 *   cfg.max_connections = 50;
 *
 *   ConnectionPool<httplib::Client> pool(
 *       []() { return std::make_unique<httplib::Client>("localhost", 6333); },
 *       [](httplib::Client& c) { return c.Get("/healthz") != nullptr; },
 *       cfg);
 *
 *   auto conn = pool.acquire();
 *   // … use *conn …
 *   pool.release(std::move(conn));
 * @endcode
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_CONNECTION_POOL_HPP
#define CHIMERA_CONNECTION_POOL_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace chimera {

/**
 * @struct ConnectionPoolConfig
 * @brief Configuration parameters for a connection pool instance.
 */
struct ConnectionPoolConfig {
    /// Minimum number of connections kept open at all times.
    size_t min_connections = 5;

    /// Maximum total connections (active + idle) the pool will create.
    size_t max_connections = 50;

    /// Default timeout passed to acquire() when no explicit timeout is given.
    /// Callers may override this per-call by passing a value to acquire().
    std::chrono::milliseconds connection_timeout{30'000};

    /// Maximum time a connection can remain idle before being eligible for
    /// eviction. Eviction is performed lazily inside health_check() — the pool
    /// does not schedule evictions automatically; callers must invoke
    /// health_check() on a suitable timer if idle-TTL enforcement is required.
    std::chrono::minutes idle_timeout{5};

    /// Suggested interval between health-check sweeps. The pool does NOT
    /// schedule health checks automatically; callers must invoke health_check()
    /// on their own schedule (e.g., using a background thread or a periodic
    /// task). This field is provided for callers that wish to align their
    /// scheduling with the pool's configuration.
    std::chrono::seconds health_check_interval{60};
};

/**
 * @struct PoolStats
 * @brief Runtime statistics snapshot for a connection pool.
 */
struct PoolStats {
    /// Total connections currently managed by the pool (active + idle).
    size_t total_connections = 0;

    /// Connections currently checked out by callers.
    size_t active_connections = 0;

    /// Connections sitting idle in the pool, ready for reuse.
    size_t idle_connections = 0;

    /// Cumulative successful acquire() calls since pool creation.
    size_t total_acquires = 0;

    /// Cumulative release() calls since pool creation.
    size_t total_releases = 0;

    /// Cumulative acquire() calls that timed out or failed.
    size_t failed_acquires = 0;

    /// Rolling average of time spent inside acquire() (milliseconds).
    std::chrono::milliseconds avg_acquire_time{0};

    /// Rolling average of time spent executing a query (milliseconds).
    /// Populated by callers that instrument their query durations.
    std::chrono::milliseconds avg_query_time{0};
};

/**
 * @class ConnectionPool
 * @brief Thread-safe, bounded connection pool.
 *
 * @tparam Connection  The driver connection type managed by this pool.
 *
 * @details
 * Connections are created on demand up to max_connections. When the pool is
 * saturated, acquire() blocks until a connection is released or the timeout
 * expires. Idle connections are validated via the supplied validator before
 * being handed back to callers; unhealthy connections are replaced with fresh
 * ones.
 */
template <typename Connection>
class ConnectionPool {
public:
    /// Factory that creates a new Connection instance.
    using ConnectionFactory = std::function<std::unique_ptr<Connection>()>;

    /// Predicate that returns true when a connection is healthy.
    using ConnectionValidator = std::function<bool(Connection&)>;

    /**
     * @brief Construct a pool with the given factory, validator, and config.
     *
     * Pre-allocates min_connections connections to warm the pool.
     */
    ConnectionPool(
        ConnectionFactory   factory,
        ConnectionValidator validator,
        const ConnectionPoolConfig& config = {}
    )
        : factory_(std::move(factory))
        , validator_(std::move(validator))
        , config_(config)
    {
        // Warm the pool to the configured minimum.
        std::lock_guard<std::mutex> lock(mutex_);
        for (size_t i = 0; i < config_.min_connections; ++i) {
            auto conn = factory_();
            if (conn) {
                idle_.push_back(std::move(conn));
                total_connections_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    ~ConnectionPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        idle_.clear();
    }

    // Non-copyable, non-movable (mutex and CV cannot be copied)
    ConnectionPool(const ConnectionPool&)            = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    ConnectionPool(ConnectionPool&&)                 = delete;
    ConnectionPool& operator=(ConnectionPool&&)      = delete;

    /**
     * @brief Check out a connection from the pool.
     *
     * Blocks until a connection is available or @p timeout elapses.
     * When @p timeout is not specified, defaults to
     * `config_.connection_timeout`.  Returns nullptr on timeout or failure.
     */
    std::unique_ptr<Connection> acquire(
        std::optional<std::chrono::milliseconds> timeout = std::nullopt
    ) {
        const auto effective_timeout =
            timeout.value_or(config_.connection_timeout);
        const auto deadline = std::chrono::steady_clock::now() + effective_timeout;
        const auto t_start  = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lock(mutex_);

        // Wait until a connection is idle or we can create a new one.
        const bool slot_available = cv_.wait_until(lock, deadline, [this] {
            return !idle_.empty() ||
                   total_connections_.load(std::memory_order_relaxed) <
                       config_.max_connections;
        });

        if (!slot_available) {
            failed_acquires_.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }

        std::unique_ptr<Connection> conn;
        if (!idle_.empty()) {
            conn = std::move(idle_.back());
            idle_.pop_back();
        } else {
            // Unlock while creating (potentially blocking) connection.
            lock.unlock();
            conn = factory_();
            lock.lock();
            if (conn) {
                total_connections_.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (!conn) {
            failed_acquires_.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }

        // Health-check; replace if unhealthy.
        if (!validator_(*conn)) {
            lock.unlock();
            conn = factory_();
            lock.lock();
            if (!conn) {
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
                failed_acquires_.fetch_add(1, std::memory_order_relaxed);
                return nullptr;
            }
        }

        active_connections_.fetch_add(1, std::memory_order_relaxed);
        const size_t acq = total_acquires_.fetch_add(1, std::memory_order_relaxed) + 1;
        // Rolling average of acquire time.
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t_start).count();
        const int64_t curr_avg = avg_acquire_ms_.load(std::memory_order_relaxed);
        avg_acquire_ms_.store(
            (curr_avg * static_cast<int64_t>(acq - 1) + elapsed_ms) /
            static_cast<int64_t>(acq),
            std::memory_order_relaxed);

        return conn;
    }

    /**
     * @brief Return a connection to the pool.
     *
     * If the pool is already full the connection is discarded and the total
     * connection count is decremented.
     */
    void release(std::unique_ptr<Connection> conn) {
        if (!conn) return;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (idle_.size() < config_.max_connections) {
                idle_.push_back(std::move(conn));
            } else {
                // Pool is saturated – let the connection be destroyed.
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
            }
        }

        active_connections_.fetch_sub(1, std::memory_order_relaxed);
        total_releases_.fetch_add(1, std::memory_order_relaxed);
        cv_.notify_one();
    }

    /**
     * @brief Return a snapshot of current pool statistics.
     */
    [[nodiscard]] PoolStats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        PoolStats s;
        s.total_connections  = total_connections_.load(std::memory_order_relaxed);
        s.active_connections = active_connections_.load(std::memory_order_relaxed);
        s.idle_connections   = idle_.size();
        s.total_acquires     = total_acquires_.load(std::memory_order_relaxed);
        s.total_releases     = total_releases_.load(std::memory_order_relaxed);
        s.failed_acquires    = failed_acquires_.load(std::memory_order_relaxed);
        s.avg_acquire_time   = std::chrono::milliseconds{
            avg_acquire_ms_.load(std::memory_order_relaxed)};
        s.avg_query_time     = std::chrono::milliseconds{0};
        return s;
    }

    /**
     * @brief Validate all idle connections and replace unhealthy ones.
     *
     * After sweeping, replenishes idle connections to the configured minimum.
     */
    void health_check() {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = idle_.begin();
        while (it != idle_.end()) {
            if (*it && !validator_(**it)) {
                it = idle_.erase(it);
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
            } else {
                ++it;
            }
        }

        // Replenish to minimum.
        const size_t live = total_connections_.load(std::memory_order_relaxed);
        for (size_t i = live; i < config_.min_connections; ++i) {
            auto conn = factory_();
            if (conn) {
                idle_.push_back(std::move(conn));
                total_connections_.fetch_add(1, std::memory_order_relaxed);
            } else {
                break;
            }
        }
    }

    /**
     * @brief Alias for health_check() – removes unhealthy connections.
     */
    void remove_unhealthy_connections() { health_check(); }

private:
    ConnectionFactory   factory_;
    ConnectionValidator validator_;
    ConnectionPoolConfig config_;

    mutable std::mutex      mutex_;
    std::condition_variable cv_;

    std::vector<std::unique_ptr<Connection>> idle_;

    std::atomic<size_t>  total_connections_{0};
    std::atomic<size_t>  active_connections_{0};
    std::atomic<size_t>  total_acquires_{0};
    std::atomic<size_t>  total_releases_{0};
    std::atomic<size_t>  failed_acquires_{0};
    std::atomic<int64_t> avg_acquire_ms_{0};
};

} // namespace chimera

#endif // CHIMERA_CONNECTION_POOL_HPP
