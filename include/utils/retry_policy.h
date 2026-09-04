/**
 * @file retry_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <thread>

namespace themis {
namespace utils {

// ============================================================================
// RetryConfig — parameters for exponential backoff
// ============================================================================

/**
 * @brief Configuration for `retry_with_backoff()`.
 */
struct RetryConfig {
    /// Maximum total number of attempts (1 = no retry, just call once).
    uint32_t max_attempts       = 3;

    /// Delay before the first retry in milliseconds.
    uint32_t initial_backoff_ms = 100;

    /// Upper bound on the computed backoff delay in milliseconds.
    uint32_t max_backoff_ms     = 30'000;

    /// Each successive delay is multiplied by this factor (≥ 1.0).
    double   multiplier         = 2.0;

    /// Fraction of the computed delay added as uniform random jitter [0, 1].
    /// 0.0 = no jitter; 0.1 = up to 10 % random added delay.
    double   jitter_fraction    = 0.1;
};

// ============================================================================
// ExponentialBackoff — manual backoff helper
// ============================================================================

/**
 * @brief Stateful exponential-backoff sleep helper.
 *
 * Useful when the caller manages the retry loop explicitly (e.g. to check
 * a cancellation token between attempts).
 *
 * ```cpp
 * ExponentialBackoff bo(cfg);
 * for (int i = 0; i < cfg.max_attempts; ++i) {
 *     if (try_once()) return;
 *     if (!bo.wait()) break; // max backoff iterations reached
 * }
 * ```
 */
class ExponentialBackoff {
public:
    explicit ExponentialBackoff(const RetryConfig& cfg)
        : cfg_(cfg)
        , current_ms_(cfg.initial_backoff_ms)
        , rng_(std::random_device{}())
    {}

    /**
     * @brief Sleep for the current backoff duration, then advance.
     *
     * @return false after `max_attempts - 1` calls (no more retries remain).
     */
    bool wait() {
        ++attempt_;
        if (attempt_ >= cfg_.max_attempts) {
          return false;
        }

        // Compute jitter: uniform in [0, jitter_fraction * current_ms]
        double jitter = 0.0;
        if (cfg_.jitter_fraction > 0.0) {
            std::uniform_real_distribution<double> dist(0.0, cfg_.jitter_fraction * current_ms_);
            jitter = dist(rng_);
        }

        const auto sleep_ms = static_cast<uint32_t>(
            std::min(static_cast<double>(cfg_.max_backoff_ms),
                     current_ms_ + jitter));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

        // Advance: next = min(current * multiplier, max_backoff_ms)
        current_ms_ = std::min(
            static_cast<double>(cfg_.max_backoff_ms),
            current_ms_ * cfg_.multiplier);

        return true;
    }

    /// Return the current computed delay in milliseconds (before jitter).
    [[nodiscard]] uint32_t current_delay_ms() const noexcept {
        return static_cast<uint32_t>(current_ms_);
    }

    /// Return the number of waits performed so far.
    [[nodiscard]] uint32_t attempts() const noexcept { return attempt_; }

private:
    RetryConfig cfg_;
    double      current_ms_;
    uint32_t    attempt_{0};
    std::mt19937 rng_;
};

// ============================================================================
// retry_with_backoff — primary free-function template
// ============================================================================

/**
 * @brief Call @p fn up to `config.max_attempts` times with exponential backoff.
 *
 * @tparam Fn  Callable with signature `std::optional<T>()`.  Return
 *             `std::nullopt` to signal a retryable failure; return a value to
 *             signal success.  Exceptions propagate immediately.
 * @tparam T   Deduced from the return type of @p fn.
 *
 * @param fn      The operation to attempt.
 * @param config  Retry and backoff parameters.
 * @return The successful result, or `std::nullopt` if all attempts failed.
 */
template<typename Fn>
auto retry_with_backoff(Fn&& fn, const RetryConfig& config = RetryConfig{})
    -> decltype(fn())
{
    ExponentialBackoff backoff(config);

    for (uint32_t attempt = 0; attempt < config.max_attempts; ++attempt) {
        auto result = fn();
        if (result) return result;                    // success
        if (attempt + 1 < config.max_attempts) {
            if (!backoff.wait()) break;               // backoff iterator exhausted
        }
    }
    // All attempts failed — return the empty optional/nullopt.
    using ReturnType = decltype(fn());
    return ReturnType{};
}

} // namespace utils
} // namespace themis
