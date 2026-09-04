/**
 * @file thread_pool_manager.cpp
 * @brief Phase 3 P3-03-C: Work-stealing thread pool — implementation.
 * @version 1.0.0
 * @note Status: Block B P3-03-C delivery
 */

#include "execution/thread_pool_manager.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <numeric>
#include <stdexcept>

namespace themis::resource {

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

WorkStealingThreadPool::WorkStealingThreadPool()
    : WorkStealingThreadPool(Config{}) {}

WorkStealingThreadPool::WorkStealingThreadPool(const Config& cfg)
    : cfg_(cfg) {
    if (cfg_.max_threads == 0) {
        cfg_.max_threads =
            std::max<std::size_t>(4u * std::thread::hardware_concurrency(), 4u);
    }
    cfg_.min_threads = std::max<std::size_t>(cfg_.min_threads, 1u);
    cfg_.min_threads = std::min(cfg_.min_threads, cfg_.max_threads);

    // Pre-create per-thread queues up to max_threads.
    {
        std::lock_guard<std::mutex> lk(queues_mutex_);
        queues_.reserve(cfg_.max_threads);
        for (std::size_t i = 0; i < cfg_.max_threads; ++i) {
            queues_.push_back(std::make_unique<ThreadQueue>());
        }
    }

    // Start min_threads workers.
    {
        std::lock_guard<std::mutex> lk(workers_mutex_);
        workers_.reserve(cfg_.min_threads);
        for (std::size_t i = 0; i < cfg_.min_threads; ++i) {
            queues_[i]->active.store(true, std::memory_order_relaxed);
            workers_.emplace_back([this, i] { workerLoop(i); });
        }
    }
    active_threads_.store(cfg_.min_threads, std::memory_order_relaxed);
}

WorkStealingThreadPool::~WorkStealingThreadPool() {
    shutdown();
}

// ---------------------------------------------------------------------------
// submit
// ---------------------------------------------------------------------------

bool WorkStealingThreadPool::submit(WorkItem item,
                                    std::chrono::milliseconds timeout) {
    if (shutdown_.load(std::memory_order_acquire)) {
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;

    std::unique_lock<std::mutex> lk(dispatch_mutex_);
    const bool ok = capacity_cv_.wait_until(lk, deadline, [this] {
        return queued_count_.load(std::memory_order_relaxed) <
                   cfg_.max_queue_depth ||
               shutdown_.load(std::memory_order_relaxed);
    });

    if (!ok || shutdown_.load(std::memory_order_relaxed)) {
        return false;
    }

    dispatch_queue_.push_back(std::move(item));
    queued_count_.fetch_add(1, std::memory_order_relaxed);
    lk.unlock();
    dispatch_cv_.notify_one();
    return true;
}

bool WorkStealingThreadPool::submit(std::function<void()> fn, std::string name,
                                    std::chrono::milliseconds timeout) {
    return submit(WorkItem{std::move(fn), std::move(name)}, timeout);
}

// ---------------------------------------------------------------------------
// tryGetWork — try dispatch queue first, then steal from peers
// ---------------------------------------------------------------------------

bool WorkStealingThreadPool::tryGetWork(std::size_t /*own_idx*/, WorkItem& out) {
    // Pop from the shared dispatch queue.  Per-thread work deques are
    // pre-allocated but not yet populated by submit(), so the steal path
    // is skipped to avoid spurious lock contention on empty queues.
    {
        std::lock_guard<std::mutex> lk(dispatch_mutex_);
        if (!dispatch_queue_.empty()) {
            out = std::move(dispatch_queue_.front());
            dispatch_queue_.pop_front();
            queued_count_.fetch_sub(1, std::memory_order_relaxed);
            capacity_cv_.notify_one();
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// workerLoop
// ---------------------------------------------------------------------------

void WorkStealingThreadPool::workerLoop(std::size_t thread_idx) {
    const auto idle_timeout =
        std::chrono::milliseconds(cfg_.idle_timeout_ms);

    while (!shutdown_.load(std::memory_order_acquire)) {
        WorkItem work{[] {}};
        bool     got = tryGetWork(thread_idx, work);

        if (!got) {
            // Wait for dispatch_cv_ notification.
            std::unique_lock<std::mutex> lk(dispatch_mutex_);
            dispatch_cv_.wait_for(lk, idle_timeout, [this] {
                return !dispatch_queue_.empty() ||
                       shutdown_.load(std::memory_order_relaxed);
            });
            got = tryGetWork(thread_idx, work);
        }

        if (!got) {
            continue;
        }

        // Execute work item.
        const auto t0 = std::chrono::steady_clock::now();
        try {
            work.fn();
            completed_.fetch_add(1, std::memory_order_relaxed);
        } catch (...) {
            failed_.fetch_add(1, std::memory_order_relaxed);
        }
        const auto elapsed_us = std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - t0).count();

        {
            std::lock_guard<std::mutex> lk(latency_mutex_);
            latency_samples_us_.push_back(elapsed_us);
            // Cap sample buffer at 10 000 entries.
            if (latency_samples_us_.size() > 10000) {
                latency_samples_us_.erase(latency_samples_us_.begin(),
                                          latency_samples_us_.begin() + 1000);
            }
        }
    }

    // Mark queue as inactive.
    {
        std::lock_guard<std::mutex> lk(queues_mutex_);
        if (static_cast<int>(queues_.size()) > thread_idx) {
            queues_[thread_idx]->active.store(false, std::memory_order_relaxed);
        }
    }
    active_threads_.fetch_sub(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// waitAll
// ---------------------------------------------------------------------------

bool WorkStealingThreadPool::waitAll(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lk(dispatch_mutex_);
            if (dispatch_queue_.empty() &&
                queued_count_.load(std::memory_order_relaxed) == 0) {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return queued_count_.load(std::memory_order_relaxed) == 0;
}

// ---------------------------------------------------------------------------
// statistics
// ---------------------------------------------------------------------------

WorkStealingThreadPool::Statistics
WorkStealingThreadPool::statistics() const noexcept {
    Statistics st;
    st.active_threads = active_threads_.load(std::memory_order_relaxed);
    st.queued_items   = queued_count_.load(std::memory_order_relaxed);
    st.completed      = completed_.load(std::memory_order_relaxed);
    st.failed         = failed_.load(std::memory_order_relaxed);

    std::lock_guard<std::mutex> lk(latency_mutex_);
    if (!latency_samples_us_.empty()) {
        auto sorted = latency_samples_us_;
        std::sort(sorted.begin(), sorted.end());
        const std::size_t n = sorted.size();
        st.p50_latency_us = sorted[n / 2];
        st.p99_latency_us = sorted[static_cast<std::size_t>(n * 0.99)];
    }
    return st;
}

// ---------------------------------------------------------------------------
// thread_count
// ---------------------------------------------------------------------------

std::size_t WorkStealingThreadPool::thread_count() const noexcept {
    return active_threads_.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------

void WorkStealingThreadPool::shutdown(std::chrono::milliseconds drain_timeout) {
    if (shutdown_.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    // Drain in-flight work.
    waitAll(drain_timeout);

    // Wake all sleeping workers.
    dispatch_cv_.notify_all();
    capacity_cv_.notify_all();

    // Join all worker threads.
    std::lock_guard<std::mutex> lk(workers_mutex_);
    for (auto& t : workers_) {
        if (t.joinable()) {
            t.join();
        }
    }
    workers_.clear();
}

}  // namespace themis::resource
