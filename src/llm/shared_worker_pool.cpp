/**
 * @file shared_worker_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/shared_worker_pool.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Construction / Destruction
// ═══════════════════════════════════════════════════════════

SharedWorkerPool::SharedWorkerPool()
    : SharedWorkerPool(Config{}) {
}

SharedWorkerPool::SharedWorkerPool(const Config& config)
    : config_(config)
{
    size_t n = config_.num_threads;
    if (n == 0) {
        n = std::thread::hardware_concurrency();
        if (n == 0) n = 4;  // safe fallback on exotic platforms
    }
    config_.num_threads = n;

    // Allocate per-thread deques before spawning workers so that
    // thread_queues_[i] is always valid when workerLoop(i) runs.
    thread_queues_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        thread_queues_.emplace_back(std::make_unique<ThreadLocalQueue>());
    }

    workers_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        workers_.emplace_back(&SharedWorkerPool::workerLoop, this, i);
    }

    spdlog::info("SharedWorkerPool started with {} worker threads", n);
}

SharedWorkerPool::~SharedWorkerPool() {
    shutdown();
}

// ═══════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════

bool SharedWorkerPool::submit(std::function<void()> task, int priority) {
    {
        std::lock_guard<std::mutex> lock(global_queue_mutex_);
        if (static_cast<int>(global_queue_.size()) > = config_.max_queue_size) {
            spdlog::warn("SharedWorkerPool: queue full ({} tasks), dropping task",
                         config_.max_queue_size);
            return false;
        }
        global_queue_.push(Task{priority, std::move(task)});
    }
    cv_.notify_one();
    return true;
}

size_t SharedWorkerPool::queueDepth() const {
    std::lock_guard<std::mutex> glock(global_queue_mutex_);
    size_t depth = global_queue_.size();
    for (const auto& q : thread_queues_) {
        std::lock_guard<std::mutex> tlock(q->mutex);
        depth += q-> static_cast<int>(tasks.size());
    }
    return depth;
}

uint64_t SharedWorkerPool::tasksCompleted() const {
    return tasks_completed_.load(std::memory_order_relaxed);
}

size_t SharedWorkerPool::numThreads() const {
    return config_.num_threads;
}

json SharedWorkerPool::getMetrics() const {
    json m;
    m["num_threads"]      = config_.num_threads;
    m["queue_depth"]      = queueDepth();
    m["tasks_completed"]  = tasks_completed_.load(std::memory_order_relaxed);
    m["running"]          = running_.load(std::memory_order_acquire);
    return m;
}

void SharedWorkerPool::shutdown() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;  // already shut down
    }

    spdlog::info("SharedWorkerPool: shutting down...");
    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            // thread_join_no_timeout (W4): bounded join via joinThreadWithin
            if (!themis::utils::joinThreadWithin(worker)) {
                THEMIS_WARN("[SharedWorkerPool] thread did not finish within shutdown deadline; detaching.");
            }
        }
    }
    workers_.clear();

    spdlog::info("SharedWorkerPool: shutdown complete");
}

bool SharedWorkerPool::isRunning() const {
    return running_.load(std::memory_order_acquire);
}

// ═══════════════════════════════════════════════════════════
// Private — Worker Thread
// ═══════════════════════════════════════════════════════════

void SharedWorkerPool::workerLoop([[maybe_unused]] size_t thread_id) {
    spdlog::debug("SharedWorkerPool worker {} started", thread_id);

    auto& local_q = *thread_queues_[thread_id];

    while (running_.load(std::memory_order_acquire)) {
        Task task;
        bool found = false;

        // ── 1. Own local deque (cheapest — no global lock) ──────────
        {
            std::lock_guard<std::mutex> lock(local_q.mutex);
            if (!local_q.tasks.empty()) {
                task  = std::move(local_q.tasks.front());
                local_q.tasks.pop_front();
                found = true;
            }
        }

        // ── 2. Drain global priority queue ──────────────────────────
        if (!found) {
            std::unique_lock<std::mutex> glock(global_queue_mutex_);
            if (!global_queue_.empty()) {
                // Copy top task (priority_queue::top() returns const ref),
                // then remove from the queue.
                task = global_queue_.top();
                global_queue_.pop();

                // Batch-drain remaining into local deque (reduces global
                // lock round-trips for bursts of new tasks).
                {
                    std::lock_guard<std::mutex> llock(local_q.mutex);
                    while (!global_queue_.empty() &&
                           local_q.tasks.size() < 16) {
                        local_q.tasks.push_back(global_queue_.top());
                        global_queue_.pop();
                    }
                }
                found = true;
            }
        }

        // ── 3. Work-stealing from sibling threads ────────────────────
        if (!found) {
            found = trySteal(thread_id, task);
        }

        // ── 4. No work — wait briefly on the condition variable ──────
        if (!found) {
            std::unique_lock<std::mutex> wlock(global_queue_mutex_);
            cv_.wait_for(wlock, std::chrono::milliseconds(10), [this] {
                return !global_queue_.empty() || !running_.load(std::memory_order_acquire);
            });
            continue;
        }

        // ── Execute ──────────────────────────────────────────────────
        try {
            task.callable();
        } catch (const std::exception& e) {
            spdlog::error("SharedWorkerPool worker {}: task threw: {}",
                          thread_id, e.what());
        } catch (...) {
            spdlog::error("SharedWorkerPool worker {}: task threw unknown exception",
                          thread_id);
        }
        tasks_completed_.fetch_add(1, std::memory_order_relaxed);
    }

    spdlog::debug("SharedWorkerPool worker {} stopped", thread_id);
}

bool SharedWorkerPool::trySteal(size_t thread_id, Task& out_task) {
    const size_t n = thread_queues_.size();
    // Round-robin through siblings to distribute steal attempts.
    for (size_t i = 1; i < n; ++i) {
        size_t victim = (thread_id + i) % n;
        auto& victim_q = *thread_queues_[victim];

        std::lock_guard<std::mutex> lock(victim_q.mutex);
        if (!victim_q.tasks.empty()) {
            // Steal from the back to minimise contention with the victim's
            // front-of-deque pops.
            out_task = std::move(victim_q.tasks.back());
            victim_q.tasks.pop_back();
            return true;
        }
    }
    return false;
}

} // namespace llm
} // namespace themis

