/**
 * @file safe_fail.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Safe-Fail Manager
 * =====================
 * Circuit-breaker pattern for GPU→CPU automatic degradation.
 */

#include "themis/gpu/safe_fail.h"
#include <stdexcept>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUSafeFail::GPUSafeFail(const Config &cfg) : cfg_(cfg) {}

void GPUSafeFail::reset(const Config &cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    cfg_                   = cfg;
    state_                 = State::HEALTHY;
    consecutive_failures_  = 0;
    consecutive_successes_ = 0;
    total_failures_        = 0;
    total_operations_      = 0;
    total_fallbacks_       = 0;
    cpu_fallback_active_   = false;
    last_error_.clear();
    last_failure_type_ = FailureType::DEVICE_ERROR;
}

// ============================================================================
// executeWithFallback
// ============================================================================

bool GPUSafeFail::executeWithFallback(std::function<bool()> gpu_op, std::function<bool()> cpu_fallback,
                                      const std::string &op_name) {
    // op_name is reserved for future structured-logging / audit integration.
    // Determine whether we should attempt the GPU at all (lock briefly).
    bool attempt_gpu = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++total_operations_;
        attempt_gpu = (state_ != State::CIRCUIT_OPEN && state_ != State::FAILED);
    }

    // --- Try GPU -----------------------------------------------------------
    if (attempt_gpu && gpu_op) {
        bool gpu_ok = false;
        try {
            gpu_ok = gpu_op();
        } catch (...) {
            gpu_ok = false;
        }

        if (gpu_ok) {
            std::lock_guard<std::mutex> lock(mutex_);
            applySuccess();
            cpu_fallback_active_ = false;
            return true;
        }

        // GPU failed.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            applyFailure();
        }
    }

    // --- CPU fallback ------------------------------------------------------
    if (!cfg_.enable_cpu_fallback || !cpu_fallback) {
        return false;
    }

    bool cpu_ok = false;
    try {
        cpu_ok = cpu_fallback();
    } catch (...) {
        cpu_ok = false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        cpu_fallback_active_ = cpu_ok;
        if (cpu_ok) {
            ++total_fallbacks_;
        }
    }
    return cpu_ok;
}

// ============================================================================
// Manual record methods
// ============================================================================

void GPUSafeFail::recordFailure(FailureType type, const std::string &msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_failure_type_ = type;
    last_error_        = msg;
    ++total_operations_;
    applyFailure();
}

void GPUSafeFail::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++total_operations_;
    applySuccess();
}

// ============================================================================
// Circuit-breaker control
// ============================================================================

bool GPUSafeFail::shouldAttemptGPU() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (state_ != State::CIRCUIT_OPEN && state_ != State::FAILED);
}

bool GPUSafeFail::canResetCircuit() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != State::CIRCUIT_OPEN) {
        return false;
    }
    const auto elapsed = std::chrono::steady_clock::now() - circuit_opened_at_;
    return elapsed >= cfg_.circuit_reset_timeout;
}

void GPUSafeFail::tryResetCircuit() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != State::CIRCUIT_OPEN) {
        return;
    }
    const auto elapsed = std::chrono::steady_clock::now() - circuit_opened_at_;
    if (elapsed >= cfg_.circuit_reset_timeout) {
        // Transition to DEGRADED (half-open): one probe attempt allowed.
        state_                 = State::DEGRADED;
        consecutive_failures_  = 0;
        consecutive_successes_ = 0;
    }
}

void GPUSafeFail::forceHealthy() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_                 = State::HEALTHY;
    consecutive_failures_  = 0;
    consecutive_successes_ = 0;
    cpu_fallback_active_   = false;
    last_error_.clear();
}

void GPUSafeFail::forceFailed(const std::string &reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_               = State::FAILED;
    last_error_          = reason;
    cpu_fallback_active_ = false;
}

// ============================================================================
// Queries
// ============================================================================

bool GPUSafeFail::isHealthy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (state_ == State::HEALTHY || state_ == State::DEGRADED);
}

GPUSafeFail::HealthStatus GPUSafeFail::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    HealthStatus s;
    s.state                 = state_;
    s.consecutive_failures  = consecutive_failures_;
    s.consecutive_successes = consecutive_successes_;
    s.total_failures        = total_failures_;
    s.total_operations      = total_operations_;
    s.total_fallbacks       = total_fallbacks_;
    s.last_error            = last_error_;
    s.last_failure_type     = last_failure_type_;
    s.cpu_fallback_active   = cpu_fallback_active_;
    s.error_rate            = (total_operations_ > 0)
                                  ? (static_cast<float>(total_failures_) / static_cast<float>(total_operations_))
                                  : 0.0f;
    return s;
}

float GPUSafeFail::getErrorRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (total_operations_ == 0) {
        return 0.0f;
    }
    return static_cast<float>(total_failures_) / static_cast<float>(total_operations_);
}

bool GPUSafeFail::checkMemoryAvailable(uint64_t required_bytes, uint64_t available_bytes, uint64_t total_bytes) const {
    if (available_bytes < required_bytes) {
        return false;
    }
    if (total_bytes > 0) {
        const float used_frac = static_cast<float>(total_bytes - available_bytes) / static_cast<float>(total_bytes);
        if (used_frac >= cfg_.oom_threshold) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Private helpers (called under mutex_)
// ============================================================================

void GPUSafeFail::applyFailure() {
    ++total_failures_;
    ++consecutive_failures_;
    consecutive_successes_ = 0;

    if (state_ == State::HEALTHY) {
        state_ = State::DEGRADED;
    }
    if (consecutive_failures_ >= cfg_.failure_threshold) {
        state_             = State::CIRCUIT_OPEN;
        circuit_opened_at_ = std::chrono::steady_clock::now();
    }
}

void GPUSafeFail::applySuccess() {
    ++consecutive_successes_;
    consecutive_failures_ = 0;

    if (state_ == State::DEGRADED && consecutive_successes_ >= cfg_.success_threshold) {
        state_ = State::HEALTHY;
    }
}

} // namespace gpu
} // namespace themis

