// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file circuit_breaker.h
 * @brief Plugin-level circuit breaker for ThemisDB RPC backends.
 *
 * Implements the classic three-state circuit breaker pattern:
 *   - CLOSED   — normal operation; failures are counted.
 *   - OPEN     — requests are rejected immediately; recovery window is running.
 *   - HALF_OPEN — one probe call is allowed; success → CLOSED, failure → OPEN.
 *
 * Thread-safe. All state transitions are protected by an internal mutex.
 *
 * ### Usage
 * ```cpp
 * using namespace themis::rpc;
 *
 * CircuitBreaker cb("grpc-backend", CircuitBreakerConfig{});
 *
 * if (!cb.allowRequest()) {
 *     return RpcStatus::kCircuitOpen;
 * }
 * bool ok = callDownstream();
 * cb.recordResult(ok);
 * ```
 *
 * @see src/rpc_grpc/circuit_breaker.cpp
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace themis {
namespace rpc {

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Tuning parameters for a CircuitBreaker instance.
 *
 * All timeout values use `std::chrono::milliseconds` for precision without
 * requiring floating-point arithmetic.
 */
struct CircuitBreakerConfig {
    /// Number of consecutive failures that trip the circuit (CLOSED → OPEN).
    uint32_t failure_threshold{5};

    /// Minimum number of calls in a sliding window before the failure rate is
    /// evaluated.  Prevents tripping on a single cold-start call.
    uint32_t min_calls_in_window{3};

    /// How long the circuit stays OPEN before a probe is attempted (ms).
    std::chrono::milliseconds recovery_window{std::chrono::seconds{30}};

    /// Maximum number of consecutive successes in HALF_OPEN before the
    /// circuit fully closes again.
    uint32_t half_open_success_threshold{2};

    /// Optional name for logging / Prometheus label.  May be empty.
    std::string name;
};

// ============================================================================
// State
// ============================================================================

/**
 * @brief Observable circuit state.
 */
enum class CircuitState : uint8_t {
    kClosed   = 0, ///< Normal; calls pass through.
    kOpen     = 1, ///< Tripped; calls are rejected.
    kHalfOpen = 2, ///< Probe mode; one request is admitted.
};

/// @brief Human-readable label for @p state.
inline const char* circuitStateToString(CircuitState state) noexcept {
    switch (state) {
        case CircuitState::kClosed:   return "CLOSED";
        case CircuitState::kOpen:     return "OPEN";
        case CircuitState::kHalfOpen: return "HALF_OPEN";
    }
    return "UNKNOWN";
}

// ============================================================================
// Metrics snapshot
// ============================================================================

/**
 * @brief Point-in-time snapshot of circuit breaker counters.
 */
struct CircuitBreakerStats {
    CircuitState state{CircuitState::kClosed};
    uint64_t     total_calls{0};
    uint64_t     successful_calls{0};
    uint64_t     failed_calls{0};
    uint64_t     rejected_calls{0};   ///< Calls rejected because circuit was OPEN.
    uint64_t     state_transitions{0};
    std::string  name;
};

// ============================================================================
// CircuitBreaker
// ============================================================================

/**
 * @brief Plugin-level circuit breaker.
 *
 * Instances are typically owned by an `IRPCPlugin` or `IRPCServer`
 * implementation and consulted before each outgoing call.
 *
 * ### State machine
 * ```
 *  CLOSED  --[failures >= threshold]--> OPEN
 *  OPEN    --[recovery window elapsed]--> HALF_OPEN
 *  HALF_OPEN --[probe succeeds (N times)]--> CLOSED
 *  HALF_OPEN --[probe fails]--> OPEN
 * ```
 *
 * ### Thread safety
 * All public methods are thread-safe.  `allowRequest()` and `recordResult()`
 * acquire the internal mutex for the duration of the call.
 */
class CircuitBreaker {
public:
    /**
     * @brief Construct a new circuit breaker.
     * @param config  Tuning configuration.  Copied on construction.
     */
    explicit CircuitBreaker(CircuitBreakerConfig config = {});

    /**
     * @brief Construct a new circuit breaker with an explicit name.
     * @param name    Descriptive name used in metrics and logs.
     * @param config  Tuning configuration.
     */
    CircuitBreaker(std::string name, CircuitBreakerConfig config = {});

    /// Non-copyable; circuit breakers hold live state.
    CircuitBreaker(const CircuitBreaker&)            = delete;
    CircuitBreaker& operator=(const CircuitBreaker&) = delete;

    /// Movable.
    CircuitBreaker(CircuitBreaker&&) noexcept;
    CircuitBreaker& operator=(CircuitBreaker&&) noexcept;

    ~CircuitBreaker() = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Query whether the next call should be allowed.
     *
     * - **CLOSED** → always returns `true`.
     * - **OPEN**   → returns `false` unless the recovery window has elapsed,
     *                in which case the state transitions to HALF_OPEN and
     *                `true` is returned for the first probe call.
     * - **HALF_OPEN** → returns `true` only for the first probe; subsequent
     *                concurrent callers get `false` until the probe outcome is
     *                recorded.
     *
     * @return `true` if the call may proceed; `false` if it must be rejected.
     */
    [[nodiscard]] bool allowRequest();

    /**
     * @brief Record the outcome of a call that was allowed by `allowRequest()`.
     *
     * Must be called exactly once for every call where `allowRequest()`
     * returned `true`.  Updating state based on the outcome:
     *
     * - **CLOSED + success** → failure counter reset (sliding window).
     * - **CLOSED + failure** → failure counter incremented; if threshold is
     *                          reached the circuit trips to OPEN.
     * - **HALF_OPEN + success** → success counter incremented; when
     *                             `half_open_success_threshold` is reached the
     *                             circuit closes.
     * - **HALF_OPEN + failure** → circuit reopens immediately.
     *
     * @param success  `true` = call succeeded; `false` = call failed.
     */
    void recordResult(bool success);

    /**
     * @brief Manually force the circuit into a specific state.
     *
     * Intended for operator overrides and test fixtures.  The forced state is
     * fully honoured by `allowRequest()` / `recordResult()`.
     *
     * @param state  Target state.
     */
    void forceState(CircuitState state);

    /**
     * @brief Reset all counters and return the circuit to CLOSED.
     *
     * Intended for operator overrides after a sustained outage.
     */
    void reset();

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /// @brief Return the current circuit state (lock-free read).
    [[nodiscard]] CircuitState state() const noexcept;

    /// @brief Return a consistent point-in-time metrics snapshot.
    [[nodiscard]] CircuitBreakerStats stats() const;

    /// @brief Return the configured name.
    [[nodiscard]] const std::string& name() const noexcept { return config_.name; }

    // -----------------------------------------------------------------------
    // Callback hook
    // -----------------------------------------------------------------------

    /**
     * @brief Register a listener that is called on every state transition.
     *
     * The callback receives the old state, new state, and circuit name.
     * Called under the internal mutex; keep it non-blocking.
     *
     * @param cb  Callable matching `void(CircuitState old, CircuitState next,
     *            const std::string& name)`.
     */
    void setTransitionCallback(
        std::function<void(CircuitState, CircuitState, const std::string&)> cb);

private:
    void transitionTo(CircuitState next_state);  ///< Caller must hold mutex_.

    CircuitBreakerConfig config_;

    mutable std::mutex mutex_;

    CircuitState state_{CircuitState::kClosed};

    // Counters
    uint64_t consecutive_failures_{0};
    uint64_t consecutive_successes_{0};  ///< Used in HALF_OPEN only.
    uint64_t total_calls_{0};
    uint64_t successful_calls_{0};
    uint64_t failed_calls_{0};
    uint64_t rejected_calls_{0};
    uint64_t state_transitions_{0};

    // OPEN timing
    std::chrono::steady_clock::time_point open_since_{};

    // HALF_OPEN probe guard — only one probe in-flight at a time.
    bool probe_in_flight_{false};

    std::function<void(CircuitState, CircuitState, const std::string&)>
        transition_cb_;
};

}  // namespace rpc
}  // namespace themis
