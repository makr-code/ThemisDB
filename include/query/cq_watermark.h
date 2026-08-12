/**
 * @file cq_watermark.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace themis {
namespace query {

/**
 * @brief Per-query watermark tracker with late-data detection.
 *
 * The watermark advances monotonically to (max_seen_event_ts − late_budget).
 * Events with event_ts < watermark are counted as late.  A correction-delta
 * callback is invoked immediately for late events that still fall within the
 * allowed_lateness_ms budget.
 *
 * Thread safety: all public methods are thread-safe.
 */
class CQWatermark {
public:
    /**
     * @param allowed_lateness_ms  Events older than (watermark −
     *                             allowed_lateness_ms) are silently dropped.
     */
    explicit CQWatermark(int64_t allowed_lateness_ms = 500) noexcept;

    /**
     * @brief Register the arrival of an event with the given timestamp.
     *
     * Advances the watermark if the event is on-time.  Returns whether the
     * event is on-time (true) or late but correctable (false, within budget)
     * or silently dropped (false, beyond budget — also increments
     * lateDropped()).
     *
     * @param event_ts_us  Event timestamp in microseconds since epoch.
     * @return true  → on-time; false → late (within budget or dropped).
     */
    bool observe(int64_t event_ts_us) noexcept;

    /**
     * @brief Advance the watermark to (max_seen_ts − allowed_lateness_ms).
     *
     * Called by the scheduler at each tick boundary.
     */
    void advance() noexcept;

    /** @return Current watermark in microseconds since epoch. */
    [[nodiscard]] int64_t watermarkUs() const noexcept;

    /** @return Max event timestamp seen so far (μs since epoch). */
    [[nodiscard]] int64_t maxSeenUs() const noexcept;

    /** @return Total number of late-but-correctable events seen. */
    [[nodiscard]] uint64_t lateProcessed() const noexcept;

    /** @return Total number of events dropped as beyond the late budget. */
    [[nodiscard]] uint64_t lateDropped() const noexcept;

    /** Reset all state. */
    void reset() noexcept;

private:
    int64_t              allowed_lateness_us_{0};
    std::atomic<int64_t> max_seen_us_{0};
    std::atomic<int64_t> watermark_us_{0};
    std::atomic<uint64_t> late_processed_{0};
    std::atomic<uint64_t> late_dropped_{0};
};

}  // namespace query
}  // namespace themis
