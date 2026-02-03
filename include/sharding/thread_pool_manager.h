// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_THREAD_POOL_MANAGER_H
#define THEMISDB_SHARDING_THREAD_POOL_MANAGER_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <chrono>

namespace themisdb {
namespace sharding {

/**
 * @brief Thread Pool Manager for Centralized Thread Management
 * 
 * Provides a shared thread pool to avoid thread explosion across components.
 * Features:
 * - Fixed core threads + dynamic threads up to max
 * - Task queue with size limit
 * - Task timeout support
 * - Graceful shutdown
 * - Metrics tracking
 */
class ThreadPoolManager {
public:
    using Task = std::function<void()>;
    
    struct Config {
        size_t core_threads{32};
        size_t max_threads{256};
        size_t queue_size{10000};
        std::chrono::milliseconds thread_timeout{60000};
    };
    
    struct Stats {
        size_t core_threads;
        size_t active_threads;
        size_t queued_tasks;
        size_t completed_tasks;
        size_t rejected_tasks;
    };
    
    explicit ThreadPoolManager(const Config& config);
    ~ThreadPoolManager();
    
    /**
     * @brief Submit task to thread pool
     * @param task Task to execute
     * @param timeout Maximum time to wait for queue space
     * @return true if task was submitted, false if queue full or timeout
     */
    bool submit(Task task, std::chrono::milliseconds timeout = std::chrono::seconds(5));
    
    /**
     * @brief Shutdown thread pool
     * @param wait_for_tasks If true, wait for all tasks to complete
     */
    void shutdown(bool wait_for_tasks = true);
    
    /**
     * @brief Get thread pool statistics
     * @return Current stats
     */
    Stats getStats() const;

private:
    Config config_;
    std::vector<std::thread> worker_threads_;
    std::queue<Task> task_queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<size_t> active_threads_{0};
    std::atomic<bool> shutdown_{false};
    std::atomic<size_t> completed_tasks_{0};
    std::atomic<size_t> rejected_tasks_{0};
    
    /**
     * @brief Worker thread function
     */
    void workerThread();
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_THREAD_POOL_MANAGER_H
