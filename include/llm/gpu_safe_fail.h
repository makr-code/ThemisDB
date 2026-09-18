/**
 * @file gpu_safe_fail.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    /**
     * @brief GPUSafe Fail Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GPUSafeFailManager(const Config& config);
    ~GPUSafeFailManager() = default;
    
    // Core safe-fail operations
    bool executeWithFallback(
        std::function<bool()> gpu_operation,
        std::function<bool()> cpu_fallback,
        const std::string& operation_name
    );
    
    /**
     * @brief Record Failure.
     * @param[in] type Input parameter.
     * @param[in] error_message Input parameter.
     */
    void recordFailure(FailureType type, const std::string& error_message);
    
    /**
     * @brief Record Success.
     */
    void recordSuccess();
    
    /**
     * @brief Should Attempt GPU.
     * @return True when the operation succeeds.
     */
    bool shouldAttemptGPU() const;
    
    /**
     * @brief Get Health Status.
     * @return Return value.
     */
    GPUHealthStatus getHealthStatus() const;
    
    /**
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     */
    bool isHealthy() const;
    
    /**
     * @brief Force Healthy.
     */
    void forceHealthy();
    
    /**
     * @brief Force Failed.
     * @param[in] reason Input parameter.
     */
    void forceFailed(const std::string& reason);
    
    /**
     * @brief Can Reset Circuit.
     * @return True when the operation succeeds.
     */
    bool canResetCircuit() const;
    
    /**
     * @brief Try Reset Circuit.
     */
    void tryResetCircuit();
    
    /**
     * @brief Get Error Rate.
     * @return Return value.
     */
    float getErrorRate() const;
    
    /**
     * @brief Check Memory Available.
     * @param[in] required_bytes Input parameter.
     * @param[in] available_bytes Input parameter.
     * @return True when the operation succeeds.
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
    /**
     * @brief Update State.
     */
    void updateState();
    /**
     * @brief Log Degradation.
     * @param[in] reason Input parameter.
     */
    void logDegradation(const std::string& reason);
    /**
     * @brief Log Recovery.
     */
    void logRecovery();
    /**
     * @brief Is Circuit Open.
     * @return True when the operation succeeds.
     */
    bool isCircuitOpen() const;
    /**
     * @brief Open Circuit.
     * @param[in] reason Input parameter.
     */
    void openCircuit(const std::string& reason);
    /**
     * @brief Close Circuit.
     */
    void closeCircuit();
};

class GPUTimeoutGuard {
public:
    GPUTimeoutGuard(std::chrono::seconds timeout, const std::string& operation_name);
    ~GPUTimeoutGuard();
    
    /**
     * @brief Check if operation has timed out
     * @return True when the operation succeeds.
     */
    bool hasTimedOut() const;
    
    /**
     * @brief Cancel the timeout (call when operation completes successfully)
     */
    void cancel();
    
private:
    std::chrono::seconds timeout_;
    std::string operation_name_;
    std::chrono::system_clock::time_point start_time_;
    std::atomic<bool> cancelled_{false};
    mutable std::atomic<bool> timed_out_{false};
};

class MemoryPressureMonitor {
public:
    /**
     * @brief Memory Pressure Monitor.
     * @return Return value.
     */
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
    
    /**
     * @brief Memory Pressure Monitor.
     * @param[in] total_memory_bytes Input parameter.
     * @return Return value.
     */
    explicit MemoryPressureMonitor(size_t total_memory_bytes);
    
    /**
     * @brief Update current memory usage
     * @param[in] used_bytes Input parameter.
     */
    void updateUsage(size_t used_bytes);
    
    /**
     * @brief Get current memory status
     * @return Return value.
     */
    MemoryStatus getStatus() const;
    
    /**
     * @brief Check if allocation would succeed
     * @param[in] bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool canAllocate(size_t bytes) const;
    
    // Get recommended action
    /**
     * @brief Get Recommended Action.
     * @return Return value.
     */
    std::string getRecommendedAction() const;
    
private:
    size_t total_memory_bytes_ = 0;
    std::atomic<size_t> used_memory_bytes_{0};
    // mutex_ removed: used_memory_bytes_ is std::atomic, all other reads/writes
    // use only total_memory_bytes_ (const after construction) or the atomic.
};

} // namespace llm
} // namespace themis
