/**
 * @file splinterdb.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase3/splinterdb.h"
#include <stdexcept>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace themis {
namespace performance {
namespace phase3 {

// Task queue for compaction work
struct CompactionTask {
    int level = 0;
    std::function<void()> fn;
};

/** @brief Task queue component. */
class TaskQueue {
public:
    void push(CompactionTask task) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
        cv_.notify_one();
    }
    
    bool pop(CompactionTask& task, bool wait = true) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (wait) {
            cv_.wait(lock, [this]() { return !tasks_.empty() || shutdown_; });
        }
        
        if (tasks_.empty() || shutdown_) {
            return false;
        }
        
        task = std::move(tasks_.front());
        tasks_.pop();
        return true;
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cv_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

private:
    std::queue<CompactionTask> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool shutdown_ = false;
};

// Global task queue (in production, would be per-instance)
static TaskQueue g_task_queue;

// Statistics
static std::atomic<size_t> g_compactions_completed{0};
static std::atomic<size_t> g_compactions_in_progress{0};
static std::atomic<double> g_total_compaction_time_ms{0.0};

// ==================== ConcurrentCompactor Implementation ====================

ConcurrentCompactor::ConcurrentCompactor([[maybe_unused]] size_t num_threads)
    : num_threads_(num_threads) {
}

ConcurrentCompactor::~ConcurrentCompactor() {
    stop();
}

void ConcurrentCompactor::start() {
    if (running_.exchange(true, std::memory_order_relaxed)) {
        return;  // Already running
    }
    
    // Start worker threads
    for (size_t i = 0; i < num_threads_; i++) {
        worker_threads_.emplace_back([this]() {
            worker_loop();
        });
    }
}

void ConcurrentCompactor::stop() {
    if (!running_.exchange(false, std::memory_order_relaxed)) {
        return;  // Already stopped
    }
    
    // Signal shutdown
    g_task_queue.shutdown();
    
    // Wait for all workers to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
}

void ConcurrentCompactor::schedule_compaction(int level, std::function<void()> compaction_fn) {
    CompactionTask task;
    task.level = level;
    task.fn = std::move(compaction_fn);
    
    g_task_queue.push(std::move(task));
}

ConcurrentCompactor::Stats ConcurrentCompactor::get_stats() const {
    Stats stats;
    stats.compactions_completed = g_compactions_completed.load(std::memory_order_relaxed);
    stats.compactions_in_progress = g_compactions_in_progress.load(std::memory_order_relaxed);
    
    size_t completed = stats.compactions_completed;
    if (completed > 0) {
        double total_time = g_total_compaction_time_ms.load(std::memory_order_relaxed);
        stats.avg_compaction_time_ms = total_time / completed;
    } else {
        stats.avg_compaction_time_ms = 0.0;
    }
    
    return stats;
}

void ConcurrentCompactor::worker_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        CompactionTask task = {};
        
        if (!g_task_queue.pop(task, true)) {
            continue;  // Shutdown or no tasks
        }
        
        // Execute compaction
        g_compactions_in_progress.fetch_add(1, std::memory_order_relaxed);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            task.fn();
        } catch (...) {
            // In production, log error
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        g_compactions_in_progress.fetch_sub(1, std::memory_order_relaxed);
        g_compactions_completed.fetch_add(1, std::memory_order_relaxed);
        
        // Thread-safe floating-point accumulate via compare-exchange loop
        double duration_ms = static_cast<double>(duration.count());
        double expected = g_total_compaction_time_ms.load(std::memory_order_relaxed);
        while (!g_total_compaction_time_ms.compare_exchange_weak(
                    expected, expected + duration_ms,
                    std::memory_order_release, std::memory_order_relaxed)) {
            // expected is updated by compare_exchange_weak on failure; retry
        }
    }
}

} // namespace phase3
} // namespace performance
} // namespace themis

