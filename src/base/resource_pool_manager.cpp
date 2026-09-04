/**
 * @file resource_pool_manager.cpp
 * @brief Phase 3 P3-03-A/B: Resource pool manager — implementation.
 * @version 1.0.0
 * @note Status: Block B P3-03-A/B delivery
 */

#include "base/resource_pool_manager.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace themis::resource {

// ============================================================================
// AdaptiveConnectionPool — implementation
// ============================================================================

AdaptiveConnectionPool::AdaptiveConnectionPool()
    : AdaptiveConnectionPool(Config{}) {}

AdaptiveConnectionPool::AdaptiveConnectionPool(const Config& cfg)
    : cfg_(cfg) {
    std::lock_guard<std::mutex> lk(mutex_);
    growLocked(cfg_.min_size);
}

AdaptiveConnectionPool::~AdaptiveConnectionPool() {
    shutdown();
}

// ---------------------------------------------------------------------------
// grow / shrink (caller must hold mutex_)
// ---------------------------------------------------------------------------

void AdaptiveConnectionPool::growLocked(std::size_t count) {
    const std::size_t cap = std::min(pool_size_ + count, cfg_.max_size);
    const std::size_t to_add = cap - pool_size_;
    for (std::size_t i = 0; i < to_add; ++i) {
        available_slots_.push_back(next_id_++);
    }
    pool_size_ = cap;
    if (to_add > 0) {
        ++scale_up_events_;
    }
}

void AdaptiveConnectionPool::shrinkLocked(std::size_t count) {
    const std::size_t floor = std::max(pool_size_, cfg_.min_size) - cfg_.min_size;
    const std::size_t to_remove = std::min(count, std::min(available_slots_.size(), floor));
    for (std::size_t i = 0; i < to_remove; ++i) {
        available_slots_.pop_back();
    }
    pool_size_ -= to_remove;
    if (to_remove > 0) {
        ++scale_down_events_;
    }
}

// ---------------------------------------------------------------------------
// acquire
// ---------------------------------------------------------------------------

bool AdaptiveConnectionPool::acquire(std::chrono::milliseconds timeout, int& slot_id) {
    if (shutdown_.load(std::memory_order_acquire)) {
        throw std::runtime_error("AdaptiveConnectionPool: pool is shut down");
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;

    std::unique_lock<std::mutex> lk(mutex_);

    const auto wait_start = std::chrono::steady_clock::now();
    const bool got = cv_.wait_until(lk, deadline, [this] {
        return !available_slots_.empty() ||
               shutdown_.load(std::memory_order_relaxed);
    });

    if (shutdown_.load(std::memory_order_relaxed)) {
        throw std::runtime_error("AdaptiveConnectionPool: pool is shut down");
    }

    if (!got || available_slots_.empty()) {
        ++total_timeouts_;
        return false;
    }

    // Record acquisition wait time for adaptive scaling.
    const auto wait_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - wait_start).count();
    cumulative_wait_ms_ += wait_ms;
    ++wait_samples_;

    slot_id = available_slots_.back();
    available_slots_.pop_back();
    ++total_acquires_;

    // Update peak utilization.
    const double util = static_cast<double>(pool_size_ - available_slots_.size()) /
                        static_cast<double>(pool_size_);
    if (util > peak_utilization_) {
        peak_utilization_ = util;
    }

    // Adaptive scale-up: if average wait > threshold and pool has room to grow.
    if (wait_samples_ >= 5) {
        const double avg_wait = cumulative_wait_ms_ / static_cast<double>(wait_samples_);
        if (avg_wait > cfg_.scale_up_threshold_ms && pool_size_ < cfg_.max_size) {
            growLocked(cfg_.scale_step);
        }
        idle_periods_       = 0;
        cumulative_wait_ms_ = 0.0;
        wait_samples_       = 0;
    }

    return true;
}

// ---------------------------------------------------------------------------
// release
// ---------------------------------------------------------------------------

void AdaptiveConnectionPool::release([[maybe_unused]] int slot_id) {
    std::lock_guard<std::mutex> lk(mutex_);

    available_slots_.push_back(slot_id);

    // Adaptive scale-down: if utilisation < 20 % for idle_shrink_periods.
    const double util = static_cast<double>(pool_size_ - available_slots_.size()) /
                        static_cast<double>(pool_size_);
    if (util < 0.20) {
        ++idle_periods_;
        if (idle_periods_ >= cfg_.idle_shrink_periods && pool_size_ > cfg_.min_size) {
            shrinkLocked(cfg_.scale_step);
            idle_periods_ = 0;
        }
    } else {
        idle_periods_ = 0;
    }

    cv_.notify_one();
}

// ---------------------------------------------------------------------------
// queries
// ---------------------------------------------------------------------------

std::size_t AdaptiveConnectionPool::size() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return pool_size_;
}

std::size_t AdaptiveConnectionPool::available() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return available_slots_.size();
}

std::size_t AdaptiveConnectionPool::in_use() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return pool_size_ - available_slots_.size();
}

AdaptiveConnectionPool::Statistics
AdaptiveConnectionPool::statistics() const noexcept {
    Statistics st;
    std::lock_guard<std::mutex> lk(mutex_);
    st.pool_size          = pool_size_;
    st.available          = available_slots_.size();
    st.in_use             = pool_size_ - available_slots_.size();
    st.total_acquires     = total_acquires_;
    st.total_timeouts     = total_timeouts_;
    st.scale_up_events    = scale_up_events_;
    st.scale_down_events  = scale_down_events_;
    st.peak_utilization   = peak_utilization_;
    return st;
}

void AdaptiveConnectionPool::shutdown() noexcept {
    shutdown_.store(true, std::memory_order_release);
    cv_.notify_all();
}

void AdaptiveConnectionPool::forceScaleUp() {
    std::lock_guard<std::mutex> lk(mutex_);
    growLocked(cfg_.scale_step);
}

void AdaptiveConnectionPool::forceScaleDown() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (pool_size_ > cfg_.min_size) {
        shrinkLocked(cfg_.scale_step);
    }
}

// ============================================================================
// ResourcePoolManager — implementation
// ============================================================================

ResourcePoolManager::ResourcePoolManager()
    : ResourcePoolManager(Config{}) {}

ResourcePoolManager::ResourcePoolManager(const Config& cfg)
    : cfg_(cfg),
      conn_pool_(std::make_unique<AdaptiveConnectionPool>(cfg.conn_pool)),
      buf_pool_(std::make_unique<BufferPool>(cfg.buffer_pool)) {}

ResourcePoolManager::~ResourcePoolManager() {
    shutdown();
}

ResourcePoolManager::GlobalStatistics
ResourcePoolManager::statistics() const noexcept {
    GlobalStatistics gs;
    gs.conn   = conn_pool_->statistics();
    gs.buffer = buf_pool_->statistics();

    if (gs.conn.pool_size > 0) {
        gs.saturation_conn = static_cast<double>(gs.conn.in_use) /
                             static_cast<double>(gs.conn.pool_size);
    }

    const std::size_t max_buf =
        BufferPool::kSlabSizes.back() > 0 ? gs.buffer.total_allocations : 1;
    gs.saturation_buffer = max_buf > 0
        ? static_cast<double>(gs.buffer.current_live) /
          static_cast<double>(std::max(max_buf, std::size_t{1}))
        : 0.0;

    gs.saturation_alert =
        gs.saturation_conn   > cfg_.saturation_alert_threshold ||
        gs.saturation_buffer > cfg_.saturation_alert_threshold;
    return gs;
}

void ResourcePoolManager::shutdown() noexcept {
    if (shutdown_.exchange(true, std::memory_order_acq_rel)) {
        return;  // Already shut down.
    }
    conn_pool_->shutdown();
    buf_pool_->shutdown();
}

}  // namespace themis::resource
