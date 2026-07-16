/**
 * @file rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: rate_limiter.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
