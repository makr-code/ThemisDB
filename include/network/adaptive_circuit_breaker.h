/**
 * @file adaptive_circuit_breaker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
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
#include <unordered_map>

namespace themis {
namespace network {

/**
 * @brief Circuit breaker states for the network layer.
 */
enum class CircuitState {
    CLOSED,    ///< Normal operation — requests flow through
    HALF_OPEN, ///< Recovery probe — limited requests allowed
    OPEN       ///< Circuit tripped — fail fast, requests rejected
};

/**
 * @brief Adaptive Circuit Breaker for network failure resilience.
 *
 * Implements the CLOSED → OPEN → HALF_OPEN → CLOSED state machine with
 * an adaptive failure threshold that can self-adjust based on the observed
 * error rate.
 *
 * Features:
 * - Thread-safe via lock-free atomics on the hot read path
 * - Configurable failure/success thresholds and open timeout
 * - Adaptive threshold adjustment: raises/lowers threshold by
 *   `adaptive_factor` percent on sustained success/failure streaks
 * - Statistics tracking: total/successful/failed/rejected calls
 * - Optional state-change callback for observability integration
 *
 * Usage example:
 * @code
 * AdaptiveCircuitBreaker::Config cfg;
 * cfg.failure_threshold = 5;
 * cfg.open_timeout      = std::chrono::seconds(30);
 * AdaptiveCircuitBreaker cb(cfg);
 *
 * if (cb.shouldAllow()) {
 *     try {
 *         doNetworkCall();
 *         cb.recordSuccess();
 *     } catch (...) {
 *         cb.recordFailure();
 *     }
 * } else {
 *     // fast-path fallback
 * }
 * @endcode
 */
class AdaptiveCircuitBreaker {
public:
        /**
         * @brief Error class identifier for per-class threshold configuration.
         *
         * Use string-typed error class labels (e.g., "timeout", "connection",
         * "auth") to apply independent failure thresholds per error category.
         * The empty string is treated as the global/default class.
         */
        using ErrorClass = std::string;

        /**
         * @brief Per-error-class threshold override.
         *
         * When an error class has an entry here its failures are counted
         * separately against @c threshold.  A class whose consecutive failure
         * count reaches its threshold immediately trips the circuit even if the
         * global counter has not yet reached @c failure_threshold.
         */
        struct ErrorClassConfig {
            /// Consecutive failures of this class that trip the circuit.
            size_t threshold = 5;

            /// Fraction of the per-class threshold used for adaptive reduction.
            /// Inherits the parent Config::adaptive_factor when 0.
            double adaptive_factor = 0.0;
        };

        /**
         * @brief Construction-time configuration.
         */
        struct Config {
            /// Consecutive failures in CLOSED state that trip the circuit.
            size_t failure_threshold = 10;

            /// Consecutive successes in HALF_OPEN state that close the circuit.
            size_t success_threshold = 5;

            /// How long to keep the circuit OPEN before entering HALF_OPEN.
            std::chrono::seconds open_timeout{60};

            /// Maximum time in HALF_OPEN before falling back to OPEN.
            std::chrono::seconds half_open_timeout{30};

            /// Enable dynamic adjustment of @c failure_threshold at runtime.
            bool enable_adaptive_threshold = true;

            /**
             * @brief Fraction by which the threshold is adjusted on adaptation.
             *
             * When the circuit trips repeatedly (indicating the threshold is too
             * generous), @c failure_threshold is decreased by this fraction.
             * When the circuit stays closed for a long time it is increased.
             * Range: (0, 1).  Default: 0.10 (10 %).
             */
            double adaptive_factor = 0.1;

            /**
             * @brief Per-error-class threshold overrides.
             *
             * Errors reported via @c recordFailure(error_class) that match a key
             * here are tracked separately.  If any class reaches its own
             * threshold, the circuit trips independently of the global counter.
             *
             * Example:
             * @code
             * cfg.error_class_configs["timeout"]    = {3};
             * cfg.error_class_configs["connection"] = {5};
             * @endcode
             */
            std::unordered_map<ErrorClass, ErrorClassConfig> error_class_configs;
        };

    /**
     * @brief Snapshot of circuit breaker statistics.
     */
    struct Stats {
        CircuitState state = CircuitState::CLOSED;
        uint64_t total_calls      = 0;
        uint64_t successful_calls = 0;
        uint64_t failed_calls     = 0;
        uint64_t rejected_calls   = 0; ///< Requests fast-failed when OPEN
        size_t   current_failure_threshold = 0;
        std::chrono::steady_clock::time_point last_state_change;

        /// Per-error-class failure counters (consecutive, since last reset/close).
        std::unordered_map<ErrorClass, uint64_t> error_class_failures;
    };

    /**
     * @brief Construct with default configuration.
     */
    AdaptiveCircuitBreaker();

    /**
     * @brief Construct with the given configuration.
     */
    explicit AdaptiveCircuitBreaker(const Config& config);

    // Non-copyable, movable
    AdaptiveCircuitBreaker(const AdaptiveCircuitBreaker&)            = delete;
    AdaptiveCircuitBreaker& operator=(const AdaptiveCircuitBreaker&) = delete;
    AdaptiveCircuitBreaker(AdaptiveCircuitBreaker&&)                 noexcept = default;
    AdaptiveCircuitBreaker& operator=(AdaptiveCircuitBreaker&&)      noexcept = default;

    ~AdaptiveCircuitBreaker() = default;

    // -----------------------------------------------------------------------
    // Core circuit breaker API
    // -----------------------------------------------------------------------

    /**
     * @brief Decide whether to allow the next request.
     *
     * - CLOSED  → always allows
     * - OPEN    → blocks unless @c open_timeout has elapsed; if elapsed,
     *             transitions to HALF_OPEN and allows one probe request
     * - HALF_OPEN → allows up to @c success_threshold probe requests
     *
     * @return true if the request should be attempted, false to fast-fail.
     */
    bool shouldAllow();

    /**
     * @brief Notify that the last request succeeded.
     *
     * In HALF_OPEN: accumulates successes; closes circuit when
     * @c success_threshold consecutive successes are reached.
     */
    void recordSuccess();

    /**
     * @brief Notify that the last request failed.
     *
     * In CLOSED: increments failure counter; opens circuit when
     * @c failure_threshold is reached.  Adaptive mode may lower the
     * effective threshold on repeated trips.
     *
     * In HALF_OPEN: immediately re-opens the circuit.
     */
    void recordFailure();

    /**
     * @brief Notify that the last request failed with a specific error class.
     *
     * If the error class has a configured override in
     * @c Config::error_class_configs, its consecutive-failure counter is
     * incremented separately.  When that counter reaches the class threshold
     * the circuit trips immediately, regardless of the global counter.
     *
     * Falls through to the global @c recordFailure() accounting as well.
     *
     * @param error_class  String label for the error category (e.g., "timeout",
     *                     "connection", "auth").  Unknown classes are tracked
     *                     globally only (no per-class threshold applied).
     */
    void recordFailure(const ErrorClass& error_class);

    /**
     * @brief Return the current circuit state (lock-free read).
     */
    CircuitState getState() const;

    /**
     * @brief Return a consistent statistics snapshot.
     */
    Stats getStats() const;

    /**
     * @brief Force the circuit breaker back to CLOSED and clear all counters.
     *
     * Resets consecutive failure/success streaks, the adaptive trip counter,
     * and restores the effective failure threshold to the configured value.
     */
    void reset();

    /**
     * @brief Immediately trip the circuit to OPEN regardless of failure count.
     */
    void forceOpen();

    /**
     * @brief Register a callback invoked on every state transition.
     *
     * Signature: void(CircuitState from, CircuitState to)
     * The callback is invoked while the internal lock is held; keep it short.
     */
    void setStateChangeCallback(
        std::function<void(CircuitState, CircuitState)> callback);

    /**
     * @brief Convert a CircuitState to its human-readable name.
     */
    static std::string stateToString(CircuitState state);

private:
    Config  config_;
    size_t  effective_failure_threshold_; ///< May differ from config_ when adaptive
    size_t  consecutive_trip_count_{0};   ///< How many times circuit has tripped without long-term recovery

    // Atomic state — hot read path is lock-free
    std::atomic<CircuitState> state_{CircuitState::CLOSED};

    // Counters
    std::atomic<uint64_t> total_calls_{0};
    std::atomic<uint64_t> successful_calls_{0};
    std::atomic<uint64_t> failed_calls_{0};
    std::atomic<uint64_t> rejected_calls_{0};

    // Protected by mutex_
    mutable std::mutex mutex_;
    size_t  consecutive_failures_{0}; ///< Failure streak in CLOSED state
    size_t  half_open_successes_{0};  ///< Success streak in HALF_OPEN state

    /// Per-error-class consecutive failure counters (protected by mutex_).
    std::unordered_map<ErrorClass, size_t> error_class_consecutive_failures_;

    /// Per-error-class effective thresholds (initialised from config, may
    /// be reduced by adaptive logic independently of the global threshold).
    std::unordered_map<ErrorClass, size_t> error_class_effective_thresholds_;

    std::chrono::steady_clock::time_point open_timestamp_;
    std::chrono::steady_clock::time_point last_state_change_{
        std::chrono::steady_clock::now()};

    std::function<void(CircuitState, CircuitState)> state_change_cb_;

    // -----------------------------------------------------------------------
    // Internal helpers (called under mutex_)
    // -----------------------------------------------------------------------

    /**
     * @brief Transition to @p new_state, invoke callback, record timestamp.
     */
    void transitionTo(CircuitState new_state);

    /**
     * @brief Adapt the effective failure threshold after a circuit trip.
     *
     * Reduces the threshold by @c adaptive_factor percent (minimum 1) so that
     * a flapping downstream service trips the breaker faster next time.
     */
    void adaptThresholdOnTrip();

    /**
     * @brief Record a per-error-class failure and return true if the class
     *        threshold was reached (circuit should be tripped).
     *
     * Called under @c mutex_ while in CLOSED state.
     *
     * @param error_class  Error class label.
     * @return true if the per-class threshold was reached.
     */
    bool recordErrorClassFailure(const ErrorClass& error_class);

    /**
     * @brief Reset all per-error-class consecutive failure counters.
     *
     * Called under @c mutex_ on circuit close or reset.
     */
    void resetErrorClassCounters();
};

} // namespace network
} // namespace themis
