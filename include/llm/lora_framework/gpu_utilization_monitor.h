/**
 * @file gpu_utilization_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <string>
#include <memory>

namespace themis {
namespace llm {
namespace lora {

class GPUUtilizationMonitor {
public:
    struct Metrics {
        float gpu_utilization_pct = 0.0f;    // % of time GPU executing kernels
        float memory_utilization_pct = 0.0f; // % of VRAM used
        float compute_throughput_tflops = 0.0f;
        float memory_bandwidth_gb_s = 0.0f;
        size_t active_sms = 0;               // Active streaming multiprocessors
        float sm_occupancy_pct = 0.0f;       // SM occupancy
        
        // Timestamp
        long long timestamp_ms = 0;
    };
    
    /**
     * @brief GPUUtilization Monitor.
     * @param[in] device Input parameter.
     * @return Return value.
     */
    explicit GPUUtilizationMonitor(const Device& device);
    
    ~GPUUtilizationMonitor();
    
    /**
     * @brief Query Metrics.
     * @return Return value.
     */
    Metrics queryMetrics();
    
    bool isUnderutilized(float threshold = 0.8f) const;
    
    /**
     * @brief Get Optimization Recommendations.
     * @return Return value.
     */
    std::vector<std::string> getOptimizationRecommendations() const;
    
    Metrics getAverageMetrics(size_t num_samples = 10) const;
    
    bool isAvailable() const { return is_available_; }
    
    /**
     * @brief Get Device Info.
     * @return Return value.
     */
    std::string getDeviceInfo() const;
    
private:
    Device device_;
    bool is_available_ = false;
    
    // Metrics history
    std::vector<Metrics> metrics_history_;
    mutable Metrics last_metrics_;
    
    // Backend-specific handles (opaque types for type safety)
#ifdef THEMIS_ENABLE_CUDA
    struct nvmlDevice_st* nvml_device_ = nullptr;  // NVIDIA: nvmlDevice_t (opaque pointer)
#else
    void* nvml_device_ = nullptr;  // Fallback when CUDA not available
#endif
#ifdef THEMIS_ENABLE_HIP
    uint32_t rocm_device_index_ = 0;  // AMD: device index (not pointer)
#else
    void* rocm_device_ = nullptr;  // Fallback when HIP not available
#endif
    
    // Initialization
    /**
     * @brief Initialize NVML.
     * @return True when the operation succeeds.
     */
    bool initializeNVML();
    /**
     * @brief Initialize ROCm.
     * @return True when the operation succeeds.
     */
    bool initializeROCm();
    /**
     * @brief Initialize Vulkan.
     * @return True when the operation succeeds.
     */
    bool initializeVulkan();
    /**
     * @brief Initialize Direct X.
     * @return True when the operation succeeds.
     */
    bool initializeDirectX();
    
    // Cleanup
    /**
     * @brief Shutdown NVML.
     */
    void shutdownNVML();
    /**
     * @brief Shutdown ROCm.
     */
    void shutdownROCm();
    /**
     * @brief Shutdown Vulkan.
     */
    void shutdownVulkan();
    /**
     * @brief Shutdown Direct X.
     */
    void shutdownDirectX();
    
    // Query methods
    /**
     * @brief Query NVML.
     * @return Return value.
     */
    Metrics queryNVML();
    /**
     * @brief Query ROCm.
     * @return Return value.
     */
    Metrics queryROCm();
    /**
     * @brief Query Vulkan.
     * @return Return value.
     */
    Metrics queryVulkan();
    /**
     * @brief Query Direct X.
     * @return Return value.
     */
    Metrics queryDirectX();
    
    /**
     * @brief Fallback when monitoring not available
     * @return Return value.
     */
    Metrics getFallbackMetrics() const;
};

} // namespace lora
} // namespace llm
} // namespace themis
