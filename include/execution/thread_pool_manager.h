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

/**
 * @brief A single unit of work submitted to @ref WorkStealingThreadPool.
 */
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

/**
 * @brief Bounded central-queue thread pool.
 *
 * ### Dispatch design
 * All submitted work items are placed into a single shared dispatch queue.
 * Each thread pops work from that central queue.  Per-thread @c ThreadQueue
 * deques are pre-allocated but not currently used for dispatch; they are
 * retained for a future work-stealing upgrade path.
 *
 * ### Backpressure
 * If the total pending item count exceeds @p Config::max_queue_depth,
 * @ref submit() blocks until capacity is available or the supplied
 * @p timeout elapses.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 */
class WorkStealingThreadPool {
public:
    /**
     * @brief Pool configuration.
     */
    struct Config {
        std::size_t min_threads       = 1;    ///< Minimum worker threads.
        std::size_t max_threads       = 0;    ///< 0 = 4 × hardware_concurrency.
        std::size_t max_queue_depth   = 1000; ///< Backpressure trigger.
        /// Milliseconds a thread waits for work before checking idle policy.
        long        idle_timeout_ms   = 1000;
    };

    /**
     * @brief Runtime statistics.
     */
    struct Statistics {
        std::size_t active_threads  = 0;
        std::size_t queued_items    = 0;
        std::uint64_t completed     = 0;
        std::uint64_t failed        = 0;
        double   p50_latency_us     = 0.0;
        double   p99_latency_us     = 0.0;
    };

    /**
     * @brief Constructs the pool with default configuration.
     */
    WorkStealingThreadPool();

    /**
     * @brief Constructs the pool and starts @p cfg.min_threads workers.
     * @param cfg  Pool configuration.
     */
    explicit WorkStealingThreadPool(const Config& cfg);

    ~WorkStealingThreadPool();

    // Non-copyable, non-movable.
    WorkStealingThreadPool(const WorkStealingThreadPool&)            = delete;
    WorkStealingThreadPool& operator=(const WorkStealingThreadPool&) = delete;

    /**
     * @brief Submits a work item to the pool.
     *
     * Blocks if the pool is at backpressure capacity until @p timeout elapses
     * or capacity becomes available.
     *
     * @param item     Work to execute.
     * @param timeout  Maximum time to wait for capacity.
     * @return @c true if the item was accepted; @c false on timeout or shutdown.
     */
    bool submit(WorkItem item,
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Convenience overload: submit a raw callable.
     *
     * @param fn       Callable.
     * @param name     Optional diagnostic name.
     * @param timeout  Maximum time to wait.
     * @return @c true if accepted.
     */
    bool submit(std::function<void()> fn, std::string name = {},
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Waits for all currently queued work to complete.
     *
     * @param timeout  Maximum wait duration.
     * @return @c true if all work drained within @p timeout.
     */
    bool waitAll(std::chrono::milliseconds timeout = std::chrono::seconds(30));

    /// @brief Returns current statistics.
    [[nodiscard]] Statistics statistics() const noexcept;

    /**
     * @brief Initiates graceful shutdown.
     *
     * No new work is accepted.  In-flight items complete before threads exit.
     * Blocks until all threads have joined.
     *
     * @param drain_timeout  Maximum time to wait for in-flight work to drain.
     */
    void shutdown(std::chrono::milliseconds drain_timeout = std::chrono::seconds(30));

    /// @brief Returns true once shutdown has been requested.
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    /// @brief Current number of worker threads.
    [[nodiscard]] std::size_t thread_count() const noexcept;

private:
    // -----------------------------------------------------------------------
    // Per-thread work deque (with its own lock).
    // -----------------------------------------------------------------------
    struct ThreadQueue {
        std::deque<WorkItem>  items;
        mutable std::mutex    lock;
        std::atomic<bool>     active{false};

        // Try to steal one item from the back.
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

    void workerLoop(std::size_t thread_idx);
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
