/**
 * @file circuit_breaker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/circuit_breaker.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace themis::sharding {

CircuitBreaker::CircuitBreaker(const Config& config)
    : config_(config) {
}

bool CircuitBreaker::allowRequest() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    State current_state = state_.load();
    
    switch (current_state) {
        case State::CLOSED:
            // Allow all requests in CLOSED state
            return true;
            
        case State::OPEN:
            // Check if timeout has elapsed
            if (isTimeoutElapsed()) {
                // Transition to HALF_OPEN for recovery testing
                transitionTo(State::HALF_OPEN);
                return true; // Allow first test request
            }
            // Circuit still open, block request
            return false;
            
        case State::HALF_OPEN:
            // Allow limited requests for recovery testing
            return true;
    }
    
    return false;
}

void CircuitBreaker::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    State current_state = state_.load();
    
    if (current_state == State::HALF_OPEN) {
        half_open_successes_++;
        
        // If enough successes in HALF_OPEN, close the circuit
        if (half_open_successes_ >= config_.success_threshold) {
            transitionTo(State::CLOSED);
            half_open_successes_ = 0;
            consecutive_failures_ = 0;
            failure_timestamps_.clear();
        }
    } else if (current_state == State::CLOSED) {
        // Reset failure counter on success
        consecutive_failures_ = 0;
    }
}

void CircuitBreaker::recordFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    State current_state = state_.load();
    auto now = std::chrono::steady_clock::now();
    
    // Add failure timestamp
    failure_timestamps_.push_back(now);
    consecutive_failures_++;
    
    // Clean up old failures outside the rolling window
    cleanupOldFailures();
    
    size_t failures_in_window = getCurrentFailureCount();
    
    if (current_state == State::CLOSED) {
        // Check if failures exceed threshold
        if (failures_in_window >= config_.failure_threshold) {
            transitionTo(State::OPEN);
            open_timestamp_ = now;
        }
    } else if (current_state == State::HALF_OPEN) {
        // Any failure in HALF_OPEN state reopens the circuit
        transitionTo(State::OPEN);
        open_timestamp_ = now;
        half_open_successes_ = 0;
    }
}

CircuitBreaker::State CircuitBreaker::getState() const {
    return state_.load();
}

size_t CircuitBreaker::getFailureCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getCurrentFailureCount();
}

size_t CircuitBreaker::getSuccessCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return half_open_successes_;
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    transitionTo(State::CLOSED);
    failure_timestamps_.clear();
    consecutive_failures_ = 0;
    half_open_successes_ = 0;
}

void CircuitBreaker::forceOpen() {
    std::lock_guard<std::mutex> lock(mutex_);
    transitionTo(State::OPEN);
    open_timestamp_ = std::chrono::steady_clock::now();
}

std::string CircuitBreaker::stateToString(State state) {
    switch (state) {
        case State::CLOSED: return "CLOSED";
        case State::OPEN: return "OPEN";
        case State::HALF_OPEN: return "HALF_OPEN";
    }
    return "UNKNOWN";
}

void CircuitBreaker::transitionTo(State new_state) {
    State old_state = state_.load();
    if (old_state != new_state) {
        state_.store(new_state);
        spdlog::info("CircuitBreaker: {} → {}", stateToString(old_state), stateToString(new_state));
    }
}

bool CircuitBreaker::isTimeoutElapsed() const {
    if (state_.load() != State::OPEN) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - open_timestamp_
    );
    
    return elapsed >= config_.timeout;
}

void CircuitBreaker::cleanupOldFailures() {
    auto now = std::chrono::steady_clock::now();
    auto window_start = now - config_.failure_window;
    
    // Remove failures older than the rolling window
    failure_timestamps_.erase(
        std::remove_if(
            failure_timestamps_.begin(),
            failure_timestamps_.end(),
            [window_start](const auto& timestamp) {
                return timestamp < window_start;
            }
        ),
        failure_timestamps_.end()
    );
}

size_t CircuitBreaker::getCurrentFailureCount() const {
    return failure_timestamps_.size();
}

// ============================================================================
// CircuitBreakerManager Implementation
// ============================================================================

CircuitBreaker& CircuitBreakerManager::getCircuitBreaker(
    const std::string& shard_id,
    const CircuitBreaker::Config& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = circuit_breakers_.find(shard_id);
    if (it != circuit_breakers_.end()) {
        return *it->second;
    }
    
    // Create new circuit breaker
    auto cb = std::make_unique<CircuitBreaker>(config);
    auto& ref = *cb;
    circuit_breakers_[shard_id] = std::move(cb);
    
    return ref;
}

bool CircuitBreakerManager::hasCircuitBreaker(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return circuit_breakers_.find(shard_id) != circuit_breakers_.end();
}

void CircuitBreakerManager::removeCircuitBreaker(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    circuit_breakers_.erase(shard_id);
}

void CircuitBreakerManager::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [shard_id, cb] : circuit_breakers_) {
        cb->reset();
    }
}

std::vector<std::string> CircuitBreakerManager::getAllShardIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> shard_ids;
    shard_ids.reserve(circuit_breakers_.size());
    
    for (const auto& [shard_id, _] : circuit_breakers_) {
        shard_ids.push_back(shard_id);
    }
    
    return shard_ids;
}

CircuitBreakerManager::StateCount CircuitBreakerManager::getStateCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    StateCount count;
    
    for (const auto& [shard_id, cb] : circuit_breakers_) {
        switch (cb->getState()) {
            case CircuitBreaker::State::CLOSED:
                count.closed++;
                break;
            case CircuitBreaker::State::OPEN:
                count.open++;
                break;
            case CircuitBreaker::State::HALF_OPEN:
                count.half_open++;
                break;
        }
    }
    
    return count;
}

} // namespace themis::sharding
