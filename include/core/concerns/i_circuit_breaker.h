/**
 * @file i_circuit_breaker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

class ICircuitBreaker {
public:
    enum class State {
        CLOSED,    ///< Normal operation — requests are allowed
        OPEN,      ///< Circuit tripped — requests are blocked
        HALF_OPEN  ///< Recovery probe — limited requests are allowed
    };

    struct Config {
        size_t failure_threshold = 5;
        std::chrono::seconds timeout = std::chrono::seconds(30);
        size_t success_threshold = 2;
        std::chrono::seconds failure_window = std::chrono::seconds(60);
    };

    /**
     * @brief ICircuit Breaker.
     * @return Return value.
     */
    virtual ~ICircuitBreaker() = default;

    // -----------------------------------------------------------------------
    // Core circuit breaker operations
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual bool allowRequest() = 0;

    /**
     * @brief Record Success.
     */
    virtual void recordSuccess() = 0;

    /**
     * @brief Record Failure.
     */
    virtual void recordFailure() = 0;

    [[nodiscard]] virtual State getState() const = 0;

    [[nodiscard]] virtual size_t getFailureCount() const = 0;

    [[nodiscard]] virtual size_t getSuccessCount() const = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;

    /**
     * @brief Force Open.
     */
    virtual void forceOpen() = 0;

    // -----------------------------------------------------------------------
    // Call wrapper
    // -----------------------------------------------------------------------

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
     * @brief State To String.
     * @param[in] state Input parameter.
     * @return Return value.
     * @details Implements stateToString without additional internal calls.
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

    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const {
        return getState() == State::OPEN
            ? ProbeResult::unhealthy("circuit breaker OPEN")
            : ProbeResult::healthy();
    }
};

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
