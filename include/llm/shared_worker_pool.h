/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shared_worker_pool.h                               ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-14 18:39:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     141                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a2c5bc969d  2026-02-22  feat(llm): add SharedWorkerPool shared between AsyncInfer... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Shared work-stealing thread pool for AsyncInferenceEngine and
 *        InferenceEngineEnhanced.
 *
 * Design:
 * - A single global priority queue receives new tasks from either engine.
 * - Each worker drains from the global queue into its own local deque
 *   (reduces contention on the global mutex).
 * - Idle workers steal tasks from the back of sibling workers' deques.
 *
 * Thread-count defaults to std::thread::hardware_concurrency();
 * configurable via Config::num_threads.
 *
 * Both engines can share one instance:
 * @code
 *   auto pool = std::make_shared<SharedWorkerPool>();
 *   AsyncInferenceEngine async_eng(plugin, async_cfg, pool);
 *   InferenceEngineEnhanced enh_eng(enh_cfg, pool);
 * @endcode
 */
class SharedWorkerPool {
public:
    struct Config {
        /// Worker thread count; 0 = std::thread::hardware_concurrency().
        size_t num_threads    = 0;
        /// Maximum tasks that may be queued across the whole pool.
        size_t max_queue_size = 10000;
    };

    SharedWorkerPool();
    explicit SharedWorkerPool(const Config& config);
    ~SharedWorkerPool();

    SharedWorkerPool(const SharedWorkerPool&)            = delete;
    SharedWorkerPool& operator=(const SharedWorkerPool&) = delete;

    /**
     * @brief Submit a callable with a priority.
     *
     * Higher priority value → picked up sooner.
     * Returns false and drops the task when the queue is at capacity.
     *
     * @param task     Callable to execute on a worker thread.
     * @param priority Scheduling hint (higher = more urgent).
     * @return true if queued, false if queue is full.
     */
    bool submit(std::function<void()> task, int priority = 0);

    /// Current number of tasks queued (not yet executing).
    size_t   queueDepth()      const;
    /// Total tasks completed since pool creation.
    uint64_t tasksCompleted()  const;
    /// Number of worker threads in the pool.
    size_t   numThreads()      const;

    /// JSON metrics snapshot (queue_depth, tasks_completed, num_threads).
    json getMetrics() const;

    /// Graceful shutdown — stops accepting new tasks and joins workers.
    void shutdown();
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

    void workerLoop(size_t thread_id);
    bool trySteal(size_t thread_id, Task& out_task);
};

} // namespace llm
} // namespace themis
