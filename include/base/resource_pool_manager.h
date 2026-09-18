/**
 * @file resource_pool_manager.h
 * @brief Phase 3 P3-03-A/B: Unified resource pool orchestrator for ThemisDB.
 *
 * Provides:
 *  - @ref AdaptiveConnectionPool — pool of generic connection slots with
 *    dynamic min/max sizing and acquisition-timeout.
 *  - @ref ResourcePoolManager — unified facade over the connection pool,
 *    buffer pool (@ref BufferPool), and thread pool
 *    (@ref themis::utils::ThreadPoolManager).
 *
 * Design goals (P3-03 acceptance criteria):
 *  - Connection pool: min=5, max=50, scale-up latency < 10 ms.
 *  - Leak detection: borrowed resources timed out and reclaimed.
 *  - Saturation monitoring: alert threshold at > 80 % utilization.
 *  - Unified statistics across all pool types.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Block B P3-03-A/B delivery
 */

#pragma once

#include "base/buffer_pool.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace themis::resource {

// ============================================================================
// AdaptiveConnectionPool
// ============================================================================

/**
 * @brief Thread-safe, adaptive pool of abstract connection slots.
 *
 * A "connection slot" is an opaque token.  Real implementations would wrap
 * a TCP/TLS socket; this abstraction allows unit testing without a network.
 *
 * ### Adaptive sizing rules
 *  - If the average acquisition wait exceeds @p scale_up_threshold_ms for
 *    two consecutive sampling intervals, grow the pool by @p scale_step
 *    (up to @p max_size).
 *  - If utilisation falls below 20 % for @p idle_shrink_periods consecutive
 *    intervals, shrink by @p scale_step (down to @p min_size).
 *
 * ### Acquisition semantics
 *  - @ref acquire() blocks up to @p timeout; returns @c false on timeout.
 *  - @ref release() returns the slot to the pool.
 */
class AdaptiveConnectionPool {
public:
    /**
     * @brief Pool configuration.
     */
    struct Config {
        std::size_t min_size    =  5;   ///< Minimum pool size.
        std::size_t max_size    = 50;   ///< Maximum pool size.
        std::size_t scale_step  =  5;   ///< Connections added/removed per scaling event.
        /// Acquisition wait above this triggers a scale-up (milliseconds).
        double      scale_up_threshold_ms  = 1.0;
        /// Consecutive idle periods before shrinking.
        std::size_t idle_shrink_periods    = 3;
    };

    /**
     * @brief Statistics snapshot.
     */
    struct Statistics {
        std::size_t pool_size     = 0; ///< Current pool capacity.
        std::size_t available     = 0; ///< Connections currently available.
        std::size_t in_use        = 0; ///< Connections currently acquired.
        std::size_t total_acquires = 0;
        std::size_t total_timeouts = 0;
        std::size_t scale_up_events   = 0;
        std::size_t scale_down_events = 0;
        double      peak_utilization  = 0.0; ///< Peak in-use / pool_size ratio seen.
    };

    /**
     * @brief Constructs the pool with default configuration.
     */
    AdaptiveConnectionPool();

    /**
     * @brief Constructs the pool and allocates @p cfg.min_size initial slots.
     * @param cfg  Pool configuration.
     */
    explicit AdaptiveConnectionPool(const Config& cfg);

    ~AdaptiveConnectionPool();

    // Non-copyable, non-movable.
    AdaptiveConnectionPool(const AdaptiveConnectionPool&)            = delete;
    AdaptiveConnectionPool& operator=(const AdaptiveConnectionPool&) = delete;

    /**
     * @brief Acquires a connection slot.
     *
     * Blocks until a slot is available or @p timeout expires.
     *
     * @param timeout  Maximum time to wait.
     * @param slot_id  Output: identifier of the acquired slot (>=0 on success).
     * @return @c true if a slot was acquired, @c false on timeout.
     *
     * @throws std::runtime_error if the pool has been shut down.
     */
    bool acquire(std::chrono::milliseconds timeout, int& slot_id);

    /**
     * @brief Returns a slot to the pool.
     *
     * @param slot_id  The identifier returned by @ref acquire().
     * @throws std::invalid_argument if @p slot_id is not a valid acquired slot.
     */
    void release(int slot_id);

    /// @brief Returns the current pool capacity.
    [[nodiscard]] std::size_t size()      const noexcept;
    /// @brief Returns the number of available (unacquired) slots.
    [[nodiscard]] std::size_t available() const noexcept;
    /// @brief Returns the number of slots currently in use.
    [[nodiscard]] std::size_t in_use()    const noexcept;

    /// @brief Returns a statistics snapshot.
    [[nodiscard]] Statistics statistics() const noexcept;

    /// @brief Returns true once shutdown() has been called.
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    /**
     * @brief Shuts down the pool, unblocking all waiters.
     *
     * After shutdown, @ref acquire() throws @c std::runtime_error.
     */
    void shutdown() noexcept;

    /// @brief Manually trigger a scale-up (for testing).
    void forceScaleUp();

    /// @brief Manually trigger a scale-down (for testing).
    void forceScaleDown();

private:
    void growLocked(std::size_t count);  ///< Grow pool (caller holds lock).
    void shrinkLocked(std::size_t count); ///< Shrink pool (caller holds lock).

    Config                    cfg_;
    mutable std::mutex        mutex_;
    std::condition_variable   cv_;

    std::vector<int>          available_slots_;  ///< Free-list of slot IDs.
    std::size_t               pool_size_ = 0;    ///< Total allocated slots.
    int                       next_id_   = 0;    ///< Monotonically increasing slot ID.

    std::atomic<bool>         shutdown_{false};

    // Stats (protected by mutex_).
    std::size_t total_acquires_     = 0;
    std::size_t total_timeouts_     = 0;
    std::size_t scale_up_events_    = 0;
    std::size_t scale_down_events_  = 0;
    double      peak_utilization_   = 0.0;

    // Timing for adaptive scaling.
    std::chrono::steady_clock::time_point last_wait_start_;
    double      cumulative_wait_ms_ = 0.0;
    std::size_t wait_samples_       = 0;
    std::size_t idle_periods_       = 0;
};

// ============================================================================
// ResourcePoolManager
// ============================================================================

/**
 * @brief Unified orchestrator over ThemisDB resource pools.
 *
 * Aggregates the @ref AdaptiveConnectionPool, @ref BufferPool, and (via the
 * existing @c themis::utils::ThreadPoolManager) the thread pool.  Provides:
 *  - Coordinated initialization and teardown.
 *  - Aggregated statistics across all pools.
 *  - Saturation monitoring with configurable alert threshold (default 80 %).
 *  - Resource-leak detection: borrowed connections tracked and force-reclaimed
 *    after a configurable timeout.
 */
class ResourcePoolManager {
public:
    /**
     * @brief Configuration for the resource pool manager.
     */
    struct Config {
        AdaptiveConnectionPool::Config  conn_pool;    ///< Connection pool config.
        BufferPool::Config              buffer_pool;  ///< Buffer pool config.
        /// Saturation alert threshold (0–1, default 0.80).
        double saturation_alert_threshold = 0.80;
        /// Leak-detection: reclaim borrowed connections after this timeout.
        std::chrono::seconds leak_timeout{30};
    };

    /**
     * @brief Aggregated statistics across all pools.
     */
    struct GlobalStatistics {
        AdaptiveConnectionPool::Statistics conn;
        BufferPool::Statistics             buffer;
        double saturation_conn   = 0.0; ///< conn.in_use / conn.pool_size
        double saturation_buffer = 0.0; ///< buffer.current_live / theoretical max
        bool   saturation_alert  = false; ///< True if any pool > threshold.
    };

    /**
     * @brief Constructs and initialises all managed pools with default config.
     */
    ResourcePoolManager();

    /**
     * @brief Constructs and initialises all managed pools.
     * @param cfg  Configuration.
     */
    explicit ResourcePoolManager(const Config& cfg);

    ~ResourcePoolManager();

    // Non-copyable.
    ResourcePoolManager(const ResourcePoolManager&)            = delete;
    ResourcePoolManager& operator=(const ResourcePoolManager&) = delete;

    /// @brief Access the connection pool.
    [[nodiscard]] AdaptiveConnectionPool& connectionPool() noexcept {
        return *conn_pool_;
    }

    /// @brief Access the buffer pool.
    [[nodiscard]] BufferPool& bufferPool() noexcept {
        return *buf_pool_;
    }

    /// @brief Returns aggregated statistics.
    [[nodiscard]] GlobalStatistics statistics() const noexcept;

    /**
     * @brief Shuts down all managed pools in dependency order.
     *
     * Safe to call multiple times.
     */
    void shutdown() noexcept;

    /// @brief Returns true once @ref shutdown() has been called.
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

private:
    Config                                     cfg_;
    std::unique_ptr<AdaptiveConnectionPool>    conn_pool_;
    std::unique_ptr<BufferPool>                buf_pool_;
    std::atomic<bool>                          shutdown_{false};
};

}  // namespace themis::resource
