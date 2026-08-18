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
    // Resource limits (Phase 2.6 cross-cutting hardening)
    static constexpr double MAX_RATE_PER_SECOND = 1'000'000.0;  // 1M tokens/sec
    static constexpr double MAX_BURST_SIZE = 100'000'000.0;  // 100M tokens max
    static constexpr double MIN_RATE_PER_SECOND = 0.001;  // 1 token per 1000 seconds min
    
    /**
     * @param rate_per_second  Steady-state token refill rate (tokens/s).
     * @param burst_size       Maximum token capacity (controls burst allowance).
     *
     * @note Resource Limits (Phase 2.6): rate_per_second clamped to [MIN_RATE, MAX_RATE];
     *       burst_size clamped to [rate_per_second, MAX_BURST_SIZE].
     */
    RateLimiter(double rate_per_second, double burst_size);

    /**
     * @brief Attempt to consume @p tokens without blocking.
     *
     * Non-blocking operation that checks if sufficient tokens are available
     * in the bucket. If available, tokens are consumed and true is returned.
     * Otherwise, state is unchanged and false is returned.
     *
     * @param tokens Number of tokens to acquire (default 1.0)
     * @return true if tokens were available and consumed; false otherwise
     *
    * @error_contract
    * | Condition | ErrorCode | Severity | Logging | Recovery |
    * |-----------|-----------|----------|---------|----------|
    * | tokens <= 0 (no-op) | n/a | n/a | None | Return true |
    * | Token bucket exhausted (insufficient tokens) | RATELIMIT_EXCEEDED (9073) | Warning | requested, available, rate_per_s | Return false (RATE_LIMIT_EXHAUSTED) |
    *
    * @degradation explicit false return + structured diagnostic on every rejection
    * @bounded_resources
    * - Tokens capped at burst_size (no unbounded accumulation)
    * - Operation O(1) time complexity
    * 
    * @thread_safety Thread-safe via internal mutex (no spin-loops)
    * @performance Sub-microsecond; no sleep/wait
    * 
    * @see ErrorCode 9070-9079 for concurrency error taxonomy
    * @see acquire() for blocking variant with automatic retry
    */
    bool try_acquire(double tokens = 1.0);

    /**
     * @brief Try to acquire tokens, waiting at most @p timeout.
     *
     * @param tokens  Number of tokens to acquire (default 1.0)
     * @param timeout Maximum time to wait
     * @return true if tokens were acquired; false on timeout
     *
     * @error_contract
     * - Returns true immediately if tokens <= 0 (no-op)
     * - Returns false if timeout expires before tokens become available
     * - Thread-safe via internal mutex
     */
    bool try_acquire_for(double tokens,
                         std::chrono::milliseconds timeout) noexcept;

    /**
     * @brief Block until @p tokens are available and consume them with timeout support.
     * 
     * Graceful degradation:
     * - If tokens > burst_size: returns false immediately (tokens can never accumulate)
     * - If timeout expires: returns false and propagates error to caller
     * - Caller should implement fallback strategy (queue rejection or degraded mode)
     * 
     * @param tokens Number of tokens to acquire (must be <= burst_size)
     * @param timeout Maximum time to wait for tokens to become available
     * @return true if tokens acquired successfully; false on timeout or invalid request
     * 
     * @error_contract
     * - If tokens <= 0: returns true immediately (no-op); no logging emitted
     * - If tokens > burst_size: returns false (error code: 9073 RATELIMIT_EXCEEDED)
     * - If timeout exceeded: returns false (error code: 9073 RATELIMIT_EXCEEDED)
     * - On successful acquisition: returns true
     * 
     * @bounded_resources
     * - Maximum wait time: timeout parameter (caller-controlled)
     * - Memory: stack-only; no dynamic allocation
     * - CPU: minimal while waiting (uses condition_variable)
     * 
     * @thread_safety Thread-safe; condition variable handles concurrent waiters
     * @performance O(wait_time); blocks thread until available or timeout
     * 
     * @graceful_degradation
     * When rate limit is exceeded:
     * 1. Caller receives false return value
     * 2. Should log incident (rate limit threshold breached)
     * 3. Can choose to: queue for later, reject request, or use degraded service tier
     * 
     * @note Always use timeouts in production to prevent unbounded blocking
     * @note Ensure tokens <= burst_size to avoid timeout guarantee violation
     * @see try_acquire() for non-blocking variant (fail-fast)
     * @see set_rate() to adjust rate dynamically
     */
    bool acquire_with_timeout(double tokens, std::chrono::milliseconds timeout);

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
