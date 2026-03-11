/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_circuit_breaker.cpp                       ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-11                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "network/adaptive_circuit_breaker.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace network {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AdaptiveCircuitBreaker::AdaptiveCircuitBreaker()
    : AdaptiveCircuitBreaker(Config{})
{}

AdaptiveCircuitBreaker::AdaptiveCircuitBreaker(const Config& config)
    : config_(config)
    , effective_failure_threshold_(config.failure_threshold)
{
    if (config_.failure_threshold == 0) {
        throw std::invalid_argument(
            "AdaptiveCircuitBreaker: failure_threshold must be >= 1");
    }
    if (config_.success_threshold == 0) {
        throw std::invalid_argument(
            "AdaptiveCircuitBreaker: success_threshold must be >= 1");
    }
    if (config_.adaptive_factor <= 0.0 || config_.adaptive_factor >= 1.0) {
        throw std::invalid_argument(
            "AdaptiveCircuitBreaker: adaptive_factor must be in (0, 1)");
    }
}

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

bool AdaptiveCircuitBreaker::shouldAllow() {
    ++total_calls_;

    const CircuitState s = state_.load(std::memory_order_acquire);

    if (s == CircuitState::CLOSED) {
        return true;
    }

    if (s == CircuitState::OPEN) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Re-check under lock — another thread may have transitioned already
        if (state_.load(std::memory_order_relaxed) != CircuitState::OPEN) {
            return true;
        }
        const auto elapsed = std::chrono::steady_clock::now() - open_timestamp_;
        if (elapsed >= config_.open_timeout) {
            // Timeout elapsed — probe with one request
            transitionTo(CircuitState::HALF_OPEN);
            return true;
        }
        ++rejected_calls_;
        return false;
    }

    // HALF_OPEN: allow limited probe requests
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.load(std::memory_order_relaxed) != CircuitState::HALF_OPEN) {
            // Raced with a state transition — re-evaluate
            return shouldAllow();
        }
        // Check half-open timeout: if we've been in HALF_OPEN too long,
        // re-open the circuit.
        const auto elapsed = std::chrono::steady_clock::now() - last_state_change_;
        if (elapsed >= config_.half_open_timeout) {
            transitionTo(CircuitState::OPEN);
            ++rejected_calls_;
            return false;
        }
        // Allow the probe request
        return true;
    }
}

void AdaptiveCircuitBreaker::recordSuccess() {
    ++successful_calls_;

    const CircuitState s = state_.load(std::memory_order_acquire);

    if (s == CircuitState::CLOSED) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Reset consecutive failure counter on any success in CLOSED state
        consecutive_failures_ = 0;
        return;
    }

    if (s == CircuitState::HALF_OPEN) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.load(std::memory_order_relaxed) != CircuitState::HALF_OPEN) {
            return;
        }
        ++half_open_successes_;
        if (half_open_successes_ >= config_.success_threshold) {
            half_open_successes_ = 0;
            consecutive_trip_count_ = 0; // Full recovery — reset trip counter
            // On sustained recovery, relax the adaptive threshold back toward
            // the original configured value.
            if (config_.enable_adaptive_threshold &&
                effective_failure_threshold_ < config_.failure_threshold) {
                const size_t bump = std::max<size_t>(
                    1,
                    static_cast<size_t>(
                        std::ceil(config_.failure_threshold * config_.adaptive_factor)));
                effective_failure_threshold_ = std::min(
                    effective_failure_threshold_ + bump,
                    config_.failure_threshold);
            }
            transitionTo(CircuitState::CLOSED);
        }
    }
    // Success while OPEN is ignored (shouldn't happen but is harmless)
}

void AdaptiveCircuitBreaker::recordFailure() {
    ++failed_calls_;

    const CircuitState s = state_.load(std::memory_order_acquire);

    if (s == CircuitState::CLOSED) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.load(std::memory_order_relaxed) != CircuitState::CLOSED) {
            return; // Already transitioned
        }
        ++consecutive_failures_;
        if (consecutive_failures_ >= effective_failure_threshold_) {
            consecutive_failures_ = 0;
            if (config_.enable_adaptive_threshold) {
                adaptThresholdOnTrip();
            }
            transitionTo(CircuitState::OPEN);
        }
        return;
    }

    if (s == CircuitState::HALF_OPEN) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.load(std::memory_order_relaxed) != CircuitState::HALF_OPEN) {
            return;
        }
        half_open_successes_ = 0;
        if (config_.enable_adaptive_threshold) {
            adaptThresholdOnTrip();
        }
        transitionTo(CircuitState::OPEN);
    }
    // Failure while already OPEN is ignored
}

CircuitState AdaptiveCircuitBreaker::getState() const {
    return state_.load(std::memory_order_acquire);
}

AdaptiveCircuitBreaker::Stats AdaptiveCircuitBreaker::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.state                     = state_.load(std::memory_order_relaxed);
    s.total_calls               = total_calls_.load(std::memory_order_relaxed);
    s.successful_calls          = successful_calls_.load(std::memory_order_relaxed);
    s.failed_calls              = failed_calls_.load(std::memory_order_relaxed);
    s.rejected_calls            = rejected_calls_.load(std::memory_order_relaxed);
    s.current_failure_threshold = effective_failure_threshold_;
    s.last_state_change         = last_state_change_;
    return s;
}

void AdaptiveCircuitBreaker::setStateChangeCallback(
    std::function<void(CircuitState, CircuitState)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_change_cb_ = std::move(callback);
}

// static
std::string AdaptiveCircuitBreaker::stateToString(CircuitState state) {
    switch (state) {
        case CircuitState::CLOSED:    return "CLOSED";
        case CircuitState::HALF_OPEN: return "HALF_OPEN";
        case CircuitState::OPEN:      return "OPEN";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Private helpers (called under mutex_)
// ---------------------------------------------------------------------------

void AdaptiveCircuitBreaker::transitionTo(CircuitState new_state) {
    const CircuitState old_state = state_.load(std::memory_order_relaxed);
    if (old_state == new_state) {
        return;
    }

    state_.store(new_state, std::memory_order_release);
    last_state_change_ = std::chrono::steady_clock::now();

    if (new_state == CircuitState::OPEN) {
        open_timestamp_ = last_state_change_;
        half_open_successes_ = 0;
    } else if (new_state == CircuitState::HALF_OPEN) {
        half_open_successes_ = 0;
    } else if (new_state == CircuitState::CLOSED) {
        consecutive_failures_ = 0;
    }

    if (state_change_cb_) {
        state_change_cb_(old_state, new_state);
    }
}

void AdaptiveCircuitBreaker::adaptThresholdOnTrip() {
    ++consecutive_trip_count_;
    // Only reduce threshold after multiple consecutive trips to avoid
    // thrashing on transient spikes
    if (consecutive_trip_count_ >= 2) {
        const size_t reduction = std::max<size_t>(
            1,
            static_cast<size_t>(
                std::ceil(effective_failure_threshold_ * config_.adaptive_factor)));
        if (effective_failure_threshold_ > reduction) {
            effective_failure_threshold_ -= reduction;
        } else {
            effective_failure_threshold_ = 1; // Minimum threshold
        }
    }
}

} // namespace network
} // namespace themis
