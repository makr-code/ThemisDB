/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            thread_pool_manager.cpp                            ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:38:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac1c6ff53e  2026-03-26  fix: thread pool priority queue + latency, lora memory/ba... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/thread_pool_manager.h"
#include <algorithm>

namespace themis::utils {

// ============================================================================
// ThreadPool Implementation
// ============================================================================

ThreadPool::ThreadPool(const Config& config)
    : config_(config), running_(true) {
    
    spdlog::info("Starting ThreadPool '{}' with {} min/{} max threads",
                config.name, config.min_threads, config.max_threads);
    
    // Create minimum threads
    for (size_t i = 0; i < config.min_threads; i++) {
        workers_.emplace_back([this]() { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::workerLoop() {
    while (running_) {
        std::shared_ptr<Task> task;
        
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            
            // Wait for task
            bool available = cv_.wait_for(lock, std::chrono::seconds(1), [this]() {
                return !task_queue_.empty() || !running_;
            });
            
            if (!available || task_queue_.empty()) {
                continue;
            }
            
            // Dequeue highest-priority task from the priority queue.
            task = task_queue_.top();
            task_queue_.pop();
        }
        
        if (task) {
            active_threads_++;
            auto exec_start = std::chrono::steady_clock::now();
            try {
                task->execute();
                total_executed_++;
            } catch (const std::exception& e) {
                spdlog::error("Task {} failed with exception: {}", task->getName(), e.what());
                total_failed_++;
            } catch (...) {
                spdlog::error("Task {} failed with unknown exception", task->getName());
                total_failed_++;
            }
            double latency_ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - exec_start
            ).count();
            {
                std::unique_lock<std::shared_mutex> lk(mutex_);
                latency_sum_ms_ += latency_ms;
                ++latency_count_;
            }
            active_threads_--;
        }
    }
}

bool ThreadPool::submit(std::shared_ptr<Task> task, std::chrono::milliseconds timeout) {
    if (!running_) {
        spdlog::warn("ThreadPool '{}' is not running", config_.name);
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Wait for space in queue with timeout
    bool space = cv_.wait_for(lock, timeout, [this]() {
        return task_queue_.size() < config_.queue_size;
    });
    
    if (!space) {
        spdlog::error("ThreadPool '{}' queue full, rejecting task", config_.name);
        return false;
    }
    
    task_queue_.push(task);
    cv_.notify_one();
    
    return true;
}

bool ThreadPool::waitAll(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            if (task_queue_.empty() && active_threads_ == 0) {
                return true;
            }
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

ThreadPool::Statistics ThreadPool::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    Statistics stats;
    stats.active_threads = active_threads_.load();
    stats.idle_threads = workers_.size() - stats.active_threads;
    stats.queued_tasks = task_queue_.size();
    stats.total_executed = total_executed_.load();
    stats.total_failed = total_failed_.load();
    
    // Average latency from the running sum maintained in workerLoop.
    stats.average_task_latency_ms =
        (latency_count_ > 0) ? (latency_sum_ms_ / static_cast<double>(latency_count_)) : 0.0;
    
    return stats;
}

void ThreadPool::shutdown() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    spdlog::info("Shutting down ThreadPool '{}'", config_.name);
    
    // Wake up all workers
    cv_.notify_all();
    
    // Wait for all workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

// ============================================================================
// ThreadPoolManager Implementation
// ============================================================================

ThreadPoolManager::ThreadPoolManager()
    : ThreadPoolManager(Config{}) {
}

ThreadPoolManager::ThreadPoolManager(const Config& config)
    : config_(config), running_(true) {
    
    // Set default pool names if not provided
    ThreadPool::Config io_config = config_.io_pool;
    if (io_config.name.empty()) {
        io_config.name = "IO-Pool";
    }
    if (io_config.min_threads == 0) {
        io_config.min_threads = 2;
    }
    if (io_config.max_threads == 0) {
        io_config.max_threads = 16;
    }
    
    ThreadPool::Config cpu_config = config_.cpu_pool;
    if (cpu_config.name.empty()) {
        cpu_config.name = "CPU-Pool";
    }
    if (cpu_config.min_threads == 0) {
        cpu_config.min_threads = 2;
    }
    if (cpu_config.max_threads == 0) {
        cpu_config.max_threads = 8;
    }
    
    ThreadPool::Config blocking_config = config_.blocking_pool;
    if (blocking_config.name.empty()) {
        blocking_config.name = "Blocking-Pool";
    }
    if (blocking_config.min_threads == 0) {
        blocking_config.min_threads = 2;
    }
    if (blocking_config.max_threads == 0) {
        blocking_config.max_threads = 32;
    }
    
    // Create thread pools
    io_pool_ = std::make_unique<ThreadPool>(io_config);
    cpu_pool_ = std::make_unique<ThreadPool>(cpu_config);
    blocking_pool_ = std::make_unique<ThreadPool>(blocking_config);
    
    // Start metrics thread if enabled
    if (config_.enable_metrics) {
        metrics_thread_ = std::thread([this]() { metricsLoop(); });
    }
}

ThreadPoolManager::~ThreadPoolManager() {
    shutdown();
}

bool ThreadPoolManager::submit(
    PoolType pool,
    std::shared_ptr<Task> task,
    std::chrono::milliseconds timeout
) {
    switch (pool) {
        case PoolType::IO:
            return io_pool_->submit(task, timeout);
        case PoolType::CPU:
            return cpu_pool_->submit(task, timeout);
        case PoolType::BLOCKING:
            return blocking_pool_->submit(task, timeout);
        default:
            spdlog::error("Unknown pool type");
            return false;
    }
}

ThreadPool::Statistics ThreadPoolManager::getPoolStatistics(PoolType pool) const {
    switch (pool) {
        case PoolType::IO:
            return io_pool_->getStatistics();
        case PoolType::CPU:
            return cpu_pool_->getStatistics();
        case PoolType::BLOCKING:
            return blocking_pool_->getStatistics();
        default:
            return ThreadPool::Statistics{};
    }
}

ThreadPoolManager::GlobalStatistics ThreadPoolManager::getStatistics() const {
    GlobalStatistics stats;
    stats.io_stats = io_pool_->getStatistics();
    stats.cpu_stats = cpu_pool_->getStatistics();
    stats.blocking_stats = blocking_pool_->getStatistics();
    stats.snapshot_time = std::chrono::steady_clock::now();
    return stats;
}

void ThreadPoolManager::shutdown() {
    if (!running_.exchange(false)) {
        return;  // Already stopped
    }
    
    spdlog::info("Shutting down ThreadPoolManager");
    
    // Shutdown all pools
    if (io_pool_) {
        io_pool_->shutdown();
    }
    if (cpu_pool_) {
        cpu_pool_->shutdown();
    }
    if (blocking_pool_) {
        blocking_pool_->shutdown();
    }
    
    // Wait for metrics thread
    if (metrics_thread_.joinable()) {
        metrics_thread_.join();
    }
}

void ThreadPoolManager::metricsLoop() {
    while (running_) {
        std::this_thread::sleep_for(config_.metrics_interval);
        
        if (!running_) {
            break;
        }
        
        // Log metrics
        auto stats = getStatistics();
        
        spdlog::debug("ThreadPool Metrics - IO: active={}, queued={}, executed={}, failed={}",
                     stats.io_stats.active_threads,
                     stats.io_stats.queued_tasks,
                     stats.io_stats.total_executed,
                     stats.io_stats.total_failed);
        
        spdlog::debug("ThreadPool Metrics - CPU: active={}, queued={}, executed={}, failed={}",
                     stats.cpu_stats.active_threads,
                     stats.cpu_stats.queued_tasks,
                     stats.cpu_stats.total_executed,
                     stats.cpu_stats.total_failed);
        
        spdlog::debug("ThreadPool Metrics - Blocking: active={}, queued={}, executed={}, failed={}",
                     stats.blocking_stats.active_threads,
                     stats.blocking_stats.queued_tasks,
                     stats.blocking_stats.total_executed,
                     stats.blocking_stats.total_failed);
    }
}

// Global singleton
ThreadPoolManager& getThreadPoolManager() {
    static ThreadPoolManager instance;
    return instance;
}

} // namespace themis::utils
