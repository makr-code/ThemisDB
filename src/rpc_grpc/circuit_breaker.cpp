// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file circuit_breaker.cpp
 * @brief Plugin-level circuit breaker implementation.
 *
 * @see include/rpc_grpc/circuit_breaker.h for full API documentation.
 */

#include "rpc_grpc/circuit_breaker.h"

#include <stdexcept>
#include <utility>

namespace themis {
namespace rpc {

// ============================================================================
// Construction / move
// ============================================================================

CircuitBreaker::CircuitBreaker(CircuitBreakerConfig config)
    : config_(std::move(config)) {}

CircuitBreaker::CircuitBreaker(std::string name, CircuitBreakerConfig config)
    : config_(std::move(config)) {
    config_.name = std::move(name);
}

CircuitBreaker::CircuitBreaker(CircuitBreaker&& other) noexcept {
    std::lock_guard<std::mutex> lk(other.mutex_);
    config_               = std::move(other.config_);
    state_                = other.state_;
    consecutive_failures_ = other.consecutive_failures_;
    consecutive_successes_= other.consecutive_successes_;
    total_calls_          = other.total_calls_;
    successful_calls_     = other.successful_calls_;
    failed_calls_         = other.failed_calls_;
    rejected_calls_       = other.rejected_calls_;
    state_transitions_    = other.state_transitions_;
    open_since_           = other.open_since_;
    probe_in_flight_      = other.probe_in_flight_;
    transition_cb_        = std::move(other.transition_cb_);
}

CircuitBreaker& CircuitBreaker::operator=(CircuitBreaker&& other) noexcept {
    if (this == &other) return *this;
    // Lock both to avoid TOCTOU during move.
    std::lock_guard<std::mutex> lk_self(mutex_);
    std::lock_guard<std::mutex> lk_other(other.mutex_);
    config_               = std::move(other.config_);
    state_                = other.state_;
    consecutive_failures_ = other.consecutive_failures_;
    consecutive_successes_= other.consecutive_successes_;
    total_calls_          = other.total_calls_;
    successful_calls_     = other.successful_calls_;
    failed_calls_         = other.failed_calls_;
    rejected_calls_       = other.rejected_calls_;
    state_transitions_    = other.state_transitions_;
    open_since_           = other.open_since_;
    probe_in_flight_      = other.probe_in_flight_;
    transition_cb_        = std::move(other.transition_cb_);
    return *this;
}

// ============================================================================
// Core API
// ============================================================================

bool CircuitBreaker::allowRequest() {
    std::lock_guard<std::mutex> lk(mutex_);
    ++total_calls_;

    switch (state_) {
        case CircuitState::kClosed:
            return true;

        case CircuitState::kOpen: {
            const auto elapsed = std::chrono::steady_clock::now() - open_since_;
            if (elapsed >= config_.recovery_window) {
                // Recovery window elapsed — admit a single probe.
                transitionTo(CircuitState::kHalfOpen);
                probe_in_flight_ = true;
                return true;
            }
            ++rejected_calls_;
            return false;
        }

        case CircuitState::kHalfOpen:
            if (!probe_in_flight_) {
                // No probe in-flight (previous probe settled) — allow another.
                probe_in_flight_ = true;
                return true;
            }
            // Probe already in-flight; reject concurrent callers.
            ++rejected_calls_;
            return false;
    }

    // Unreachable; guard against UB if enum is extended.
    ++rejected_calls_;
    return false;
}

void CircuitBreaker::recordResult(bool success) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (success) {
        ++successful_calls_;
    } else {
        ++failed_calls_;
    }

    switch (state_) {
        case CircuitState::kClosed:
            if (success) {
                consecutive_failures_ = 0;
            } else {
                ++consecutive_failures_;
                if (consecutive_failures_ >= config_.failure_threshold &&
                    total_calls_ >= config_.min_calls_in_window) {
                    transitionTo(CircuitState::kOpen);
                    open_since_ = std::chrono::steady_clock::now();
                }
            }
            break;

        case CircuitState::kHalfOpen:
            probe_in_flight_ = false;
            if (success) {
                ++consecutive_successes_;
                if (consecutive_successes_ >= config_.half_open_success_threshold) {
                    consecutive_failures_  = 0;
                    consecutive_successes_ = 0;
                    transitionTo(CircuitState::kClosed);
                }
            } else {
                consecutive_successes_ = 0;
                transitionTo(CircuitState::kOpen);
                open_since_ = std::chrono::steady_clock::now();
            }
            break;

        case CircuitState::kOpen:
            // recordResult() should not be called when the circuit is OPEN
            // (allowRequest() would have returned false).  Ignore gracefully.
            break;
    }
}

void CircuitBreaker::forceState(CircuitState state) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (state == CircuitState::kOpen) {
        open_since_ = std::chrono::steady_clock::now();
    }
    probe_in_flight_      = false;
    consecutive_failures_ = 0;
    consecutive_successes_= 0;
    transitionTo(state);
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lk(mutex_);
    consecutive_failures_  = 0;
    consecutive_successes_ = 0;
    probe_in_flight_       = false;
    transitionTo(CircuitState::kClosed);
}

// ============================================================================
// Observability
// ============================================================================

CircuitState CircuitBreaker::state() const noexcept {
    // state_ is an enum, not an atomic.  We acquire the mutex for a consistent
    // read even though a single-byte read would likely be torn-free in practice.
    std::lock_guard<std::mutex> lk(mutex_);
    return state_;
}

CircuitBreakerStats CircuitBreaker::stats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    CircuitBreakerStats s;
    s.state             = state_;
    s.total_calls       = total_calls_;
    s.successful_calls  = successful_calls_;
    s.failed_calls      = failed_calls_;
    s.rejected_calls    = rejected_calls_;
    s.state_transitions = state_transitions_;
    s.name              = config_.name;
    return s;
}

void CircuitBreaker::setTransitionCallback(
    std::function<void(CircuitState, CircuitState, const std::string&)> cb) {
    std::lock_guard<std::mutex> lk(mutex_);
    transition_cb_ = std::move(cb);
}

// ============================================================================
// Private helpers
// ============================================================================

void CircuitBreaker::transitionTo(CircuitState next_state) {
    // Caller must hold mutex_.
    if (next_state == state_) return;
    const CircuitState prev = state_;
    state_ = next_state;
    ++state_transitions_;
    if (transition_cb_) {
        transition_cb_(prev, next_state, config_.name);
    }
}

}  // namespace rpc
}  // namespace themis
