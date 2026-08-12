/**
 * @file gpu_safe_fail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <string>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>

namespace themis {
namespace llm {

/**
 * @brief GPU Safe-Fail Manager
 * 
 * Implements database best-practice safe-fail mechanisms for GPU operations:
 * - Automatic degradation to CPU on GPU failures
 * - OOM (Out of Memory) detection and recovery
 * - GPU operation timeout handling
 * - Circuit breaker pattern for repeated failures
 * - Health monitoring and alerting
 * 
 * Best Practices:
 * - Graceful degradation: Automatically fallback to CPU when GPU fails
 * - Fail-fast: Detect failures quickly and prevent cascading issues
 * - Circuit breaker: Temporarily disable failing components to allow recovery
 * - Health monitoring: Track error rates and trigger alerts
 * - Timeout handling: Prevent hung operations from blocking the system
 */
class GPUSafeFailManager {
public:
    enum class GPUState {
        HEALTHY,        // GPU is operational
        DEGRADED,       // GPU experiencing issues but still usable
        FAILED,         // GPU has failed, using CPU fallback
        CIRCUIT_OPEN    // Circuit breaker open, not attempting GPU operations
    };
    
    enum class FailureType {
        OOM,            // Out of memory
        TIMEOUT,        // Operation timeout
        DEVICE_ERROR,   // Device initialization or communication error
        KERNEL_ERROR,   // Kernel execution error
        MEMORY_ERROR    // Memory allocation or access error
    };
    
    struct Config {
        // Circuit breaker configuration
        size_t failure_threshold = 5;         // Failures before opening circuit
        size_t success_threshold = 3;         // Successes to close circuit
        std::chrono::seconds circuit_reset_timeout{60};  // Time before retry
        
        // Timeout configuration
        std::chrono::seconds gpu_operation_timeout{30};  // Max time for GPU op
        
        // Memory thresholds
        float oom_threshold = 0.95f;  // 95% memory usage triggers OOM warning
        size_t min_free_memory = 512 * 1024 * 1024;  // 512 MB minimum free
        
        // Health check configuration
        std::chrono::seconds health_check_interval{10};  // Health check frequency
        size_t max_error_count = 10;  // Max errors before marking unhealthy
        
        // Fallback configuration
        bool enable_cpu_fallback = true;  // Enable automatic CPU fallback
        bool log_degradation = true;      // Log when degrading to CPU
    };
    
    struct GPUHealthStatus {
        GPUState state;
        size_t consecutive_failures = 0;
        size_t consecutive_successes = 0;
        size_t total_failures = 0;
        size_t total_operations = 0;
        std::chrono::system_clock::time_point last_failure_time;
        std::chrono::system_clock::time_point last_success_time;
        std::chrono::system_clock::time_point circuit_opened_time;
        std::string last_error_message;
        FailureType last_failure_type;
        bool is_cpu_fallback_active = false;
        float error_rate = 0.0f;  // Percentage of operations that failed
    };
    
    GPUSafeFailManager();
    explicit GPUSafeFailManager(const Config& config);
    ~GPUSafeFailManager() = default;
    
    // Core safe-fail operations
    /**
     * @brief Execute a GPU operation with automatic fallback
     * 
     * Wraps a GPU operation with timeout, error handling, and automatic
     * CPU fallback. Implements circuit breaker pattern.
     * 
     * @param gpu_operation Function to execute on GPU
     * @param cpu_fallback Function to execute on CPU if GPU fails
     * @param operation_name Name for logging
     * @return true if operation succeeded (on GPU or CPU), false otherwise
     */
    bool executeWithFallback(
        std::function<bool()> gpu_operation,
        std::function<bool()> cpu_fallback,
        const std::string& operation_name
    );
    
    /**
     * @brief Record a GPU operation failure
     * 
     * Updates circuit breaker state and health metrics
     */
    void recordFailure(FailureType type, const std::string& error_message);
    
    /**
     * @brief Record a successful GPU operation
     * 
     * Updates circuit breaker state and health metrics
     */
    void recordSuccess();
    
    /**
     * @brief Check if GPU operations should be attempted
     * 
     * Returns false if circuit is open or GPU is in failed state
     */
    bool shouldAttemptGPU() const;
    
    /**
     * @brief Get current GPU health status
     */
    GPUHealthStatus getHealthStatus() const;
    
    /**
     * @brief Check if GPU is healthy enough for operations
     */
    bool isHealthy() const;
    
    /**
     * @brief Force GPU to healthy state (use after manual recovery)
     */
    void forceHealthy();
    
    /**
     * @brief Force GPU to failed state (use for maintenance)
     */
    void forceFailed(const std::string& reason);
    
    /**
     * @brief Check if circuit breaker should reset
     */
    bool canResetCircuit() const;
    
    /**
     * @brief Attempt to reset circuit breaker
     */
    void tryResetCircuit();
    
    /**
     * @brief Get error rate (0.0 - 1.0)
     */
    float getErrorRate() const;
    
    /**
     * @brief Check if memory is available
     * 
     * @param required_bytes Memory required for operation
     * @param available_bytes Currently available memory
     * @return true if allocation should proceed
     */
    bool checkMemoryAvailable(size_t required_bytes, size_t available_bytes) const;
    
private:
    Config config_;
    mutable std::mutex mutex_;
    
    std::atomic<GPUState> current_state_{GPUState::HEALTHY};
    std::atomic<size_t> consecutive_failures_{0};
    std::atomic<size_t> consecutive_successes_{0};
    std::atomic<size_t> total_failures_{0};
    std::atomic<size_t> total_operations_{0};
    
    std::chrono::system_clock::time_point last_failure_time_;
    std::chrono::system_clock::time_point last_success_time_;
    std::chrono::system_clock::time_point circuit_opened_time_;
    std::chrono::system_clock::time_point last_health_check_;
    
    std::string last_error_message_;
    FailureType last_failure_type_ = FailureType::DEVICE_ERROR;
    
    bool is_cpu_fallback_active_ = false;
    
    // Helper methods
    void updateState();
    void logDegradation(const std::string& reason);
    void logRecovery();
    bool isCircuitOpen() const;
    void openCircuit(const std::string& reason);
    void closeCircuit();
};

/**
 * @brief GPU Operation Timeout Guard
 * 
 * RAII-style timeout handler for GPU operations.
 * Launches a watchdog thread that can detect hung operations.
 */
class GPUTimeoutGuard {
public:
    GPUTimeoutGuard(std::chrono::seconds timeout, const std::string& operation_name);
    ~GPUTimeoutGuard();
    
    // Check if operation has timed out
    bool hasTimedOut() const;
    
    // Cancel the timeout (call when operation completes successfully)
    void cancel();
    
private:
    std::chrono::seconds timeout_;
    std::string operation_name_;
    std::chrono::system_clock::time_point start_time_;
    std::atomic<bool> cancelled_{false};
    mutable std::atomic<bool> timed_out_{false};
};

/**
 * @brief Memory Pressure Monitor
 * 
 * Monitors GPU memory usage and triggers warnings/actions
 * before running out of memory completely.
 */
class MemoryPressureMonitor {
public:
    virtual ~MemoryPressureMonitor() = default;
    enum class PressureLevel {
        NORMAL,    // < 70% memory used
        MODERATE,  // 70-85% memory used
        HIGH,      // 85-95% memory used
        CRITICAL   // > 95% memory used
    };
    
    struct MemoryStatus {
        size_t total_bytes = 0;
        size_t used_bytes = 0;
        size_t free_bytes = 0;
        float usage_percent = 0.0f;
        PressureLevel pressure = PressureLevel::NORMAL;
        bool should_trigger_gc = false;    // Should trigger garbage collection
        bool should_block_new = false;     // Should block new allocations
    };
    
    explicit MemoryPressureMonitor(size_t total_memory_bytes);
    
    // Update current memory usage
    void updateUsage(size_t used_bytes);
    
    // Get current memory status
    MemoryStatus getStatus() const;
    
    // Check if allocation would succeed
    bool canAllocate(size_t bytes) const;
    
    // Get recommended action
    std::string getRecommendedAction() const;
    
private:
    size_t total_memory_bytes_ = 0;
    std::atomic<size_t> used_memory_bytes_{0};
    // mutex_ removed: used_memory_bytes_ is std::atomic, all other reads/writes
    // use only total_memory_bytes_ (const after construction) or the atomic.
};

} // namespace llm
} // namespace themis
