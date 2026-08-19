/**
 * @file thread_pool_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     
    /**
     * @brief Submit a task to the thread pool for execution
     * 
     * Submits a task to the work queue. The task will be executed by an available
     * worker thread based on priority. If queue is full, returns false after timeout.
     * 
     * @param task Shared pointer to task to execute
     * @param timeout Maximum time to wait for queue space (default 5 seconds)
     * @return true if task accepted, false if queue full or timeout exceeded
     * 
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Pool is stopped (not running) | THREADPOOL_SHUTDOWN (9071) | Error | pool_name | Return false (THREAD_POOL_OVERLOAD) |
     * | Queue at capacity, timeout elapsed | THREADPOOL_QUEUE_FULL (9070) | Error | pool_name, queue_size, task_name | Return false (THREAD_POOL_OVERLOAD) |
     *
     * @degradation explicit false return on any rejection; no silent discard
     * @bounded_resources
     * - Queue depth capped at config.queue_size (default: 1,000 tasks)
     * - Task wait time bounded by timeout parameter
     * 
     * @thread_safety Thread-safe for concurrent submit() calls
     * @performance O(log n) insertion into priority queue where n = queue depth
     * 
     * @see ErrorCode 9070-9079 for concurrency error taxonomy
     */
    bool submit(std::shared_ptr<Task> task, std::chrono::milliseconds timeout = std::chrono::seconds(5));
     
    /**
     * @brief Wait for all submitted tasks to complete
     * 
     * Blocks until all previously submitted tasks have finished execution
     * or timeout is exceeded.
     * 
     * @param timeout Maximum time to wait (default 30 seconds)
     * @return true if all tasks completed; false if timeout exceeded
     * 
     * @error_contract
     * - Returns false if timeout exceeded (logs ERR_THREADPOOL_TIMEOUT)
     * - If pool not running: returns false with warning
     * - Task execution exceptions: logged but waitAll() returns true (tasks finished)
     * 
     * @thread_safety Blocking call; thread-safe for concurrent waitAll() calls
     * @performance O(n) polling where n = number of active tasks
     * 
     * @note Use sparingly in production; consider async patterns instead
     * @note Set timeout appropriately for your task workload
     */
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

    // Priority comparator: higher priority value → dequeued first.
    struct PriorityCompare {
        bool operator()(const std::shared_ptr<Task>& a,
                        const std::shared_ptr<Task>& b) const {
            return static_cast<int>(a->getPriority()) <
                   static_cast<int>(b->getPriority());
        }
    };
    
    std::priority_queue<
        std::shared_ptr<Task>,
        std::vector<std::shared_ptr<Task>>,
        PriorityCompare
    > task_queue_;
    std::vector<std::thread> workers_;
    
    mutable std::shared_mutex mutex_;
    std::condition_variable_any cv_;
    
    Config config_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> active_threads_{0};
    std::atomic<uint64_t> total_executed_{0};
    std::atomic<uint64_t> total_failed_{0};

    // Latency tracking (running sum + count protected by mutex_).
    double latency_sum_ms_  = 0.0;
    uint64_t latency_count_ = 0;
};

/**
 * @brief Global ThreadPoolManager with different pool types
 */
class ThreadPoolManager {
public:
    // Resource limits (Phase 2.6 cross-cutting hardening)
    static constexpr size_t DEFAULT_QUEUE_DEPTH = 10'000;  // Tasks queued per pool
    static constexpr size_t MAX_QUEUE_DEPTH = 100'000;  // Hard limit
    static constexpr size_t MAX_THREAD_COUNT = 1024;  // Per-pool thread limit
    static constexpr size_t DEFAULT_THREAD_COUNT = 4;  // Typical IO pool size
    
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

