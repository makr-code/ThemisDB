/**
 * @file adaptive_flush_controller.cpp
 * @brief Phase 2 hardening: Adaptive flush controller with concurrency safety and fail-safe behavior.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 — Core Implementation Complete
 * 
 * ## Phase 2 Enhancements (2026-08-07)
 * 
 * This implementation provides:
 * - **Concurrency Safety**: std::mutex with std::lock_guard for all shared state access
 * - **Fail-Safe Behavior**: Bounded buffer pressure response with explicit error modes
 * - **Deterministic Flush Coordination**: Watermark + timeout triggers with explicit ordering
 * - **Performance Gates**: Optimized for p99 ≤ 200µs flush latency (GATE-TSRG-04)
 * 
 * ## Key Guarantees
 * 
 * 1. **Monotonic Buffer Management**: Points added in order, flushed maintaining insertion order
 * 2. **Backpressure Bounding**: When at/above watermark, producers block until threshold released
 * 3. **Timeout Reliability**: Periodic flush timer ensures no data held > configured interval
 * 4. **Fail-Safe on Stop**: Remaining points flushed synchronously before thread termination
 * 5. **Statistics Accuracy**: Atomic counters provide lockless stats readout
 * 
 * @see include/timeseries/adaptive_flush_controller.h
 * @see include/timeseries/timeseries_api_contract.h
 * @see src/timeseries/ROADMAP.md — Phase 2 items
 */

#include "timeseries/adaptive_flush_controller.h"
#include "timeseries/timeseries_metrics.h"
#include "utils/logger.h"

#include <algorithm>
#include <stdexcept>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

AdaptiveFlushController::AdaptiveFlushController(TSStore*                        tsstore,
                                                 AdaptiveFlushControllerConfig   config)
    : tsstore_(tsstore)
    , config_(std::move(config))
{
    if (!tsstore_) {
        throw std::invalid_argument("AdaptiveFlushController: tsstore cannot be null");
    }
    if (config_.buffer_capacity == 0) {
        throw std::invalid_argument("AdaptiveFlushController: buffer_capacity must be > 0");
    }
    if (config_.watermark_ratio <= 0.0 || config_.watermark_ratio > 1.0) {
        throw std::invalid_argument(
            "AdaptiveFlushController: watermark_ratio must be in (0, 1]");
    }

    stat_last_flush_ns_.store(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count(),
        std::memory_order_relaxed);
}

AdaptiveFlushController::~AdaptiveFlushController() {
    if (running_.load(std::memory_order_relaxed)) {
        stop();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void AdaptiveFlushController::start() {
    if (running_.exchange(true, std::memory_order_acq_rel)) {
        THEMIS_WARN("AdaptiveFlushController already running");
        return;
    }

    THEMIS_INFO("AdaptiveFlushController starting: capacity={} interval={}ms watermark={:.0f}%",
                config_.buffer_capacity,
                config_.flush_interval.count(),
                config_.watermark_ratio * 100.0);

    if (config_.async_flush) {
        flush_thread_ = std::thread(&AdaptiveFlushController::flushThread, this);
    }
}

void AdaptiveFlushController::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    THEMIS_INFO("AdaptiveFlushController stopping...");

    // Wake flush thread
    flush_cv_.notify_all();

    // Unblock any producers waiting on backpressure
    {
        std::lock_guard<std::mutex> lock(bp_mutex_);
        bp_cv_.notify_all();
    }

    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }

    // Final synchronous flush of any remaining points
    size_t remaining = flushInternal();
    THEMIS_INFO("AdaptiveFlushController stopped; final flush wrote {} points", remaining);
}

// ─────────────────────────────────────────────────────────────────────────────
// Write API
// ─────────────────────────────────────────────────────────────────────────────

const char* AdaptiveFlushController::validatePoint(const TSStore::DataPoint& p) noexcept {
    if (p.metric.empty()) {
      return "metric name cannot be empty";
    }
    if (p.entity.empty()) {
      return "entity ID cannot be empty";
    }
    return nullptr;
}

Result<void> AdaptiveFlushController::add(const TSStore::DataPoint& point) {
    if (const char* err = validatePoint(point)) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST, err);
    }

    // Backpressure: block producer when buffer is at/above watermark.
    if (watermarkReached()) {
        ++stat_backpressure_events_;
        THEMIS_WARN("AdaptiveFlushController backpressure: buffer={}/{} ({}%), blocking producer",
                    buffer_size_.load(std::memory_order_relaxed),
                    config_.buffer_capacity,
                    static_cast<int>(config_.watermark_ratio * 100));

        if (config_.metrics) {
            config_.metrics->recordBackpressure(point.metric);
        }

        // Wake flush thread immediately to drain the buffer
        flush_cv_.notify_one();

        // Block until buffer drains below watermark or controller stops
        std::unique_lock<std::mutex> bp_lock(bp_mutex_);
        bp_cv_.wait(bp_lock, [this]() noexcept {
            return !running_.load(std::memory_order_relaxed) || !watermarkReached();
        });

        if (!running_.load(std::memory_order_relaxed)) {
            return ErrVoid(errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED,
                           "AdaptiveFlushController stopped while waiting for backpressure relief");
        }
    }

    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (!has_oldest_point_) {
            oldest_point_time_ = std::chrono::steady_clock::now();
            has_oldest_point_  = true;
        }
        buffer_.push_back(point);
    }

    size_t new_size = ++buffer_size_;
    ++stat_points_buffered_;

    // Update backpressure flag
    backpressure_.store(new_size >= watermarkThreshold(), std::memory_order_relaxed);

    // Trigger immediate flush when watermark is reached
    if (new_size >= watermarkThreshold()) {
        flush_cv_.notify_one();
    }

    return OkVoid();
}

Result<size_t> AdaptiveFlushController::addBatch(
    const std::vector<TSStore::DataPoint>& points)
{
    if (points.empty()) {
        return Ok(size_t{0});
    }

    // Validate all points before acquiring the lock
    for (const auto& p : points) {
        if (const char* err = validatePoint(p)) {
            return Err<size_t>(errors::ErrorCode::ERR_API_INVALID_REQUEST, err);
        }
    }

    // Backpressure check for the batch as a whole
    if (watermarkReached()) {
        ++stat_backpressure_events_;
        THEMIS_WARN("AdaptiveFlushController backpressure (batch of {}): buffer={}/{}",
                    points.size(),
                    buffer_size_.load(std::memory_order_relaxed),
                    config_.buffer_capacity);

        if (config_.metrics) {
            config_.metrics->recordBackpressure(points.front().metric);
        }

        flush_cv_.notify_one();

        std::unique_lock<std::mutex> bp_lock(bp_mutex_);
        bp_cv_.wait(bp_lock, [this]() noexcept {
            return !running_.load(std::memory_order_relaxed) || !watermarkReached();
        });

        if (!running_.load(std::memory_order_relaxed)) {
            return Err<size_t>(errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED,
                               "AdaptiveFlushController stopped during backpressure wait");
        }
    }

    size_t accepted = 0;
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (!has_oldest_point_) {
            oldest_point_time_ = std::chrono::steady_clock::now();
            has_oldest_point_  = true;
        }
        for (const auto& p : points) {
            buffer_.push_back(p);
            ++accepted;
        }
    }

    size_t new_size = (buffer_size_ += accepted);
    stat_points_buffered_ += accepted;

    backpressure_.store(new_size >= watermarkThreshold(), std::memory_order_relaxed);

    if (new_size >= watermarkThreshold()) {
        flush_cv_.notify_one();
    }

    return Ok(accepted);
}

// ─────────────────────────────────────────────────────────────────────────────
// Flush
// ─────────────────────────────────────────────────────────────────────────────

size_t AdaptiveFlushController::flush() {
    return flushInternal();
}

size_t AdaptiveFlushController::flushInternal() {
    // Drain buffer in flush_batch_size chunks
    size_t total_flushed = 0;

    while (true) {
        std::vector<TSStore::DataPoint> batch;
        batch.reserve(config_.flush_batch_size);

        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            size_t take = std::min(config_.flush_batch_size, buffer_.size());
            if (take == 0) {
              break;
            }

            for (size_t i = 0; i < take; ++i) {
                batch.push_back(std::move(buffer_.front()));
                buffer_.pop_front();
            }

            // Reset oldest-point tracking
            if (buffer_.empty()) {
                has_oldest_point_ = false;
            } else {
                oldest_point_time_ = std::chrono::steady_clock::now();
            }
        }

        if (batch.empty()) {
          break;
        }

        size_t batch_written = 0;
        for (auto& p : batch) {
            auto res = tsstore_->putDataPoint(p);
            if (res) {
                ++batch_written;
            } else {
                THEMIS_ERROR("AdaptiveFlushController: write failed for {}/{}: {}",
                             p.metric, p.entity, res.error().message());
            }
        }

        buffer_size_         -= batch.size();
        total_flushed        += batch_written;
        stat_points_flushed_ += batch_written;
    }

    if (total_flushed > 0) {
        ++stat_flush_count_;
        auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        stat_last_flush_ns_.store(now_ns, std::memory_order_relaxed);

        // Release any backpressure waiters
        backpressure_.store(false, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> bp_lock(bp_mutex_);
            bp_cv_.notify_all();
        }
    }

    return total_flushed;
}

// ─────────────────────────────────────────────────────────────────────────────
// Background flush thread
// ─────────────────────────────────────────────────────────────────────────────

void AdaptiveFlushController::flushThread() {
    THEMIS_INFO("AdaptiveFlushController flush thread started");

    while (running_.load(std::memory_order_relaxed)) {
        // Wait for flush_interval or an explicit wakeup (watermark / stop)
        {
            std::unique_lock<std::mutex> lock(flush_mutex_);
            flush_cv_.wait_for(lock, config_.flush_interval, [this]() noexcept {
                return !running_.load(std::memory_order_relaxed) ||
                       watermarkReached();
            });
        }

        if (!running_.load(std::memory_order_relaxed)) {
            break;
        }

        // Check for overdue flush before flushing
        if (isOverdue()) {
            ++stat_overdue_flush_events_;
            THEMIS_WARN("AdaptiveFlushController overdue flush: data held >{}× interval ({}ms)",
                        config_.overdue_flush_multiplier,
                        config_.flush_interval.count() * config_.overdue_flush_multiplier);
            if (config_.metrics) {
                auto held_ms = static_cast<double>(
                    config_.flush_interval.count() * config_.overdue_flush_multiplier);
                config_.metrics->recordOverdueFlush("", held_ms);
            }
        }

        bool by_watermark = watermarkReached();
        size_t flushed    = flushInternal();

        if (flushed > 0) {
            if (by_watermark) {
                ++stat_watermark_triggered_;
            } else {
                ++stat_timeout_triggered_;
            }

            THEMIS_DEBUG("AdaptiveFlushController flushed {} points (trigger={})",
                         flushed, by_watermark ? "watermark" : "timeout");
        }
    }

    THEMIS_INFO("AdaptiveFlushController flush thread stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Query helpers
// ─────────────────────────────────────────────────────────────────────────────

size_t AdaptiveFlushController::watermarkThreshold() const noexcept {
    return static_cast<size_t>(
        static_cast<double>(config_.buffer_capacity) * config_.watermark_ratio);
}

bool AdaptiveFlushController::watermarkReached() const noexcept {
    return buffer_size_.load(std::memory_order_relaxed) >= watermarkThreshold();
}

bool AdaptiveFlushController::isOverdue() const {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (!has_oldest_point_) {
      return false;
    }

    auto overdue_threshold = config_.flush_interval *
                             static_cast<int64_t>(config_.overdue_flush_multiplier);
    auto age = std::chrono::steady_clock::now() - oldest_point_time_;
    return age >= overdue_threshold;
}

AdaptiveFlushControllerStats AdaptiveFlushController::getStats() const {
    AdaptiveFlushControllerStats s;
    s.points_buffered             = stat_points_buffered_.load(std::memory_order_relaxed);
    s.points_flushed              = stat_points_flushed_.load(std::memory_order_relaxed);
    s.flush_count                 = stat_flush_count_.load(std::memory_order_relaxed);
    s.watermark_triggered_flushes = stat_watermark_triggered_.load(std::memory_order_relaxed);
    s.timeout_triggered_flushes   = stat_timeout_triggered_.load(std::memory_order_relaxed);
    s.backpressure_events         = stat_backpressure_events_.load(std::memory_order_relaxed);
    s.overdue_flush_events        = stat_overdue_flush_events_.load(std::memory_order_relaxed);

    s.current_buffer_size  = buffer_size_.load(std::memory_order_relaxed);
    s.buffer_utilization   = (config_.buffer_capacity > 0)
        ? static_cast<double>(s.current_buffer_size) /
          static_cast<double>(config_.buffer_capacity)
        : 0.0;

    auto ns = stat_last_flush_ns_.load(std::memory_order_relaxed);
    s.last_flush_time =
        std::chrono::steady_clock::time_point(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::nanoseconds(ns)));

    return s;
}

bool AdaptiveFlushController::isBackpressured() const noexcept {
    return backpressure_.load(std::memory_order_relaxed);
}

} // namespace themis

