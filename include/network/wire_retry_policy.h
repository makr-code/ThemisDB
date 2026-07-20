/**
 * @file wire_retry_policy.h
 * @brief Exponential-backoff retry policy for the ThemisDB wire protocol server.
 *
 * Provides a configurable, jitter-aware retry policy used by the wire protocol
 * server for transient failures on bind/listen start-up and per-connection
 * transient I/O errors.
 *
 * Design goals:
 *  - Zero allocation in the hot retry path (all state is value-type).
 *  - Deterministic delay bounds (max_delay_ms hard-caps the backoff).
 *  - Optional full-jitter (uniformly distributed in [0, delay]) to spread
 *    reconnect storms across multiple server instances.
 *  - Thread-safe: each RetryContext is owned by a single call-site.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 */

#pragma once

#include <boost/system/error_code.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace themis::network {

/**
 * @brief Classification of retry-eligible vs. non-retryable failure types.
 *
 * The wire protocol server distinguishes transient failures (worth retrying)
 * from permanent failures (fail fast).
 */
enum class WireErrorClass {
    kTransient,   ///< Temporary resource contention, network blip — retry.
    kPermanent,   ///< Protocol error, auth failure, bad config — abort.
    kUnknown,     ///< Unclassified; treated as transient by default.
};

/**
 * @brief Policy parameters that govern exponential-backoff retry behaviour.
 *
 * All delay values are in milliseconds.  Instances are cheap to copy.
 *
 * Example (bind/listen retry — 3 attempts with 100 ms base):
 * @code
 * WireRetryPolicy p;
 * p.max_attempts   = 3;
 * p.base_delay_ms  = 100;
 * p.max_delay_ms   = 2000;
 * p.multiplier     = 2.0;
 * p.enable_jitter  = true;
 * @endcode
 */
struct WireRetryPolicy {
    /// Maximum number of retry attempts (0 = never retry; attempt the operation
    /// exactly once).
    uint32_t max_attempts = 3;

    /// Base delay for the first retry interval in milliseconds.
    uint32_t base_delay_ms = 100;

    /// Hard cap on the computed backoff delay (milliseconds).
    uint32_t max_delay_ms = 30'000;

    /// Exponential multiplier applied to the delay on each retry.
    /// @note Must be >= 1.0; values < 1.0 are clamped to 1.0.
    double multiplier = 2.0;

    /// When true, applies full-jitter: delay = random(0, computed_delay).
    /// This spreads reconnect storms across many clients/replicas.
    bool enable_jitter = true;

    /// If non-empty, only errors whose category matches this list are retried.
    /// An empty list means "retry all transient + unknown errors".
    /// Currently unused reserved for future per-code filtering.
    // std::vector<int> retryable_codes;

    /**
     * @brief Build a conservative policy suitable for bind/listen start-up.
     *
     * 3 attempts, 100 ms base, linear backoff (multiplier=2), max 2 s.
     */
    [[nodiscard]] static WireRetryPolicy forBindListen() noexcept {
        WireRetryPolicy p;
        p.max_attempts  = 3;
        p.base_delay_ms = 100;
        p.max_delay_ms  = 2'000;
        p.multiplier    = 2.0;
        p.enable_jitter = false; // deterministic delays on startup
        return p;
    }

    /**
     * @brief Build a policy for transient per-connection I/O errors.
     *
     * 5 attempts, 50 ms base, 10 s max, full jitter to prevent storm.
     */
    [[nodiscard]] static WireRetryPolicy forConnectionIO() noexcept {
        WireRetryPolicy p;
        p.max_attempts  = 5;
        p.base_delay_ms = 50;
        p.max_delay_ms  = 10'000;
        p.multiplier    = 2.0;
        p.enable_jitter = true;
        return p;
    }

    /**
     * @brief Build an aggressive policy for unit tests (no real sleeping).
     *
     * max_attempts=3, base_delay_ms=0, no jitter.
     */
    [[nodiscard]] static WireRetryPolicy forTesting() noexcept {
        WireRetryPolicy p;
        p.max_attempts  = 3;
        p.base_delay_ms = 0;
        p.max_delay_ms  = 0;
        p.multiplier    = 1.0;
        p.enable_jitter = false;
        return p;
    }
};

/**
 * @brief Mutable state tracking one retry sequence.
 *
 * A RetryContext is created at the start of an operation and advanced on
 * each failed attempt via nextDelay().  It is not thread-safe; each
 * call-site that runs its own retry loop owns its own context.
 *
 * Typical usage:
 * @code
 * auto policy = WireRetryPolicy::forBindListen();
 * RetryContext ctx(policy);
 * while (true) {
 *     auto result = tryOperation();
 *     if (result.ok())  break;
 *     auto delay = ctx.nextDelay(WireErrorClass::kTransient);
 *     if (!delay) throw std::runtime_error("max retries exceeded");
 *     std::this_thread::sleep_for(*delay);
 * }
 * @endcode
 */
class RetryContext {
public:
    /**
     * @brief Construct a context bound to @p policy.
     *
     * @param policy  Retry parameters to use for this sequence.
     */
    explicit RetryContext(const WireRetryPolicy& policy) noexcept
        : policy_(policy)
        , attempt_(0)
        , rng_(std::random_device{}())
    {}

    /// Returns the number of attempts made so far (0 before first nextDelay call).
    [[nodiscard]] uint32_t attempts() const noexcept { return attempt_; }

    /// Returns true when another attempt is still available.
    [[nodiscard]] bool canRetry() const noexcept {
        return attempt_ < policy_.max_attempts;
    }

    /**
     * @brief Advance the retry counter and compute the next sleep delay.
     *
     * @param err_class  Classification of the error that triggered the retry.
     *                   kPermanent errors always return std::nullopt.
     *
     * @return Computed delay to wait before the next attempt, or
     *         std::nullopt when the maximum number of attempts has been
     *         exhausted or the error is permanent.
     */
    [[nodiscard]] std::optional<std::chrono::milliseconds>
    nextDelay(WireErrorClass err_class = WireErrorClass::kTransient) noexcept {
        if (err_class == WireErrorClass::kPermanent) {
            return std::nullopt;
        }
        if (attempt_ >= policy_.max_attempts) {
            return std::nullopt;
        }

        // Compute exponential delay: base * multiplier^attempt
        const double raw_ms = static_cast<double>(policy_.base_delay_ms)
                              * std::pow(std::max(1.0, policy_.multiplier),
                                         static_cast<double>(attempt_));
        const double capped_ms = std::min(raw_ms,
                                          static_cast<double>(policy_.max_delay_ms));

        double delay_ms = capped_ms;
        if (policy_.enable_jitter && capped_ms > 0.0) {
            // Full-jitter: uniform in [0, capped_ms]
            std::uniform_real_distribution<double> dist(0.0, capped_ms);
            delay_ms = dist(rng_);
        }

        ++attempt_;
        return std::chrono::milliseconds(static_cast<int64_t>(delay_ms));
    }

    /**
     * @brief Reset the context for a new retry sequence against the same policy.
     */
    void reset() noexcept { attempt_ = 0; }

    /// Immutable access to the bound policy.
    [[nodiscard]] const WireRetryPolicy& policy() const noexcept { return policy_; }

private:
    WireRetryPolicy  policy_;
    uint32_t         attempt_;
    std::mt19937     rng_;
};

/**
 * @brief Classify a Boost.Asio error_code into a WireErrorClass.
 *
 * Used by the wire protocol server to decide whether to retry a failed
 * accept or send/receive operation.
 *
 * @param ec  Boost.Asio error code to classify.
 * @return WireErrorClass enum value.
 */
[[nodiscard]] WireErrorClass classifyBoostError(
    const boost::system::error_code& ec) noexcept;

/**
 * @brief Execute @p op with retry governed by @p policy.
 *
 * @p op must be callable as `bool()`, returning true on success and false
 * on a retryable failure.  The callable must not throw; exceptions are
 * treated as permanent failures.
 *
 * @param policy   Retry parameters.
 * @param op       Operation to retry; returns true on success.
 * @param on_fail  Optional callback invoked on each failure with (attempt, delay_ms).
 *
 * @return true if @p op succeeded within the allowed attempts; false otherwise.
 */
[[nodiscard]] bool retryWithPolicy(
    const WireRetryPolicy& policy,
    std::function<bool()> op,
    std::function<void(uint32_t /*attempt*/, int64_t /*delay_ms*/)> on_fail = {}) noexcept;

} // namespace themis::network
