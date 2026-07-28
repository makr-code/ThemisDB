/**
 * @file i_circuit_breaker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include "sharding/circuit_breaker.h"
#include <string>
#include <chrono>
#include <memory>
#include <type_traits>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract circuit breaker interface for dependency injection.
 *
 * Provides a unified circuit breaker abstraction that can be wired into the
 * ConcernsContext as a first-class concern alongside logging, tracing, metrics,
 * and caching.  Implementations wrap or re-implement the CLOSED → OPEN →
 * HALF_OPEN state machine and are swappable at construction time.
 *
 * Thread-safety: all methods must be safe to call concurrently.
 */
class ICircuitBreaker {
public:
    /**
     * @brief Circuit breaker states.
     */
    enum class State {
        CLOSED,    ///< Normal operation — requests are allowed
        OPEN,      ///< Circuit tripped — requests are blocked
        HALF_OPEN  ///< Recovery probe — limited requests are allowed
    };

    /**
     * @brief Construction-time configuration.
     */
    struct Config {
        /// Number of failures within @c failure_window that trips the circuit.
        size_t failure_threshold = 5;
        /// How long the circuit stays OPEN before probing recovery.
        std::chrono::seconds timeout = std::chrono::seconds(30);
        /// Consecutive successes in HALF_OPEN required to close the circuit.
        size_t success_threshold = 2;
        /// Rolling window for counting failures.
        std::chrono::seconds failure_window = std::chrono::seconds(60);
    };

    virtual ~ICircuitBreaker() = default;

    // -----------------------------------------------------------------------
    // Core circuit breaker operations
    // -----------------------------------------------------------------------

    /**
     * @brief Decide whether to allow the next request.
     *
     * Returns false immediately when the circuit is OPEN and the recovery
     * timeout has not elapsed.  In HALF_OPEN state a limited number of probe
     * requests are passed through.
     *
     * @return true if the request should be attempted, false if it should be
     *         short-circuited.
     */
    [[nodiscard]] virtual bool allowRequest() = 0;

    /**
     * @brief Notify the circuit breaker that the last request succeeded.
     *
     * In HALF_OPEN state enough consecutive successes close the circuit.
     */
    virtual void recordSuccess() = 0;

    /**
     * @brief Notify the circuit breaker that the last request failed.
     *
     * In CLOSED state enough failures within the rolling window open the
     * circuit.  In HALF_OPEN state any single failure re-opens it.
     */
    virtual void recordFailure() = 0;

    /**
     * @brief Return the current circuit breaker state.
     */
    [[nodiscard]] virtual State getState() const = 0;

    /**
     * @brief Return the number of failures recorded in the current window.
     */
    [[nodiscard]] virtual size_t getFailureCount() const = 0;

    /**
     * @brief Return the number of consecutive successes in HALF_OPEN state.
     */
    [[nodiscard]] virtual size_t getSuccessCount() const = 0;

    /**
     * @brief Force the circuit breaker back to CLOSED and clear all counters.
     */
    virtual void reset() = 0;

    /**
     * @brief Immediately trip the circuit to OPEN regardless of failure count.
     */
    virtual void forceOpen() = 0;

    // -----------------------------------------------------------------------
    // Call wrapper
    // -----------------------------------------------------------------------

    /**
     * @brief Execute @p fn guarded by the circuit breaker; use @p fallback if
     *        the circuit is open.
     *
     * Behaviour by state:
     *  - CLOSED   → @p fn is called; success/failure is recorded.
     *  - OPEN     → @p fallback is called immediately (no @p fn attempt).
     *  - HALF_OPEN → @p fn is called as a probe; result is recorded.
     *
     * If @p fn throws the exception is propagated after calling
     * @c recordFailure().
     *
     * @tparam Fn       Callable with signature @c () -> R.
     * @tparam Fallback Callable with signature @c () -> R (same return type).
     * @param  fn       Primary function to execute.
     * @param  fallback Fallback to execute when the circuit is open.
     * @return          Result of @p fn on success or @p fallback when open.
     */
    template<typename Fn, typename Fallback>
    auto call(Fn&& fn, Fallback&& fallback) -> decltype(fn()) {
        if (!allowRequest()) {
            return std::forward<Fallback>(fallback)();
        }
        if constexpr (std::is_void_v<decltype(fn())>) {
            try {
                std::forward<Fn>(fn)();
                recordSuccess();
            } catch (...) {
                recordFailure();
                throw;
            }
        } else {
            try {
                auto result = std::forward<Fn>(fn)();
                recordSuccess();
                return result;
            } catch (...) {
                recordFailure();
                throw;
            }
        }
    }

    /**
     * @brief Convert a State value to its human-readable name.
     */
    static std::string stateToString(State state) {
        switch (state) {
            case State::CLOSED:    return "CLOSED";
            case State::OPEN:      return "OPEN";
            case State::HALF_OPEN: return "HALF_OPEN";
        }
        return "UNKNOWN";
    }

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush any pending state (no-op for in-process implementations).
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down and release resources.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the circuit breaker is healthy.
     *
     * An OPEN circuit is considered unhealthy; CLOSED and HALF_OPEN are
     * healthy (the latter means recovery is in progress).
     *
     * @return ProbeResult with ok=true when the circuit is not OPEN.
     */
    virtual ProbeResult isHealthy() const {
        return getState() == State::OPEN
            ? ProbeResult::unhealthy("circuit breaker OPEN")
            : ProbeResult::healthy();
    }
};

/**
 * @brief Default circuit breaker backed by themis::sharding::CircuitBreaker.
 *
 * Adapts the existing production-grade sharding circuit breaker implementation
 * to the ICircuitBreaker interface so it can be injected as a core concern.
 */
class DefaultCircuitBreaker : public ICircuitBreaker {
public:
    explicit DefaultCircuitBreaker(const Config& config = Config{}) {
        sharding::CircuitBreaker::Config sharding_cfg;
        sharding_cfg.failure_threshold = config.failure_threshold;
        sharding_cfg.timeout           = config.timeout;
        sharding_cfg.success_threshold = config.success_threshold;
        sharding_cfg.failure_window    = config.failure_window;
        impl_ = std::make_unique<sharding::CircuitBreaker>(sharding_cfg);
    }

    bool   allowRequest()          override { return impl_->allowRequest(); }
    void   recordSuccess()         override { impl_->recordSuccess(); }
    void   recordFailure()         override { impl_->recordFailure(); }
    size_t getFailureCount() const override { return impl_->getFailureCount(); }
    size_t getSuccessCount() const override { return impl_->getSuccessCount(); }
    void   reset()                 override { impl_->reset(); }
    void   forceOpen()             override { impl_->forceOpen(); }

    State getState() const override {
        switch (impl_->getState()) {
            case sharding::CircuitBreaker::State::CLOSED:    return State::CLOSED;
            case sharding::CircuitBreaker::State::OPEN:      return State::OPEN;
            case sharding::CircuitBreaker::State::HALF_OPEN: return State::HALF_OPEN;
        }
        return State::CLOSED;
    }

private:
    std::unique_ptr<sharding::CircuitBreaker> impl_;
};

} // namespace concerns
} // namespace core
} // namespace themis
