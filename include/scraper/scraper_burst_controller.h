// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file scraper_burst_controller.h
 * @brief Token-bucket burst-rate limiter for the scraper crawl pipeline.
 * @version 1.0.0
 *
 * Header-only.  No additional translation unit is required.
 *
 * ## Design
 *
 * `BurstCrawlController` implements a fail-fast token-bucket algorithm:
 *  - The bucket holds at most `max_tokens` tokens.
 *  - Tokens are added at `refill_rate_per_sec` per second (capped at capacity).
 *  - `tryAcquire()` first tops-up the bucket for elapsed time, then attempts
 *    an atomic decrement.  The call never blocks; it returns false immediately
 *    when the bucket is empty.
 *
 * @see include/scraper/scraper_plugin.h
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <algorithm>

namespace themis {
namespace scraper {

/**
 * @brief Fail-fast token-bucket rate limiter for URL dispatch.
 *
 * Thread-safe: `tryAcquire()` and `burstUtilization()` may be called
 * concurrently from multiple threads.  `reset()` serialises via the
 * same refill mutex.
 */
class BurstCrawlController {
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    /**
     * @brief Construct a fully-charged token bucket.
     *
     * @param max_tokens           Maximum bucket capacity (must be > 0).
     * @param refill_rate_per_sec  Tokens restored per second (must be >= 0).
     */
    explicit BurstCrawlController(uint32_t max_tokens,
                                   double   refill_rate_per_sec) noexcept
        : max_tokens_(max_tokens)
        , refill_rate_per_sec_(refill_rate_per_sec)
        , current_tokens_(max_tokens)
        , last_refill_(std::chrono::steady_clock::now())
    {}

    BurstCrawlController(const BurstCrawlController&)            = delete;
    BurstCrawlController& operator=(const BurstCrawlController&) = delete;

    // -------------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------------

    /**
     * @brief Attempt to consume one token from the bucket.
     *
     * Performs a refill pass for elapsed time, then tries an atomic
     * compare-exchange decrement.  Never blocks.
     *
     * @return true  A token was successfully consumed; the caller may proceed.
     * @return false The bucket is exhausted; the caller should skip/defer.
     */
    bool tryAcquire() noexcept {
        refill();

        // Optimistic CAS loop — avoids a mutex for the hot path.
        uint32_t current = current_tokens_.load(std::memory_order_acquire);
        while (current > 0) {
            if (current_tokens_.compare_exchange_weak(
                    current, current - 1,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                return true;
            }
            // current was updated by compare_exchange_weak on failure; retry.
        }
        return false;
    }

    /**
     * @brief Current fill level as a fraction of capacity.
     *
     * @return Value in [0.0, 1.0]: 0.0 = empty, 1.0 = full.
     */
    [[nodiscard]] double burstUtilization() const noexcept {
        if (max_tokens_ == 0) return 0.0;
        const double ratio = static_cast<double>(
            current_tokens_.load(std::memory_order_relaxed))
            / static_cast<double>(max_tokens_);
        return std::clamp(ratio, 0.0, 1.0);
    }

    /**
     * @brief Restore the bucket to full capacity.
     *
     * Typically called between crawl runs to reset rate-limit state.
     */
    void reset() noexcept {
        std::lock_guard<std::mutex> lk(refill_mu_);
        current_tokens_.store(max_tokens_, std::memory_order_release);
        last_refill_ = std::chrono::steady_clock::now();
    }

private:
    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Top-up the bucket for time elapsed since the last refill.
     *
     * Protected by `refill_mu_` to serialise the elapsed-time calculation and
     * `last_refill_` update.  The actual token store is still atomic so that
     * `tryAcquire()` CAS loops remain correct even if multiple threads race
     * through this path.
     */
    void refill() noexcept {
        if (refill_rate_per_sec_ <= 0.0) return;

        const auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lk(refill_mu_);
        const double elapsed_sec =
            std::chrono::duration<double>(now - last_refill_).count();

        if (elapsed_sec <= 0.0) return;

        const double tokens_to_add = elapsed_sec * refill_rate_per_sec_;
        last_refill_ = now;

        const uint32_t current = current_tokens_.load(std::memory_order_relaxed);
        const uint32_t added   = static_cast<uint32_t>(tokens_to_add);
        // Saturating add capped at max_tokens_.
        const uint32_t updated =
            (added >= max_tokens_ - current)
            ? max_tokens_
            : current + added;

        current_tokens_.store(updated, std::memory_order_release);
    }

    // -------------------------------------------------------------------------
    // Fields
    // -------------------------------------------------------------------------

    /// Bucket capacity — maximum number of tokens.
    uint32_t max_tokens_;

    /// Tokens replenished per second.
    double refill_rate_per_sec_;

    /// Current available tokens.  Decremented atomically by tryAcquire().
    std::atomic<uint32_t> current_tokens_;

    /// Timestamp of the most recent refill pass.
    std::chrono::steady_clock::time_point last_refill_;

    /// Serialises elapsed-time calculation and last_refill_ updates.
    mutable std::mutex refill_mu_;
};

} // namespace scraper
} // namespace themis
