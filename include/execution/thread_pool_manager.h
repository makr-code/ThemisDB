/**
 * @file thread_pool_manager.h
 * @brief Phase 3 P3-03-C: Central-queue thread pool for ThemisDB execution layer.
 *
 * Provides a self-contained, bounded thread pool with:
 *  - Central dispatch queue: all work items share a single deque; per-thread
 *    deques are pre-allocated for a future work-stealing upgrade path.
 *  - Backpressure: @ref submit() blocks (with timeout) when the global pending
 *    count exceeds @c Config::max_queue_depth.
 *  - Dynamic thread scaling: threads added when queue depth > 2× threshold;
 *    idle threads exit after @c Config::idle_timeout_ms.
 *  - Exception safety: exceptions in tasks are caught, logged, and the thread
 *    continues to process subsequent work items.
 *  - Graceful shutdown: in-flight work completes before threads exit.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Block B P3-03-C delivery
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace themis::resource {

// ============================================================================
// WorkItem
// ============================================================================

struct WorkItem {
    using Fn = std::function<void()>;

    Fn          fn;       ///< The callable to execute.
    std::string name;     ///< Optional diagnostic name.

    explicit WorkItem(Fn f, std::string n = {})
        : fn(std::move(f)), name(std::move(n)) {}
};

// ============================================================================
// WorkStealingThreadPool
// ============================================================================

class WorkStealingThreadPool {
public:
    struct Config {
        std::size_t min_threads       = 1;    ///< Minimum worker threads.
        std::size_t max_threads       = 0;    ///< 0 = 4 × hardware_concurrency.
        std::size_t max_queue_depth   = 1000; ///< Backpressure trigger.
        long        idle_timeout_ms   = 1000;
    };

    struct Statistics {
        std::size_t active_threads  = 0;
        std::size_t queued_items    = 0;
        std::uint64_t completed     = 0;
        std::uint64_t failed        = 0;
        double   p50_latency_us     = 0.0;
        double   p99_latency_us     = 0.0;
    };

    WorkStealingThreadPool();

    /**
     * @brief Work Stealing Thread Pool.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit WorkStealingThreadPool(const Config& cfg);

    ~WorkStealingThreadPool();

    // Non-copyable, non-movable.
    WorkStealingThreadPool(const WorkStealingThreadPool&)            = delete;
    WorkStealingThreadPool& operator=(const WorkStealingThreadPool&) = delete;

    bool submit(WorkItem item,
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    bool submit(std::function<void()> fn, std::string name = {},
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    bool waitAll(std::chrono::milliseconds timeout = std::chrono::seconds(30));

    [[nodiscard]] Statistics statistics() const noexcept;

    void shutdown(std::chrono::milliseconds drain_timeout = std::chrono::seconds(30));

    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t thread_count() const noexcept;

private:
    // -----------------------------------------------------------------------
    // Per-thread work deque (with its own lock).
    // -----------------------------------------------------------------------
    struct ThreadQueue {
        std::deque<WorkItem>  items;
        mutable std::mutex    lock;
        std::atomic<bool>     active{false};

        /**
         * @brief Try to steal one item from the back.
         * @param[in,out] out Input/output parameter.
         * @return True when the operation succeeds.
         * @details Calls: lk(), empty(), std::move(), back(), pop_back().
         */
        bool trySteal(WorkItem& out) {
            std::lock_guard<std::mutex> lk(lock);
            if (items.empty()) {
              return false;
            }
            out = std::move(items.back());
            items.pop_back();
            return true;
        }
    };

    /**
     * @brief Worker Loop.
     * @param[in] thread_idx Input parameter.
     */
    void workerLoop(std::size_t thread_idx);
    /**
     * @brief Try Get Work.
     * @param[in] own_idx Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool tryGetWork(std::size_t own_idx, WorkItem& out);

    Config  cfg_;

    // Shared dispatch queue + backpressure.
    std::deque<WorkItem>     dispatch_queue_;
    mutable std::mutex       dispatch_mutex_;
    std::condition_variable  dispatch_cv_;     ///< Notified when queue non-empty.
    std::condition_variable  capacity_cv_;     ///< Notified when queue drains.

    std::vector<std::unique_ptr<ThreadQueue>> queues_;
    mutable std::mutex                        queues_mutex_;

    std::vector<std::thread> workers_;
    mutable std::mutex       workers_mutex_;

    std::atomic<bool>        shutdown_{false};
    std::atomic<std::size_t> active_threads_{0};
    std::atomic<std::size_t> queued_count_{0};
    std::atomic<uint64_t>    completed_{0};
    std::atomic<uint64_t>    failed_{0};

    // Latency histogram (microseconds).
    mutable std::mutex       latency_mutex_;
    std::vector<double>      latency_samples_us_;
};

}  // namespace themis::resource
