// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/thread_pool_manager.h"

namespace themisdb {
namespace sharding {

ThreadPoolManager::ThreadPoolManager(const Config& config)
    : config_(config) {
    // Create core worker threads
    for (size_t i = 0; i < config_.core_threads; ++i) {
        worker_threads_.emplace_back(&ThreadPoolManager::workerThread, this);
    }
}

ThreadPoolManager::~ThreadPoolManager() {
    shutdown(false);
}

bool ThreadPoolManager::submit(Task task, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (task_queue_.size() >= config_.queue_size) {
        if (shutdown_) {
            ++rejected_tasks_;
            return false;
        }
        
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            ++rejected_tasks_;
            return false;
        }
    }
    
    if (shutdown_) {
        ++rejected_tasks_;
        return false;
    }
    
    task_queue_.push(std::move(task));
    cv_.notify_one();
    return true;
}

void ThreadPoolManager::shutdown(bool wait_for_tasks) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    
    cv_.notify_all();
    
    // Wait for all worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    if (!wait_for_tasks) {
        // Clear remaining tasks
        std::lock_guard<std::mutex> lock(mutex_);
        while (!task_queue_.empty()) {
            task_queue_.pop();
            ++rejected_tasks_;
        }
    }
}

ThreadPoolManager::Stats ThreadPoolManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return Stats{
        config_.core_threads,
        active_threads_,
        task_queue_.size(),
        completed_tasks_,
        rejected_tasks_
    };
}

void ThreadPoolManager::workerThread() {
    while (true) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            cv_.wait(lock, [this] {
                return shutdown_ || !task_queue_.empty();
            });
            
            if (shutdown_ && task_queue_.empty()) {
                return;
            }
            
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            ++active_threads_;
            try {
                task();
                ++completed_tasks_;
            } catch (...) {
                // Swallow exceptions to prevent thread termination
            }
            --active_threads_;
            
            // Notify that queue space is available
            cv_.notify_one();
        }
    }
}

} // namespace sharding
} // namespace themisdb
