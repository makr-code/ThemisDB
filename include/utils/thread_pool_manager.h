/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            thread_pool_manager.h                              ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     208                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <string>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <shared_mutex>
#include <condition_variable>
#include <spdlog/spdlog.h>

namespace themis::utils {

/**
 * @brief Task to be executed in thread pool
 */
class Task {
public:
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };
    
    using Callback = std::function<void()>;
    
    explicit Task(
        Callback callback,
        Priority priority = Priority::NORMAL,
        std::string name = ""
    ) : callback_(std::move(callback)), priority_(priority), name_(std::move(name)),
        created_at_(std::chrono::steady_clock::now()) {}
    
    void execute() {
        auto start = std::chrono::steady_clock::now();
        callback_();
        auto duration = std::chrono::steady_clock::now() - start;
        execution_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }
    
    Priority getPriority() const { return priority_; }
    const std::string& getName() const { return name_; }
    auto getExecutionTime() const { return execution_time_; }
    
private:
    Callback callback_;
    Priority priority_;
    std::string name_;
    std::chrono::steady_clock::time_point created_at_;
    std::chrono::milliseconds execution_time_{0};
};

/**
 * @brief Single thread pool (IO, CPU, or Blocking)
 */
class ThreadPool {
public:
    struct Config {
        size_t min_threads = 2;          // Minimum threads
        size_t max_threads = 16;         // Maximum threads
        size_t queue_size = 1000;        // Max queued tasks
        std::string name;                // Pool name for logging
    };
    
    explicit ThreadPool(const Config& config);
    ~ThreadPool();
    
    // Submit task to pool
    bool submit(std::shared_ptr<Task> task, std::chrono::milliseconds timeout = std::chrono::seconds(5));
    
    // Wait for all tasks to complete
    bool waitAll(std::chrono::milliseconds timeout = std::chrono::seconds(30));
    
    // Get pool statistics
    struct Statistics {
        size_t active_threads;
        size_t idle_threads;
        size_t queued_tasks;
        size_t total_executed;
        size_t total_failed;
        double average_task_latency_ms;
    };
    Statistics getStatistics() const;
    
    // Graceful shutdown
    void shutdown();
    
    bool isRunning() const { return running_.load(); }
    
private:
    void workerLoop();
    
    std::queue<std::shared_ptr<Task>> task_queue_;
    std::vector<std::thread> workers_;
    
    mutable std::shared_mutex mutex_;
    std::condition_variable_any cv_;
    
    Config config_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> active_threads_{0};
    std::atomic<uint64_t> total_executed_{0};
    std::atomic<uint64_t> total_failed_{0};
};

/**
 * @brief Global ThreadPoolManager with different pool types
 */
class ThreadPoolManager {
public:
    enum class PoolType {
        IO,        // Network, File I/O operations
        CPU,       // CPU-bound operations
        BLOCKING   // Long-running blocking operations
    };
    
    struct Config {
        ThreadPool::Config io_pool;
        ThreadPool::Config cpu_pool;
        ThreadPool::Config blocking_pool;
        
        bool enable_metrics = true;
        std::chrono::seconds metrics_interval{10};
    };
    
    ThreadPoolManager();
    explicit ThreadPoolManager(const Config& config);
    ~ThreadPoolManager();
    
    // Submit task to appropriate pool
    bool submit(
        PoolType pool,
        std::shared_ptr<Task> task,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );
    
    // Convenience method
    bool submitTask(
        PoolType pool,
        Task::Callback callback,
        const std::string& name = "",
        Task::Priority priority = Task::Priority::NORMAL
    ) {
        auto task = std::make_shared<Task>(std::move(callback), priority, name);
        return submit(pool, task);
    }
    
    // Get statistics for specific pool
    ThreadPool::Statistics getPoolStatistics(PoolType pool) const;
    
    // Get combined statistics
    struct GlobalStatistics {
        ThreadPool::Statistics io_stats;
        ThreadPool::Statistics cpu_stats;
        ThreadPool::Statistics blocking_stats;
        std::chrono::steady_clock::time_point snapshot_time;
    };
    GlobalStatistics getStatistics() const;
    
    // Graceful shutdown of all pools
    void shutdown();
    
private:
    std::unique_ptr<ThreadPool> io_pool_;
    std::unique_ptr<ThreadPool> cpu_pool_;
    std::unique_ptr<ThreadPool> blocking_pool_;
    
    Config config_;
    std::thread metrics_thread_;
    std::atomic<bool> running_{false};
    
    void metricsLoop();
};

// Global singleton
ThreadPoolManager& getThreadPoolManager();

} // namespace themis::utils
