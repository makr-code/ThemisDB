/**
 * @file shared_worker_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <thread>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace llm {

class SharedWorkerPool {
public:
    struct Config {
        size_t num_threads    = 0;
        size_t max_queue_size = 10000;
    };

    SharedWorkerPool();
    /**
     * @brief Shared Worker Pool.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SharedWorkerPool(const Config& config);
    ~SharedWorkerPool();

    SharedWorkerPool(const SharedWorkerPool&)            = delete;
    SharedWorkerPool& operator=(const SharedWorkerPool&) = delete;

    bool submit(std::function<void()> task, int priority = 0);

    /**
     * @brief Queue Depth.
     * @return Return value.
     */
    size_t   queueDepth()      const;
    /**
     * @brief Tasks Completed.
     * @return Return value.
     */
    uint64_t tasksCompleted()  const;
    /**
     * @brief Num Threads.
     * @return Return value.
     */
    size_t   numThreads()      const;

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    json getMetrics() const;

    /**
     * @brief Shutdown.
     */
    void shutdown();
    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;

private:
    // ── Internal task representation ────────────────────────────────
    struct Task {
        int                  priority = 0;
        std::function<void()> callable;

        // Reverse comparison: higher priority dequeued first.
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    // ── Global priority queue ────────────────────────────────────────
    std::priority_queue<Task> global_queue_;
    mutable std::mutex        global_queue_mutex_;
    std::condition_variable   cv_;

    // ── Per-thread local deques (work-stealing) ──────────────────────
    struct ThreadLocalQueue {
        std::deque<Task>  tasks;
        mutable std::mutex mutex;
    };
    std::vector<std::unique_ptr<ThreadLocalQueue>> thread_queues_;

    // ── Worker threads ───────────────────────────────────────────────
    std::vector<std::thread> workers_;
    std::atomic<bool>        running_{true};
    std::atomic<uint64_t>    tasks_completed_{0};

    Config config_;

    /**
     * @brief Worker Loop.
     * @param[in] thread_id Identifier of the thread.
     */
    void workerLoop(size_t thread_id);
    /**
     * @brief Try Steal.
     * @param[in] thread_id Identifier of the thread.
     * @param[in,out] out_task Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool trySteal(size_t thread_id, Task& out_task);
};

} // namespace llm
} // namespace themis
