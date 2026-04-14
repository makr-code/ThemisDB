/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter.h                                     ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:30:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     100                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace themis {
namespace utils {

/**
 * @brief Token-bucket rate limiter for shared use across modules.
 *
 * Tokens refill continuously at `rate_per_second` up to `burst_size`.
 * Callers may either poll non-blocking (`try_acquire`) or block until
 * the requested number of tokens is available (`acquire`).
 *
 * Thread-safety: all public methods are fully thread-safe.
 */
class RateLimiter {
public:
    /**
     * @param rate_per_second  Steady-state token refill rate (tokens/s).
     * @param burst_size       Maximum token capacity (controls burst allowance).
     */
    RateLimiter(double rate_per_second, double burst_size);

    /**
     * @brief Attempt to consume @p tokens without blocking.
     * @return true  Tokens were available and consumed.
     * @return false Insufficient tokens; state is unchanged.
     */
    bool try_acquire(double tokens = 1.0);

    /**
     * @brief Block until @p tokens are available and consume them.
     *
     * If `tokens > burst_size` this will spin with minimal sleep intervals
     * (tokens can never accumulate past burst_size) so callers must ensure
     * tokens ≤ burst_size for meaningful blocking behaviour.
     */
    void acquire(double tokens = 1.0);

    /**
     * @brief Reset the bucket to `burst_size` and update the timestamp.
     */
    void reset();

    /**
     * @brief Return the current number of available tokens (snapshot).
     */
    double available() const;

    /**
     * @brief Update the refill rate.  Takes effect immediately on the
     *        next refill calculation without resetting the bucket level.
     */
    void set_rate(double rate_per_second);

private:
    /// Refill tokens based on elapsed time (must be called under lock).
    void refill_locked();

    /// Compute wait duration needed for `tokens` to become available.
    std::chrono::duration<double> wait_for_locked(double tokens) const;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    double rate_;       ///< tokens per second
    double burst_;      ///< max token capacity
    double tokens_;     ///< current token count

    using Clock = std::chrono::steady_clock;
    Clock::time_point last_refill_;
};

} // namespace utils
} // namespace themis
