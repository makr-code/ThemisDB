/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_safe_fail.cpp                                  ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:41:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     478                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/gpu_safe_fail.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <algorithm>

namespace themis {
namespace llm {

// ============================================================================
// GPUSafeFailManager Implementation
// ============================================================================

GPUSafeFailManager::GPUSafeFailManager()
    : GPUSafeFailManager(Config{}) {
}

GPUSafeFailManager::GPUSafeFailManager(const Config& config)
    : config_(config) {
    spdlog::info("GPU Safe-Fail Manager initialized:");
    spdlog::info("  Failure threshold: {}", config_.failure_threshold);
    spdlog::info("  Circuit reset timeout: {}s", config_.circuit_reset_timeout.count());
    spdlog::info("  GPU operation timeout: {}s", config_.gpu_operation_timeout.count());
    spdlog::info("  CPU fallback: {}", config_.enable_cpu_fallback ? "enabled" : "disabled");
}

bool GPUSafeFailManager::executeWithFallback(
    std::function<bool()> gpu_operation,
    std::function<bool()> cpu_fallback,
    const std::string& operation_name
) {
    total_operations_++;
    
    // Check if we should even attempt GPU operation
    if (!shouldAttemptGPU()) {
        if (config_.enable_cpu_fallback && cpu_fallback) {
            spdlog::debug("GPU circuit open for '{}', using CPU fallback", operation_name);
            is_cpu_fallback_active_ = true;
            bool result = cpu_fallback();
            if (result) {
                recordSuccess();  // CPU fallback success
            }
            return result;
        } else {
            spdlog::error("GPU unavailable for '{}' and no CPU fallback provided", operation_name);
            return false;
        }
    }
    
    // Try GPU operation with timeout
    try {
        GPUTimeoutGuard timeout_guard(config_.gpu_operation_timeout, operation_name);
        
        bool result = gpu_operation();
        
        if (timeout_guard.hasTimedOut()) {
            recordFailure(FailureType::TIMEOUT, 
                         "GPU operation '" + operation_name + "' timed out");
            
            // Try CPU fallback
            if (config_.enable_cpu_fallback && cpu_fallback) {
                spdlog::warn("GPU operation '{}' timed out, trying CPU fallback", operation_name);
                is_cpu_fallback_active_ = true;
                return cpu_fallback();
            }
            return false;
        }
        
        timeout_guard.cancel();
        
        if (result) {
            recordSuccess();
            is_cpu_fallback_active_ = false;
            return true;
        } else {
            recordFailure(FailureType::KERNEL_ERROR,
                         "GPU operation '" + operation_name + "' returned false");
            
            // Try CPU fallback
            if (config_.enable_cpu_fallback && cpu_fallback) {
                spdlog::warn("GPU operation '{}' failed, trying CPU fallback", operation_name);
                is_cpu_fallback_active_ = true;
                return cpu_fallback();
            }
            return false;
        }
    } catch (const std::exception& e) {
        recordFailure(FailureType::DEVICE_ERROR,
                     "GPU operation '" + operation_name + "' threw exception: " + e.what());
        
        // Try CPU fallback
        if (config_.enable_cpu_fallback && cpu_fallback) {
            spdlog::warn("GPU operation '{}' threw exception, trying CPU fallback", operation_name);
            is_cpu_fallback_active_ = true;
            try {
                return cpu_fallback();
            } catch (const std::exception& fb_e) {
                spdlog::error("CPU fallback also failed for '{}': {}", operation_name, fb_e.what());
                return false;
            }
        }
        return false;
    }
}

void GPUSafeFailManager::recordFailure(FailureType type, const std::string& error_message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_failures_++;
    consecutive_failures_++;
    consecutive_successes_ = 0;
    
    last_failure_time_ = std::chrono::system_clock::now();
    last_failure_type_ = type;
    last_error_message_ = error_message;
    
    spdlog::warn("GPU operation failed ({}): {}", 
                 static_cast<int>(type), error_message);
    
    updateState();
}

void GPUSafeFailManager::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    consecutive_successes_++;
    consecutive_failures_ = 0;
    
    last_success_time_ = std::chrono::system_clock::now();
    
    updateState();
}

bool GPUSafeFailManager::shouldAttemptGPU() const {
    GPUState state = current_state_.load();
    return state != GPUState::CIRCUIT_OPEN && state != GPUState::FAILED;
}

GPUSafeFailManager::GPUHealthStatus GPUSafeFailManager::getHealthStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GPUHealthStatus status;
    status.state = current_state_.load();
    status.consecutive_failures = consecutive_failures_.load();
    status.consecutive_successes = consecutive_successes_.load();
    status.total_failures = total_failures_.load();
    status.total_operations = total_operations_.load();
    status.last_failure_time = last_failure_time_;
    status.last_success_time = last_success_time_;
    status.circuit_opened_time = circuit_opened_time_;
    status.last_error_message = last_error_message_;
    status.last_failure_type = last_failure_type_;
    status.is_cpu_fallback_active = is_cpu_fallback_active_;
    
    // Calculate error rate
    if (status.total_operations > 0) {
        status.error_rate = static_cast<float>(status.total_failures) / 
                           static_cast<float>(status.total_operations);
    }
    
    return status;
}

bool GPUSafeFailManager::isHealthy() const {
    GPUState state = current_state_.load();
    return state == GPUState::HEALTHY || state == GPUState::DEGRADED;
}

void GPUSafeFailManager::forceHealthy() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    current_state_ = GPUState::HEALTHY;
    consecutive_failures_ = 0;
    consecutive_successes_ = 0;
    is_cpu_fallback_active_ = false;
    
    spdlog::info("GPU forced to HEALTHY state");
    logRecovery();
}

void GPUSafeFailManager::forceFailed(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    current_state_ = GPUState::FAILED;
    last_error_message_ = reason;
    
    spdlog::warn("GPU forced to FAILED state: {}", reason);
    logDegradation(reason);
}

bool GPUSafeFailManager::canResetCircuit() const {
    if (current_state_.load() != GPUState::CIRCUIT_OPEN) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - circuit_opened_time_);
    
    return elapsed >= config_.circuit_reset_timeout;
}

void GPUSafeFailManager::tryResetCircuit() {
    if (!canResetCircuit()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_state_.load() == GPUState::CIRCUIT_OPEN) {
        // Transition to degraded state for testing
        current_state_ = GPUState::DEGRADED;
        consecutive_failures_ = 0;
        
        spdlog::info("Circuit breaker reset, transitioning to DEGRADED state for testing");
    }
}

float GPUSafeFailManager::getErrorRate() const {
    size_t total = total_operations_.load();
    if (total == 0) return 0.0f;
    
    size_t failures = total_failures_.load();
    return static_cast<float>(failures) / static_cast<float>(total);
}

bool GPUSafeFailManager::checkMemoryAvailable(size_t required_bytes, size_t available_bytes) const {
    if (available_bytes < required_bytes) {
        return false;
    }
    
    // Check if we're approaching OOM threshold
    size_t would_be_used = (config_.max_error_count - available_bytes) + required_bytes;
    float usage_ratio = static_cast<float>(would_be_used) / config_.max_error_count;
    
    if (usage_ratio > config_.oom_threshold) {
        spdlog::warn("Memory allocation would exceed OOM threshold: {:.1f}% usage", 
                     usage_ratio * 100);
        return false;
    }
    
    if (available_bytes - required_bytes < config_.min_free_memory) {
        spdlog::warn("Memory allocation would leave insufficient free memory: {} bytes free",
                     available_bytes - required_bytes);
        return false;
    }
    
    return true;
}

void GPUSafeFailManager::updateState() {
    // State machine for GPU health
    GPUState current = current_state_.load();
    size_t failures = consecutive_failures_.load();
    size_t successes = consecutive_successes_.load();
    
    switch (current) {
        case GPUState::HEALTHY:
            if (failures >= config_.failure_threshold) {
                openCircuit("Too many consecutive failures");
            } else if (failures > 0) {
                current_state_ = GPUState::DEGRADED;
                spdlog::info("GPU transitioned to DEGRADED state");
            }
            break;
            
        case GPUState::DEGRADED:
            if (failures >= config_.failure_threshold) {
                openCircuit("Too many consecutive failures in degraded state");
            } else if (successes >= config_.success_threshold) {
                current_state_ = GPUState::HEALTHY;
                spdlog::info("GPU recovered to HEALTHY state");
                logRecovery();
            }
            break;
            
        case GPUState::CIRCUIT_OPEN:
            // Circuit can only be reset by tryResetCircuit()
            break;
            
        case GPUState::FAILED:
            // Failed state requires manual intervention (forceHealthy)
            break;
    }
}

void GPUSafeFailManager::logDegradation(const std::string& reason) {
    if (config_.log_degradation) {
        spdlog::warn("=== GPU DEGRADATION DETECTED ===");
        spdlog::warn("Reason: {}", reason);
        spdlog::warn("Circuit breaker: {}", 
                     current_state_.load() == GPUState::CIRCUIT_OPEN ? "OPEN" : "CLOSED");
        spdlog::warn("CPU fallback: {}", is_cpu_fallback_active_ ? "ACTIVE" : "INACTIVE");
        spdlog::warn("Error rate: {:.1f}%", getErrorRate() * 100);
        spdlog::warn("================================");
    }
}

void GPUSafeFailManager::logRecovery() {
    spdlog::info("=== GPU RECOVERED ===");
    spdlog::info("Error rate: {:.1f}%", getErrorRate() * 100);
    spdlog::info("Total operations: {}", total_operations_.load());
    spdlog::info("====================");
}

bool GPUSafeFailManager::isCircuitOpen() const {
    return current_state_.load() == GPUState::CIRCUIT_OPEN;
}

void GPUSafeFailManager::openCircuit(const std::string& reason) {
    current_state_ = GPUState::CIRCUIT_OPEN;
    circuit_opened_time_ = std::chrono::system_clock::now();
    
    spdlog::error("Circuit breaker OPENED: {}", reason);
    logDegradation(reason);
}

void GPUSafeFailManager::closeCircuit() {
    if (current_state_.load() == GPUState::CIRCUIT_OPEN) {
        current_state_ = GPUState::HEALTHY;
        consecutive_failures_ = 0;
        
        spdlog::info("Circuit breaker CLOSED");
        logRecovery();
    }
}

// ============================================================================
// GPUTimeoutGuard Implementation
// ============================================================================

GPUTimeoutGuard::GPUTimeoutGuard(std::chrono::seconds timeout, const std::string& operation_name)
    : timeout_(timeout)
    , operation_name_(operation_name)
    , start_time_(std::chrono::system_clock::now()) {
}

GPUTimeoutGuard::~GPUTimeoutGuard() {
    // Nothing to clean up
}

bool GPUTimeoutGuard::hasTimedOut() const {
    if (cancelled_.load()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    
    if (elapsed >= timeout_) {
        timed_out_ = true;
        spdlog::error("GPU operation '{}' timed out after {}s", 
                     operation_name_, elapsed.count());
        return true;
    }
    
    return false;
}

void GPUTimeoutGuard::cancel() {
    cancelled_ = true;
}

// ============================================================================
// MemoryPressureMonitor Implementation
// ============================================================================

MemoryPressureMonitor::MemoryPressureMonitor(size_t total_memory_bytes)
    : total_memory_bytes_(total_memory_bytes) {
    spdlog::info("Memory Pressure Monitor initialized with {} GB total memory",
                 total_memory_bytes / (1024.0 * 1024 * 1024));
}

void MemoryPressureMonitor::updateUsage(size_t used_bytes) {
    used_memory_bytes_ = used_bytes;
}

MemoryPressureMonitor::MemoryStatus MemoryPressureMonitor::getStatus() const {
    MemoryStatus status;
    status.total_bytes = total_memory_bytes_;
    status.used_bytes = used_memory_bytes_.load();
    status.free_bytes = status.total_bytes > status.used_bytes ? 
                        status.total_bytes - status.used_bytes : 0;
    
    status.usage_percent = static_cast<float>(status.used_bytes) / 
                          static_cast<float>(status.total_bytes);
    
    // Determine pressure level
    if (status.usage_percent < 0.70f) {
        status.pressure = PressureLevel::NORMAL;
        status.should_trigger_gc = false;
        status.should_block_new = false;
    } else if (status.usage_percent < 0.85f) {
        status.pressure = PressureLevel::MODERATE;
        status.should_trigger_gc = false;
        status.should_block_new = false;
    } else if (status.usage_percent < 0.95f) {
        status.pressure = PressureLevel::HIGH;
        status.should_trigger_gc = true;
        status.should_block_new = false;
    } else {
        status.pressure = PressureLevel::CRITICAL;
        status.should_trigger_gc = true;
        status.should_block_new = true;
    }
    
    return status;
}

bool MemoryPressureMonitor::canAllocate(size_t bytes) const {
    MemoryStatus status = getStatus();
    
    if (status.should_block_new) {
        spdlog::error("Memory pressure CRITICAL - blocking new allocation of {} MB",
                     bytes / (1024.0 * 1024));
        return false;
    }
    
    if (status.free_bytes < bytes) {
        spdlog::error("Insufficient free memory: {} MB requested, {} MB available",
                     bytes / (1024.0 * 1024),
                     status.free_bytes / (1024.0 * 1024));
        return false;
    }
    
    // Check if allocation would push us into critical zone
    size_t would_be_used = status.used_bytes + bytes;
    float would_be_usage = static_cast<float>(would_be_used) / 
                          static_cast<float>(status.total_bytes);
    
    if (would_be_usage > 0.95f) {
        spdlog::warn("Allocation would push memory to {:.1f}% usage", would_be_usage * 100);
        return false;
    }
    
    return true;
}

std::string MemoryPressureMonitor::getRecommendedAction() const {
    MemoryStatus status = getStatus();
    
    switch (status.pressure) {
        case PressureLevel::NORMAL:
            return "No action needed";
        case PressureLevel::MODERATE:
            return "Monitor memory usage";
        case PressureLevel::HIGH:
            return "Consider freeing unused models or reducing batch size";
        case PressureLevel::CRITICAL:
            return "URGENT: Free memory immediately or system may fail";
        default:
            return "Unknown";
    }
}

} // namespace llm
} // namespace themis
