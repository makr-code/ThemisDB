/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            retry_policy.hpp                                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:06:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     462                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 353ab78386  2026-03-12  fix(chimera): address all review comments on retry policy ║
    • 350ef6ad71  2026-03-12  feat(chimera): implement Error Recovery and Retry Logic (... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file retry_policy.hpp
 * @brief Error Recovery and Retry Logic for the CHIMERA adapter layer
 *
 * @details
 * Provides automatic error recovery for transient database failures.
 * Key components:
 *
 *  - **RetryPolicy**        – Configurable retry parameters (max retries,
 *                             exponential backoff, error classification).
 *  - **CircuitBreaker**     – Three-state machine (CLOSED → OPEN → HALF_OPEN)
 *                             that short-circuits calls to a failing back-end
 *                             and allows controlled recovery probes.
 *  - **ConnectionWithRetry** – Decorator around IDatabaseAdapter that applies
 *                             RetryPolicy and CircuitBreaker transparently.
 *
 * @par Usage
 * @code
 * chimera::RetryPolicy policy;
 * policy.max_retries = 5;
 * policy.is_transient = [](chimera::ErrorCode code) {
 *     return code == chimera::ErrorCode::TIMEOUT ||
 *            code == chimera::ErrorCode::CONNECTION_ERROR;
 * };
 *
 * auto adapter = chimera::AdapterFactory::create("ThemisDB");
 * chimera::ConnectionWithRetry conn(std::move(adapter), policy);
 *
 * auto result = conn.execute_with_retry([&]() {
 *     return conn.adapter().execute_query("SELECT 1");
 * });
 * @endcode
 *
 * @copyright MIT License
 * @version 1.0.0
 */

#ifndef CHIMERA_RETRY_POLICY_HPP
#define CHIMERA_RETRY_POLICY_HPP

#include "chimera/database_adapter.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

namespace chimera {

// ============================================================================
// RetryPolicy
// ============================================================================

/**
 * @struct RetryPolicy
 * @brief Configurable retry policy for transient database error recovery.
 *
 * Controls how ConnectionWithRetry re-attempts a failed operation using
 * exponential backoff.  Permanent errors (as determined by
 * @c is_transient) are never retried.
 */
struct RetryPolicy {
    /// Maximum number of retry attempts after the first failure.
    size_t max_retries = 3;

    /// Delay before the first retry.
    std::chrono::milliseconds initial_delay{100};

    /// Multiplicative factor applied to the delay after each retry.
    double backoff_multiplier = 2.0;

    /// Upper bound on the computed backoff delay.
    std::chrono::milliseconds max_delay{10000};

    /**
     * @brief Classify an error code as transient (retryable) or permanent.
     *
     * Transient errors trigger a retry with backoff.  Permanent errors
     * (e.g. PERMISSION_DENIED, INVALID_ARGUMENT) propagate immediately.
     *
     * The default implementation treats CONNECTION_ERROR and TIMEOUT as
     * transient and everything else as permanent.
     */
    std::function<bool(ErrorCode)> is_transient = [](ErrorCode code) {
        return code == ErrorCode::CONNECTION_ERROR ||
               code == ErrorCode::TIMEOUT;
    };
};

// ============================================================================
// CircuitBreaker
// ============================================================================

/**
 * @enum CircuitState
 * @brief States of the circuit breaker state machine.
 */
enum class CircuitState {
    CLOSED,    ///< Normal operation — requests flow through.
    OPEN,      ///< Circuit tripped — calls are rejected immediately.
    HALF_OPEN  ///< Recovery probe — a single request is allowed through.
};

/**
 * @class CircuitBreaker
 * @brief Thread-safe circuit breaker for adapter failure isolation.
 *
 * Implements the classic CLOSED → OPEN → HALF_OPEN → CLOSED state machine:
 *
 *  - **CLOSED**: All calls pass through.  After @c failure_threshold
 *    consecutive failures the circuit trips to OPEN.
 *  - **OPEN**: Calls are rejected immediately with TIMEOUT.  After
 *    @c open_timeout the circuit transitions to HALF_OPEN.
 *  - **HALF_OPEN**: One probe call is allowed.  On success the circuit
 *    closes; on failure it re-opens.
 *
 * @note All public methods are thread-safe.
 */
class CircuitBreaker {
public:
    /**
     * @brief Construct with configurable thresholds and timeouts.
     * @param failure_threshold Consecutive failures before opening (default 5).
     * @param open_timeout      How long the circuit stays OPEN (default 60 s).
     */
    explicit CircuitBreaker(
        size_t failure_threshold = 5,
        std::chrono::milliseconds open_timeout = std::chrono::seconds(60)
    )
        : failure_threshold_(failure_threshold)
        , open_timeout_(open_timeout)
        , state_(CircuitState::CLOSED)
        , consecutive_failures_(0)
        , last_opened_{}
    {}

    /**
     * @brief Check whether a call should be allowed through.
     *
     * CLOSED  → true always.
     * OPEN    → false, unless the open timeout has elapsed in which case
     *           the circuit transitions to HALF_OPEN and returns true for
     *           the probe request.
     * HALF_OPEN → true (one probe allowed).
     */
    bool allow_request() {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();

        switch (state_) {
            case CircuitState::CLOSED:
                return true;

            case CircuitState::OPEN:
                if (now - last_opened_ >= open_timeout_) {
                    state_ = CircuitState::HALF_OPEN;
                    return true; // probe attempt
                }
                return false;

            case CircuitState::HALF_OPEN:
                return true;
        }
        return false; // unreachable
    }

    /**
     * @brief Record a successful call outcome.
     *
     * CLOSED    → resets the consecutive failure counter.
     * HALF_OPEN → transitions back to CLOSED.
     */
    void record_success() {
        std::lock_guard<std::mutex> lock(mutex_);
        consecutive_failures_ = 0;
        state_ = CircuitState::CLOSED;
    }

    /**
     * @brief Record a failed call outcome.
     *
     * CLOSED    → increments failure counter; opens circuit when threshold
     *             is reached.
     * HALF_OPEN → re-opens the circuit immediately (counter not incremented).
     * OPEN      → no-op (already open).
     */
    void record_failure() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == CircuitState::OPEN) {
            return; // already open — no-op
        }

        ++consecutive_failures_;

        if (state_ == CircuitState::HALF_OPEN ||
            consecutive_failures_ >= failure_threshold_) {
            state_ = CircuitState::OPEN;
            last_opened_ = std::chrono::steady_clock::now();
        }
    }

    /**
     * @brief Return the current circuit state (snapshot).
     */
    CircuitState state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    /**
     * @brief Return the current consecutive failure count (snapshot).
     */
    size_t consecutive_failures() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return consecutive_failures_;
    }

    /**
     * @brief Reset the circuit breaker to the CLOSED state.
     *
     * Useful in tests and after manual recovery.
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        consecutive_failures_ = 0;
        state_ = CircuitState::CLOSED;
    }

private:
    const size_t failure_threshold_;
    const std::chrono::milliseconds open_timeout_;

    mutable std::mutex mutex_;
    CircuitState state_;
    size_t consecutive_failures_;
    std::chrono::steady_clock::time_point last_opened_;
};

// ============================================================================
// ConnectionWithRetry
// ============================================================================

/**
 * @class ConnectionWithRetry
 * @brief Decorator that adds retry and circuit-breaker behaviour to any adapter.
 *
 * Wraps an @c IDatabaseAdapter and transparently retries transient failures
 * using exponential backoff as configured by @c RetryPolicy.  A built-in
 * @c CircuitBreaker prevents retry storms when the back-end is consistently
 * unavailable.
 *
 * @par Thread safety
 * The class itself is thread-safe: the underlying adapter and circuit breaker
 * are accessed under their own mutexes.  The wrapped @c IDatabaseAdapter,
 * however, may or may not be thread-safe — consult its documentation.
 */
class ConnectionWithRetry {
public:
    /**
     * @brief Construct wrapping the given adapter with the given policy.
     * @param adapter Owning pointer to the underlying adapter.
     * @param policy  Retry configuration.
     */
    ConnectionWithRetry(
        std::unique_ptr<IDatabaseAdapter> adapter,
        const RetryPolicy& policy = RetryPolicy{}
    )
        : adapter_(std::move(adapter))
        , policy_(policy)
        , circuit_breaker_(
              /* failure_threshold = */ 5,
              /* open_timeout      = */ std::chrono::seconds(60)
          )
    {}

    // Non-copyable; movable.
    ConnectionWithRetry(const ConnectionWithRetry&) = delete;
    ConnectionWithRetry& operator=(const ConnectionWithRetry&) = delete;
    ConnectionWithRetry(ConnectionWithRetry&&) = default;
    ConnectionWithRetry& operator=(ConnectionWithRetry&&) = default;

    // -------------------------------------------------------------------------
    // Core retry primitive
    // -------------------------------------------------------------------------

    /**
     * @brief Execute @p operation with automatic retry and circuit-breaker.
     *
     * @tparam T         The success value type wrapped in the returned Result.
     * @tparam Operation Callable type with signature @c Result<T>().
     * @param operation  Callable that performs the database operation.
     *                   Called repeatedly until it succeeds, a non-transient
     *                   error is encountered, retries are exhausted, or the
     *                   circuit breaker rejects the call.
     * @return The last Result returned by @p operation (success or the final
     *         error after all retries are consumed).
     *
     * @par Behaviour
     * - If the circuit breaker is OPEN the call is rejected immediately with
     *   TIMEOUT and the message "Circuit breaker is OPEN".
     * - On a permanent error (is_transient returns false) the error is
     *   returned without retrying.
     * - On a transient error the delay starts at @c initial_delay and doubles
     *   each attempt (capped at @c max_delay).
     */
    template<typename T, typename Operation>
    Result<T> execute_with_retry(Operation&& operation) {
        static_assert(
            std::is_invocable_r<Result<T>, Operation>::value,
            "Operation must be callable with no arguments and return Result<T>");
        if (!circuit_breaker_.allow_request()) {
            return Result<T>::err(ErrorCode::TIMEOUT,
                                  "Circuit breaker is OPEN — request rejected");
        }

        Result<T> last_result = Result<T>::err(
            ErrorCode::INTERNAL_ERROR, "No attempts made");

        auto delay = policy_.initial_delay;

        for (size_t attempt = 0; attempt <= policy_.max_retries; ++attempt) {
            // Re-check the circuit breaker on subsequent attempts.
            if (attempt > 0 && !circuit_breaker_.allow_request()) {
                return Result<T>::err(ErrorCode::TIMEOUT,
                                      "Circuit breaker is OPEN — request rejected");
            }

            last_result = operation();

            if (last_result.is_ok()) {
                circuit_breaker_.record_success();
                return last_result;
            }

            // Permanent error — do not retry.
            if (!policy_.is_transient || !policy_.is_transient(last_result.error_code)) {
                circuit_breaker_.record_failure();
                return last_result;
            }

            // Transient error — record failure and maybe retry.
            circuit_breaker_.record_failure();

            if (attempt < policy_.max_retries) {
                std::this_thread::sleep_for(delay);
                const auto next_ms = static_cast<
                    std::chrono::milliseconds::rep>(
                    delay.count() * policy_.backoff_multiplier);
                const auto capped = std::min(
                    next_ms, policy_.max_delay.count());
                delay = std::chrono::milliseconds(capped);
            }
        }

        return last_result;
    }

    // -------------------------------------------------------------------------
    // Accessor helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Access the underlying adapter (const).
     * @return Reference to the wrapped IDatabaseAdapter.
     */
    const IDatabaseAdapter& adapter() const {
        return *adapter_;
    }

    /**
     * @brief Access the underlying adapter (mutable).
     * @return Reference to the wrapped IDatabaseAdapter.
     */
    IDatabaseAdapter& adapter() {
        return *adapter_;
    }

    /**
     * @brief Access the circuit breaker for monitoring (const).
     */
    const CircuitBreaker& circuit_breaker() const {
        return circuit_breaker_;
    }

    /**
     * @brief Access the circuit breaker for state manipulation (mutable).
     */
    CircuitBreaker& circuit_breaker() {
        return circuit_breaker_;
    }

    /**
     * @brief Access the active retry policy.
     */
    const RetryPolicy& policy() const {
        return policy_;
    }

    /**
     * @brief Execute a health check on the underlying adapter.
     *
     * Calls @c is_connected() and, if disconnected, attempts @c connect()
     * with the provided (or default empty) @p connection_string to restore
     * the connection.  The circuit breaker is updated accordingly.
     *
     * @param connection_string   Optional URI forwarded to connect() on
     *                            reconnect (defaults to an empty string).
     * @return true if the adapter reports a live connection after the check.
     */
    bool health_check(const std::string& connection_string = "") {
        if (adapter_->is_connected()) {
            circuit_breaker_.record_success();
            return true;
        }

        auto result = adapter_->connect(connection_string);
        if (result.is_ok() && result.value.value_or(false)) {
            circuit_breaker_.record_success();
            return true;
        }

        circuit_breaker_.record_failure();
        return false;
    }

private:
    std::unique_ptr<IDatabaseAdapter> adapter_;
    RetryPolicy policy_;
    CircuitBreaker circuit_breaker_;
};

} // namespace chimera

#endif // CHIMERA_RETRY_POLICY_HPP
