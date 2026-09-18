/**
 * @file cq_watermark.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class CQWatermark {
public:
    explicit CQWatermark(int64_t allowed_lateness_ms = 500) noexcept;

    /**
     * @brief Observe.
     * @param[in] event_ts_us Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool observe(int64_t event_ts_us) noexcept;

    /**
     * @brief Advance an iterator within the validated range.
     * @note Exception safety: noexcept.
     */
    void advance() noexcept;

    [[nodiscard]] int64_t watermarkUs() const noexcept;

    [[nodiscard]] int64_t maxSeenUs() const noexcept;

    [[nodiscard]] uint64_t lateProcessed() const noexcept;

    [[nodiscard]] uint64_t lateDropped() const noexcept;

    /**
     * @brief Reset the modification detection flag.
     * @note Exception safety: noexcept.
     */
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
