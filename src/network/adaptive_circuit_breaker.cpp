/**
 * @file adaptive_circuit_breaker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    // Initialise per-error-class effective thresholds from config
    for (const auto& [cls, cls_cfg] : config_.error_class_configs) {
        if (cls_cfg.threshold == 0) {
            throw std::invalid_argument(
                "AdaptiveCircuitBreaker: error_class_configs threshold must be >= 1");
        }
        error_class_effective_thresholds_[cls] = cls_cfg.threshold;
    }
}

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

bool AdaptiveCircuitBreaker::shouldAllow() {
    ++total_calls_;

    // Fast path: CLOSED — no lock needed on the hot path
    if (state_.load(std::memory_order_acquire) == CircuitState::CLOSED) {
        return true;
    }

    // All non-CLOSED states require the lock to avoid races during transitions.
    // We never recurse while holding the lock to prevent deadlocks.
    std::lock_guard<std::mutex> lock(mutex_);
    const CircuitState s = state_.load(std::memory_order_relaxed);

    if (s == CircuitState::CLOSED) {
        // Another thread transitioned to CLOSED between the fast-path check
        // and acquiring the lock.
        return true;
    }

    if (s == CircuitState::OPEN) {
        const auto elapsed = std::chrono::steady_clock::now() - open_timestamp_;
        if (elapsed >= config_.open_timeout) {
            // Timeout elapsed — enter probe window
            transitionTo(CircuitState::HALF_OPEN);
            return true;
        }
        ++rejected_calls_;
        return false;
    }

    // HALF_OPEN: allow probe requests; re-open if the half-open timeout expired
    const auto elapsed = std::chrono::steady_clock::now() - last_state_change_;
    if (elapsed >= config_.half_open_timeout) {
        transitionTo(CircuitState::OPEN);
        ++rejected_calls_;
        return false;
    }
    return true;
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

void AdaptiveCircuitBreaker::recordFailure(const ErrorClass& error_class) {
    // Always apply global accounting first.
    // We duplicate the logic here (instead of calling recordFailure()) so that
    // we hold the lock only once and can check both the global and per-class
    // thresholds atomically.
    ++failed_calls_;

    const CircuitState s = state_.load(std::memory_order_acquire);

    if (s == CircuitState::CLOSED) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.load(std::memory_order_relaxed) != CircuitState::CLOSED) {
            return;
        }

        // Per-class threshold check: trip immediately if the class threshold
        // is reached, before the global counter is checked.
        const bool class_tripped = recordErrorClassFailure(error_class);

        ++consecutive_failures_;
        const bool global_tripped =
            consecutive_failures_ >= effective_failure_threshold_;

        if (class_tripped || global_tripped) {
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
    s.error_class_failures      = error_class_consecutive_failures_;
    return s;
}

void AdaptiveCircuitBreaker::setStateChangeCallback(
    std::function<void(CircuitState, CircuitState)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_change_cb_ = std::move(callback);
}

void AdaptiveCircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    consecutive_failures_        = 0;
    half_open_successes_         = 0;
    consecutive_trip_count_      = 0;
    effective_failure_threshold_ = config_.failure_threshold;
    resetErrorClassCounters();
    transitionTo(CircuitState::CLOSED);
}

void AdaptiveCircuitBreaker::forceOpen() {
    std::lock_guard<std::mutex> lock(mutex_);
    consecutive_failures_ = 0;
    half_open_successes_  = 0;
    transitionTo(CircuitState::OPEN);
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
        resetErrorClassCounters();
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

bool AdaptiveCircuitBreaker::recordErrorClassFailure(const ErrorClass& error_class) {
    auto it = config_.error_class_configs.find(error_class);
    if (it == config_.error_class_configs.end()) {
        // Unknown class — no per-class threshold; global accounting only.
        return false;
    }

    size_t& consecutive = error_class_consecutive_failures_[error_class];
    ++consecutive;

    auto thresh_it = error_class_effective_thresholds_.find(error_class);
    const size_t threshold = (thresh_it != error_class_effective_thresholds_.end())
                              ? thresh_it->second
                              : it->second.threshold;

    if (consecutive >= threshold) {
        consecutive = 0;

        // Adapt per-class threshold if enabled
        if (config_.enable_adaptive_threshold) {
            const double factor = (it->second.adaptive_factor > 0.0)
                                   ? it->second.adaptive_factor
                                   : config_.adaptive_factor;
            size_t& eff_thresh = error_class_effective_thresholds_[error_class];
            const size_t reduction = std::max<size_t>(
                1,
                static_cast<size_t>(std::ceil(eff_thresh * factor)));
            eff_thresh = (eff_thresh > reduction) ? (eff_thresh - reduction) : 1;
        }
        return true;
    }
    return false;
}

void AdaptiveCircuitBreaker::resetErrorClassCounters() {
    error_class_consecutive_failures_.clear();
    // Restore per-class effective thresholds to configured values
    for (const auto& [cls, cls_cfg] : config_.error_class_configs) {
        error_class_effective_thresholds_[cls] = cls_cfg.threshold;
    }
}

} // namespace network
} // namespace themis
