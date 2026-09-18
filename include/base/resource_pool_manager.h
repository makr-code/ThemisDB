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

class AdaptiveConnectionPool {
public:
    struct Config {
        std::size_t min_size    =  5;   ///< Minimum pool size.
        std::size_t max_size    = 50;   ///< Maximum pool size.
        std::size_t scale_step  =  5;   ///< Connections added/removed per scaling event.
        double      scale_up_threshold_ms  = 1.0;
        std::size_t idle_shrink_periods    = 3;
    };

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

    AdaptiveConnectionPool();

    /**
     * @brief Adaptive Connection Pool.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit AdaptiveConnectionPool(const Config& cfg);

    ~AdaptiveConnectionPool();

    // Non-copyable, non-movable.
    AdaptiveConnectionPool(const AdaptiveConnectionPool&)            = delete;
    AdaptiveConnectionPool& operator=(const AdaptiveConnectionPool&) = delete;

    /**
     * @brief Acquire.
     * @param[in] timeout Input parameter.
     * @param[in,out] slot_id Identifier of the slot.
     * @return True when the operation succeeds.
     */
    bool acquire(std::chrono::milliseconds timeout, int& slot_id);

    /**
     * @brief Release.
     * @param[in] slot_id Identifier of the slot.
     */
    void release(int slot_id);

    [[nodiscard]] std::size_t size()      const noexcept;
    [[nodiscard]] std::size_t available() const noexcept;
    [[nodiscard]] std::size_t in_use()    const noexcept;

    [[nodiscard]] Statistics statistics() const noexcept;

    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    /**
     * @brief Shutdown.
     * @note Exception safety: noexcept.
     */
    void shutdown() noexcept;

    /**
     * @brief Force Scale Up.
     */
    void forceScaleUp();

    /**
     * @brief Force Scale Down.
     */
    void forceScaleDown();

private:
    /**
     * @brief Grow Locked.
     * @param[in] count Input parameter.
     */
    void growLocked(std::size_t count);  ///< Grow pool (caller holds lock).
    /**
     * @brief Shrink Locked.
     * @param[in] count Input parameter.
     */
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

class ResourcePoolManager {
public:
    struct Config {
        AdaptiveConnectionPool::Config  conn_pool;    ///< Connection pool config.
        BufferPool::Config              buffer_pool;  ///< Buffer pool config.
        double saturation_alert_threshold = 0.80;
        std::chrono::seconds leak_timeout{30};
    };

    struct GlobalStatistics {
        AdaptiveConnectionPool::Statistics conn;
        BufferPool::Statistics             buffer;
        double saturation_conn   = 0.0; ///< conn.in_use / conn.pool_size
        double saturation_buffer = 0.0; ///< buffer.current_live / theoretical max
        bool   saturation_alert  = false; ///< True if any pool > threshold.
    };

    ResourcePoolManager();

    /**
     * @brief Resource Pool Manager.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit ResourcePoolManager(const Config& cfg);

    ~ResourcePoolManager();

    // Non-copyable.
    ResourcePoolManager(const ResourcePoolManager&)            = delete;
    ResourcePoolManager& operator=(const ResourcePoolManager&) = delete;

    [[nodiscard]] AdaptiveConnectionPool& connectionPool() noexcept {
        return *conn_pool_;
    }

    [[nodiscard]] BufferPool& bufferPool() noexcept {
        return *buf_pool_;
    }

    [[nodiscard]] GlobalStatistics statistics() const noexcept;

    /**
     * @brief Shutdown.
     * @note Exception safety: noexcept.
     */
    void shutdown() noexcept;

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
