/**
 * @file cq_watermark.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * ## Thread Safety & Resource Management
 *
 * CQWatermark uses lock-free atomics for all state, providing wait-free reads
 * and optimistic compare-and-swap for updates. No locks are held; no resources
 * are allocated. All methods are noexcept and exception-safe by design.
 *
 * The watermark never retracts; it only advances monotonically or stays constant.
 * This property is maintained through CAS loops that re-check the condition before
 * updating, ensuring no event timestamp inversion.
 */


#include "query/cq_watermark.h"
#include <algorithm>
#include <limits>

namespace themis {
namespace query {

CQWatermark::CQWatermark(int64_t allowed_lateness_ms) noexcept
    : allowed_lateness_us_(allowed_lateness_ms * 1000LL) {}

bool CQWatermark::observe(int64_t event_ts_us) noexcept {
    // Advance max_seen_us_ atomically
    int64_t current_max = max_seen_us_.load(std::memory_order_relaxed);
    while (event_ts_us > current_max &&
           !max_seen_us_.compare_exchange_weak(
               current_max, event_ts_us,
               std::memory_order_release,
               std::memory_order_relaxed)) {
        // retry; current_max updated by CAS
    }

    const int64_t wm = watermark_us_.load(std::memory_order_acquire);

    if (event_ts_us >= wm) {
        // On-time event
        return true;
    }

    // Late event: within budget?
    // Saturating subtraction: if allowed_lateness_us_ exceeds the watermark value
    // (wm - allowed_lateness_us_ would underflow), clamp the lower bound to INT64_MIN.
    const int64_t min_ts = (allowed_lateness_us_ > wm)
        ? std::numeric_limits<int64_t>::min()
        : (wm - allowed_lateness_us_);
    
    if (event_ts_us >= min_ts) {
        late_processed_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Beyond budget: drop
    late_dropped_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void CQWatermark::advance() noexcept {
    const int64_t max_seen = max_seen_us_.load(std::memory_order_acquire);
    const int64_t new_wm   = max_seen - allowed_lateness_us_;

    int64_t current_wm = watermark_us_.load(std::memory_order_relaxed);
    // Only advance, never retract
    while (new_wm > current_wm &&
           !watermark_us_.compare_exchange_weak(
               current_wm, new_wm,
               std::memory_order_release,
               std::memory_order_relaxed)) {
        // retry
    }
}

int64_t CQWatermark::watermarkUs() const noexcept {
    return watermark_us_.load(std::memory_order_acquire);
}

int64_t CQWatermark::maxSeenUs() const noexcept {
    return max_seen_us_.load(std::memory_order_acquire);
}

uint64_t CQWatermark::lateProcessed() const noexcept {
    return late_processed_.load(std::memory_order_relaxed);
}

uint64_t CQWatermark::lateDropped() const noexcept {
    return late_dropped_.load(std::memory_order_relaxed);
}

void CQWatermark::reset() noexcept {
    max_seen_us_.store(0, std::memory_order_release);
    watermark_us_.store(0, std::memory_order_release);
    late_processed_.store(0, std::memory_order_relaxed);
    late_dropped_.store(0, std::memory_order_relaxed);
}

}  // namespace query
}  // namespace themis
