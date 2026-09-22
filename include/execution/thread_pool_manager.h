/**
 * @file thread_pool_manager.h
 * @brief Bounded fixed-worker execution pool for the execution module.
 *
 * The current implementation exposes a bounded submission queue serviced by a
 * fixed set of worker threads. The class keeps per-thread queue structures as
 * reserved internal scaffolding for future steal-path work, but live work is
 * currently routed through a single shared dispatch queue.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace themis::resource {

/**
 * @brief One task submitted to the execution pool.
 */
struct WorkItem {
    using Fn = std::function<void()>;

    Fn          fn;       ///< Callable to execute.
    std::string name;     ///< Optional diagnostic label.

    /**
     * @brief Construct a work item.
     * @param f Callable to run on a worker thread.
     * @param n Optional diagnostic name.
     */
    explicit WorkItem(Fn f, std::string n = {})
        : fn(std::move(f)), name(std::move(n)) {}
};

/**
 * @brief Bounded worker pool for execution tasks.
 *
 * @note Despite the type name, the current runtime path uses a central shared
 *       dispatch queue only. Reserved per-thread queues are not yet populated
 *       by `submit()`.
 */
class WorkStealingThreadPool {
public:
    /**
     * @brief Runtime configuration for the execution pool.
     */
    struct Config {
        std::size_t min_threads       = 1;    ///< Number of workers started immediately and kept alive until shutdown.
        std::size_t max_threads       = 0;    ///< Upper bound for allocation/clamping; `0` defaults to `4 * hardware_concurrency()`.
        std::size_t max_queue_depth   = 1000; ///< Maximum pending task count before submitters wait.
        long        idle_timeout_ms   = 1000; ///< Worker wait interval before re-checking the queue; not a retirement timer.
    };

    /**
     * @brief Snapshot of pool runtime counters.
     */
    struct Statistics {
        std::size_t active_threads  = 0; ///< Workers currently running and not yet joined.
        std::size_t queued_items    = 0; ///< Pending items currently stored in the dispatch queue.
        std::uint64_t completed     = 0; ///< Tasks whose callables returned without throwing.
        std::uint64_t failed        = 0; ///< Tasks whose callables threw and were caught by the pool.
        double   p50_latency_us     = 0.0; ///< Median task runtime from the bounded sample buffer.
        double   p99_latency_us     = 0.0; ///< P99 task runtime from the bounded sample buffer.
    };

    /**
     * @brief Construct a pool with default configuration.
     */
    WorkStealingThreadPool();

    /**
     * @brief Construct a pool with an explicit configuration.
     * @param cfg Worker-count and queue-capacity settings.
     */
    explicit WorkStealingThreadPool(const Config& cfg);

    /**
     * @brief Destroy the pool after initiating shutdown.
     */
    ~WorkStealingThreadPool();

    /**
     * @brief Copying is disabled because the pool owns worker threads and synchronization state.
     * @param other Unused source pool instance.
     */
    WorkStealingThreadPool(const WorkStealingThreadPool& other) = delete;

    /**
     * @brief Copy assignment is disabled because the pool owns worker threads and synchronization state.
     * @param other Unused source pool instance.
     * @return This pool instance; the operator is deleted and cannot be used.
     */
    WorkStealingThreadPool& operator=(const WorkStealingThreadPool& other) = delete;

    /**
     * @brief Submit a work item to the bounded dispatch queue.
     * @param item Work item to move into the pool.
     * @param timeout Maximum time to wait for queue capacity.
     * @return `true` on success, or `false` if shutdown is in progress or the
     *         queue stays full until `timeout` elapses.
     */
    bool submit(WorkItem item,
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Submit a callable directly.
     * @param fn Callable to execute.
     * @param name Optional diagnostic label.
     * @param timeout Maximum time to wait for queue capacity.
     * @return Same result semantics as `submit(WorkItem, timeout)`.
     */
    bool submit(std::function<void()> fn, std::string name = {},
                std::chrono::milliseconds timeout = std::chrono::seconds(5));

    /**
     * @brief Wait until the pending queue drains or the timeout elapses.
     * @param timeout Maximum time to poll for an empty queue.
     * @return `true` when the queue drained before the deadline, otherwise `false`.
     */
    bool waitAll(std::chrono::milliseconds timeout = std::chrono::seconds(30));

    /**
     * @brief Return a snapshot of worker, queue, and latency statistics.
     * @return Statistics copied from the current pool state.
     */
    [[nodiscard]] Statistics statistics() const noexcept;

    /**
     * @brief Stop accepting work, drain pending items, wake workers, and join them.
     * @param drain_timeout Maximum time to wait for queue drain before forcing the wake/join sequence.
     */
    void shutdown(std::chrono::milliseconds drain_timeout = std::chrono::seconds(30));

    /**
     * @brief Report whether shutdown has started.
     * @return `true` once shutdown has been requested.
     */
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    /**
     * @brief Return the number of currently active worker threads.
     * @return Active worker count.
     */
    [[nodiscard]] std::size_t thread_count() const noexcept;

private:
    /**
     * @brief Reserved per-worker queue metadata for future steal-path activation.
     */
    struct ThreadQueue {
        std::deque<WorkItem>  items;
        mutable std::mutex    lock;
        std::atomic<bool>     active{false};

        /**
         * @brief Try to steal one item from the back of a reserved per-thread queue.
         * @param out Receives the stolen work item on success.
         * @return `true` when a work item was available.
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
     * @brief Worker-thread main loop.
     * @param thread_idx Index of the worker inside the pre-created queue array.
     */
    void workerLoop(std::size_t thread_idx);

    /**
     * @brief Try to obtain the next work item.
     * @param own_idx Index of the requesting worker.
     * @param out Receives the next work item on success.
     * @return `true` when work was acquired.
     *
     * The current implementation reads only from the shared dispatch queue.
     * `own_idx` is reserved for future steal-path activation.
     */
    bool tryGetWork(std::size_t own_idx, WorkItem& out);

    Config  cfg_;

    std::deque<WorkItem>     dispatch_queue_;
    mutable std::mutex       dispatch_mutex_;
    std::condition_variable  dispatch_cv_;     ///< Notified when queue becomes non-empty or shutdown begins.
    std::condition_variable  capacity_cv_;     ///< Notified when queue capacity becomes available.

    std::vector<std::unique_ptr<ThreadQueue>> queues_;
    mutable std::mutex                        queues_mutex_;

    std::vector<std::thread> workers_;
    mutable std::mutex       workers_mutex_;

    std::atomic<bool>        shutdown_{false};
    std::atomic<std::size_t> active_threads_{0};
    std::atomic<std::size_t> queued_count_{0};
    std::atomic<uint64_t>    completed_{0};
    std::atomic<uint64_t>    failed_{0};

    mutable std::mutex       latency_mutex_;
    std::vector<double>      latency_samples_us_;
};

}  // namespace themis::resource
