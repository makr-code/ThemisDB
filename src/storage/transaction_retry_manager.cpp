/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_retry_manager.cpp                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:51:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright (c) 2024 ThemisDB
// Licensed under the MIT License

#include "storage/transaction_retry_manager.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <thread>
#include <cctype>

namespace themisdb {
namespace storage {

// Maximum allowed jitter_factor value: keeps the uniform_real_distribution
// lower bound strictly positive (1.0 - MAX_JITTER_FACTOR > 0).
static constexpr double MAX_JITTER_FACTOR = 0.999;

TransactionRetryManager::TransactionRetryManager(const TransactionRetryConfig& config)
    : config_(config),
      rng_(std::random_device{}()),
      jitter_dist_(1.0 - std::min(config.jitter_factor, MAX_JITTER_FACTOR),
                   1.0 + std::min(config.jitter_factor, MAX_JITTER_FACTOR)) {
}

TransactionRetryManager::~TransactionRetryManager() = default;

RetryStatistics TransactionRetryManager::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(stats_mutex_);
    return stats_;
}

CircuitState TransactionRetryManager::getCircuitState() const {
    std::lock_guard<std::mutex> lock(circuit_mutex_);
    
    // Check if circuit should auto-reset
    if (circuit_state_ == CircuitState::CIRCUIT_OPEN) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - circuit_opened_time_).count());
        
        if (elapsed >= config_.reset_timeout_ms) {
            // Time to try again
            transitionCircuitState(CircuitState::DEGRADED);
        }
    }
    
    return circuit_state_;
}

void TransactionRetryManager::resetStatistics() {
    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    stats_ = RetryStatistics();
}

void TransactionRetryManager::setAlertCallback(AlertCallback callback) {
    alert_callback_ = std::move(callback);
}

ErrorType TransactionRetryManager::classifyError(const std::string& error_message) {
    // Convert to lowercase for case-insensitive matching
    std::string lower_msg = error_message;
    std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Retryable errors
    if (lower_msg.find("conflict") != std::string::npos ||
        lower_msg.find("write conflict") != std::string::npos ||
        lower_msg.find("concurrent") != std::string::npos ||
        lower_msg.find("serialization failure") != std::string::npos ||
        lower_msg.find("transaction must be retried") != std::string::npos) {
        return ErrorType::WRITE_CONFLICT;
    }
    
    if (lower_msg.find("timeout") != std::string::npos ||
        lower_msg.find("timed out") != std::string::npos ||
        lower_msg.find("deadline") != std::string::npos) {
        return ErrorType::TIMEOUT;
    }
    
    if (lower_msg.find("network") != std::string::npos ||
        lower_msg.find("connection") != std::string::npos ||
        lower_msg.find("socket") != std::string::npos ||
        lower_msg.find("disconnected") != std::string::npos) {
        return ErrorType::NETWORK_ERROR;
    }
    
    if (lower_msg.find("resource") != std::string::npos ||
        lower_msg.find("exhausted") != std::string::npos ||
        lower_msg.find("limit") != std::string::npos ||
        lower_msg.find("quota") != std::string::npos) {
        return ErrorType::RESOURCE_EXHAUSTED;
    }
    
    if (lower_msg.find("unavailable") != std::string::npos ||
        lower_msg.find("service down") != std::string::npos ||
        lower_msg.find("temporarily") != std::string::npos) {
        return ErrorType::SERVICE_UNAVAILABLE;
    }
    
    // Non-retryable errors
    if (lower_msg.find("constraint") != std::string::npos ||
        lower_msg.find("unique") != std::string::npos ||
        lower_msg.find("foreign key") != std::string::npos ||
        lower_msg.find("integrity") != std::string::npos) {
        return ErrorType::CONSTRAINT_VIOLATION;
    }
    
    if (lower_msg.find("invalid") != std::string::npos ||
        lower_msg.find("bad") != std::string::npos ||
        lower_msg.find("malformed") != std::string::npos) {
        return ErrorType::INVALID_ARGUMENT;
    }
    
    if (lower_msg.find("not found") != std::string::npos ||
        lower_msg.find("does not exist") != std::string::npos ||
        lower_msg.find("missing") != std::string::npos) {
        return ErrorType::NOT_FOUND;
    }
    
    if (lower_msg.find("permission") != std::string::npos ||
        lower_msg.find("denied") != std::string::npos ||
        lower_msg.find("unauthorized") != std::string::npos ||
        lower_msg.find("forbidden") != std::string::npos) {
        return ErrorType::PERMISSION_DENIED;
    }
    
    if (lower_msg.find("corrupt") != std::string::npos ||
        lower_msg.find("checksum") != std::string::npos ||
        lower_msg.find("invalid data") != std::string::npos) {
        return ErrorType::DATA_CORRUPTION;
    }
    
    return ErrorType::UNKNOWN;
}

bool TransactionRetryManager::isRetryable(ErrorType error_type) {
    switch (error_type) {
        // Retryable
        case ErrorType::WRITE_CONFLICT:
        case ErrorType::TIMEOUT:
        case ErrorType::NETWORK_ERROR:
        case ErrorType::RESOURCE_EXHAUSTED:
        case ErrorType::SERVICE_UNAVAILABLE:
            return true;
        
        // Non-retryable
        case ErrorType::CONSTRAINT_VIOLATION:
        case ErrorType::INVALID_ARGUMENT:
        case ErrorType::NOT_FOUND:
        case ErrorType::PERMISSION_DENIED:
        case ErrorType::DATA_CORRUPTION:
        case ErrorType::UNKNOWN:
        default:
            return false;
    }
}

uint32_t TransactionRetryManager::calculateDelay(size_t attempt, const RetryPolicy* policy) {
    uint32_t base_delay = policy && policy->base_delay_ms > 0 
                         ? policy->base_delay_ms 
                         : config_.base_delay_ms;
    
    uint32_t max_delay = policy && policy->max_delay_ms > 0 
                        ? policy->max_delay_ms 
                        : config_.max_delay_ms;
    
    BackoffStrategy strategy = policy ? policy->backoff_strategy : config_.backoff_strategy;
    
    uint32_t delay = 0;
    
    switch (strategy) {
        case BackoffStrategy::EXPONENTIAL: {
            // delay = base * (multiplier ^ attempt), clamped to avoid overflow.
            // Guard against non-finite or negative results from pow() (e.g. when
            // backoff_multiplier is negative or NaN).
            double raw = static_cast<double>(base_delay) *
                         std::pow(config_.backoff_multiplier, attempt);
            if (!std::isfinite(raw) || raw < 0.0) {
                delay = max_delay;
            } else if (raw >= static_cast<double>(max_delay)) {
                delay = max_delay;
            } else {
                delay = static_cast<uint32_t>(raw);
            }
            break;
        }
        
        case BackoffStrategy::LINEAR:
            // delay = base * (attempt + 1)
            delay = base_delay * static_cast<uint32_t>(attempt + 1);
            break;
        
        case BackoffStrategy::FIXED:
            // delay = base (constant)
            delay = base_delay;
            break;
    }
    
    // Cap at max delay
    delay = std::min(delay, max_delay);
    
    // Apply jitter if enabled
    if (config_.enable_jitter) {
        std::lock_guard<std::mutex> lock(rng_mutex_);
        double jitter = jitter_dist_(rng_);
        delay = static_cast<uint32_t>(delay * jitter);
    }
    
    return delay;
}

void TransactionRetryManager::recordSuccess() {
    if (!config_.enable_circuit_breaker) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(circuit_mutex_);
    
    // Reset consecutive failures
    consecutive_failures_ = 0;
    
    // Transition to HEALTHY if we were degraded
    if (circuit_state_ == CircuitState::DEGRADED) {
        transitionCircuitState(CircuitState::HEALTHY);
    }
}

void TransactionRetryManager::recordFailure() {
    if (!config_.enable_circuit_breaker) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(circuit_mutex_);
    
    consecutive_failures_++;
    
    // Determine new state based on failure count
    if (consecutive_failures_ >= config_.failure_threshold) {
        if (circuit_state_ != CircuitState::CIRCUIT_OPEN) {
            circuit_opened_time_ = std::chrono::steady_clock::now();
            transitionCircuitState(CircuitState::CIRCUIT_OPEN);
        }
    } else if (consecutive_failures_ >= 3) {
        if (circuit_state_ == CircuitState::HEALTHY) {
            transitionCircuitState(CircuitState::DEGRADED);
        }
    }
}

bool TransactionRetryManager::isCircuitOpen() const {
    std::lock_guard<std::mutex> lock(circuit_mutex_);
    
    // Check if circuit should auto-reset
    if (circuit_state_ == CircuitState::CIRCUIT_OPEN) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - circuit_opened_time_).count());
        
        if (elapsed >= config_.reset_timeout_ms) {
            // Time to try again
            transitionCircuitState(CircuitState::DEGRADED);
            return false;
        }
        return true;
    }
    
    return false;
}

void TransactionRetryManager::transitionCircuitState(CircuitState new_state) const {
    if (circuit_state_ == new_state) {
        return;
    }
    
    CircuitState old_state = circuit_state_;
    circuit_state_ = new_state;
    
    // Build alert message
    std::ostringstream oss;
    oss << "Circuit breaker state transition: ";
    
    auto state_to_string = [](CircuitState state) -> std::string {
        switch (state) {
            case CircuitState::HEALTHY: return "HEALTHY";
            case CircuitState::DEGRADED: return "DEGRADED";
            case CircuitState::CIRCUIT_OPEN: return "CIRCUIT_OPEN";
            default: return "UNKNOWN";
        }
    };
    
    oss << state_to_string(old_state) << " -> " << state_to_string(new_state);
    oss << " (consecutive_failures: " << consecutive_failures_ << ")";
    
    // Invoke callback if set
    if (alert_callback_) {
        try {
            alert_callback_(new_state, oss.str());
        } catch (const std::exception&) {
            // Ignore callback exceptions
        }
    }
}

}  // namespace storage
}  // namespace themisdb
