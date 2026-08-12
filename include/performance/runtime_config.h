/**
 * @file runtime_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>

namespace themis {
namespace performance {

/**
 * @brief Runtime configuration for cycle metrics
 * 
 * Singleton class for controlling metrics collection at runtime.
 * Thread-safe for concurrent access.
 */
class RuntimeConfig {
public:
    /**
     * @brief Get singleton instance
     * @return RuntimeConfig instance
     */
    static RuntimeConfig& instance() noexcept {
        static RuntimeConfig instance;
        return instance;
    }

    /**
     * @brief Set sampling rate
     * @param rate Measure every Nth operation (1 = measure all, 100 = measure 1%)
     */
    void setSamplingRate(uint32_t rate) noexcept {
        sampling_rate_.store(rate, std::memory_order_relaxed);
    }

    /**
     * @brief Get current sampling rate
     * @return Sampling rate
     */
    uint32_t getSamplingRate() const noexcept {
        return sampling_rate_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Check if current operation should be measured
     * @return true if should measure
     * 
     * Uses atomic counter for lock-free sampling.
     */
    bool shouldMeasure() noexcept {
        const uint32_t rate = sampling_rate_.load(std::memory_order_relaxed);
        if (rate == 0) return false;
        if (rate == 1) return true;
        
        const uint32_t count = static_cast<uint32_t>(operation_counter_.fetch_add(1, std::memory_order_relaxed));
        return (count % rate) == 0;
    }

    /**
     * @brief Enable measurement for specific operation
     * @param operation_name Operation name
     */
    void enableOperation(const std::string& operation_name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        enabled_operations_.insert(operation_name);
    }

    /**
     * @brief Disable measurement for specific operation
     * @param operation_name Operation name
     */
    void disableOperation(const std::string& operation_name) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        enabled_operations_.erase(operation_name);
    }

    /**
     * @brief Check if operation is enabled
     * @param operation_name Operation name
     * @return true if enabled
     */
    bool isOperationEnabled(const std::string& operation_name) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        // If no specific operations are enabled, all are enabled
        if (enabled_operations_.empty()) return true;
        return enabled_operations_.find(operation_name) != enabled_operations_.end();
    }

    /**
     * @brief Clear all operation filters
     */
    void clearOperationFilters() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        enabled_operations_.clear();
    }

    /**
     * @brief Reset operation counter
     */
    void resetCounter() noexcept {
        operation_counter_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Get total operation count
     * @return Total operations counted
     */
    uint64_t getOperationCount() const noexcept {
        return operation_counter_.load(std::memory_order_relaxed);
    }

private:
    RuntimeConfig() noexcept
        : sampling_rate_(1), operation_counter_(0) {}

    ~RuntimeConfig() = default;

    // Non-copyable, non-movable
    RuntimeConfig(const RuntimeConfig&) = delete;
    RuntimeConfig& operator=(const RuntimeConfig&) = delete;

    std::atomic<uint32_t> sampling_rate_;
    std::atomic<uint64_t> operation_counter_;
    
    mutable std::shared_mutex mutex_;
    std::unordered_set<std::string> enabled_operations_;
};

} // namespace performance
} // namespace themis
