/**
 * @file ts_auto_buffer_adaptive.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/ts_auto_buffer_adaptive.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

FlushController::FlushController(FlushControllerConfig config)
    : config_(std::move(config))
{
    // Clamp initial batch size to configured bounds
    batch_size_ = std::clamp(config_.initial_batch_size,
                             config_.min_batch_size,
                             config_.max_batch_size);
}

// ─────────────────────────────────────────────────────────────────────────────
// reportFlushLatency
// ─────────────────────────────────────────────────────────────────────────────

void FlushController::reportFlushLatency([[maybe_unused]] double latency_ms) {
    std::lock_guard<std::mutex> lock(mutex_);

    ++sample_count_;

    // EWMA update
    if (sample_count_ == 1) {
        // First sample: seed the EWMA
        ewma_latency_ms_ = latency_ms;
    } else {
        ewma_latency_ms_ = config_.ewma_alpha * latency_ms
                         + (1.0 - config_.ewma_alpha) * ewma_latency_ms_;
    }

    // Only adapt after warmup
    if (sample_count_ >= config_.warmup_samples) {
        updateBatchSize();
    }

    // Backpressure signalling
    bool new_backpressure = (ewma_latency_ms_ > config_.slo_threshold_ms);
    if (new_backpressure && !backpressure_) {
        ++backpressure_events_;
        backpressure_ = true;
        THEMIS_WARN("FlushController: backpressure ON "
                    "(ewma={:.1f}ms > slo={:.1f}ms batch={})",
                    ewma_latency_ms_, config_.slo_threshold_ms, batch_size_);
    } else if (!new_backpressure && backpressure_) {
        backpressure_ = false;
        THEMIS_INFO("FlushController: backpressure OFF "
                    "(ewma={:.1f}ms batch={})",
                    ewma_latency_ms_, batch_size_);
        cv_.notify_all();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// updateBatchSize (mutex_ held by caller)
// ─────────────────────────────────────────────────────────────────────────────

void FlushController::updateBatchSize() {
    double threshold   = config_.slo_threshold_ms;
    double headroom_pt = threshold * config_.headroom_factor;

    size_t old = batch_size_;

    if (ewma_latency_ms_ > threshold) {
        // Shrink
        auto nb = static_cast<size_t>(
            std::floor(static_cast<double>(batch_size_) * config_.shrink_factor));
        batch_size_ = std::max(nb, config_.min_batch_size);
    } else if (ewma_latency_ms_ < headroom_pt) {
        // Grow
        auto nb = static_cast<size_t>(
            std::ceil(static_cast<double>(batch_size_) * config_.grow_factor));
        batch_size_ = std::min(nb, config_.max_batch_size);
    }
    // else: stay the same

    if (batch_size_ != old) {
        THEMIS_INFO("FlushController: batch_size {} → {} (ewma={:.1f}ms)",
                    old, batch_size_, ewma_latency_ms_);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// checkBackpressure
// ─────────────────────────────────────────────────────────────────────────────

bool FlushController::checkBackpressure(size_t                    buffered_points,
                                         std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    current_buffered_ = buffered_points;

    if (!backpressure_) return true;

    // Block until queue drains below low-water mark or timeout
    bool ok = cv_.wait_for(lock, timeout, [this]() {
        return !backpressure_ || current_buffered_ <= config_.low_water_mark;
    });
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// notifyDrained
// ─────────────────────────────────────────────────────────────────────────────

void FlushController::notifyDrained([[maybe_unused]] size_t remaining_points) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_buffered_ = remaining_points;
    if (remaining_points <= config_.low_water_mark) {
        cv_.notify_all();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

size_t FlushController::recommendedBatchSize() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return batch_size_;
}

double FlushController::ewmaLatencyMs() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return ewma_latency_ms_;
}

bool FlushController::isBackpressureActive() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return backpressure_;
}

FlushControllerStats FlushController::stats() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    FlushControllerStats s;
    s.ewma_latency_ms    = ewma_latency_ms_;
    s.current_batch_sz   = batch_size_;
    s.backpressure_events= backpressure_events_;
    s.samples            = sample_count_;
    s.in_backpressure    = backpressure_;
    return s;
}

} // namespace themis
