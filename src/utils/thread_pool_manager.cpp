/**
 * @file thread_pool_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=4, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/thread_pool_manager.h"
#include "utils/thread_join_utils.h"
#include <stdexcept>
#include <algorithm>
#include "utils/error_contracts.h"

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
            } catch (const std::string& e) {
                spdlog::error("Task {} failed with exception: {}", task->getName(), e);
                total_failed_++;
            } catch (const char* e) {
                spdlog::error("Task {} failed with exception: {}", task->getName(), (e ? e : "<null>"));
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
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::THREADPOOL_QUEUE_FULL,
            "Task submission rejected – thread pool is stopped; pool_name=" + config_.name,
            "ThreadPool::submit",
            themis::utils::ErrorSeverity::Error,
            false);
        themis::utils::logErrorWithContext(ctx);
        return false;
    }
    
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Wait for space in queue with timeout
    bool space = cv_.wait_for(lock, timeout, [this]() {
        return static_cast<int>(task_queue_.size()) < config_.queue_size;
    });
    
    if (!space) {
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::THREADPOOL_QUEUE_FULL,
            "Task submission rejected – thread pool queue at capacity (THREAD_POOL_OVERLOAD); "
            "pool_name=" + config_.name +
            "; queue_size=" + std::to_string(config_.queue_size) +
            "; task_name=" + (task ? task->getName() : "<null>"),
            "ThreadPool::submit",
            themis::utils::ErrorSeverity::Error,
            false);
        themis::utils::logErrorWithContext(ctx);
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
    stats.idle_threads = static_cast<int>(workers_.size()) - stats.active_threads;
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
    
    // Wait for all workers to finish (bounded join; workers check running_)
    for (auto& worker : workers_) {
        if (!joinThreadWithin(worker)) {
            spdlog::warn("ThreadPool '{}': worker thread did not exit within deadline; "
                         "a detached watcher will complete the join",
                         config_.name);
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
    // Guard against concurrent shutdown: pools are destroyed only after running_
    // is set to false in shutdown(), so checking the flag before dereferencing
    // the unique_ptrs is sufficient (a TOCTOU here is acceptable — the pools are
    // long-lived and the worst case is returning zeroed statistics).
    if (!running_.load(std::memory_order_acquire)) {
        return GlobalStatistics{};
    }
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
    
    // Wait for metrics thread (bounded join; metricsLoop checks running_)
    if (metrics_thread_.joinable()) {
        if (!joinThreadWithin(metrics_thread_)) {
            spdlog::warn("ThreadPoolManager: metrics thread did not exit within deadline; "
                         "a detached watcher will complete the join");
        }
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
