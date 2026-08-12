/**
 * @file gpu_utilization_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief GPU utilization monitoring for training optimization
 * 
 * Tracks real-time GPU metrics to identify optimization opportunities:
 * - GPU compute utilization
 * - Memory utilization
 * - Memory bandwidth
 * - SM (Streaming Multiprocessor) occupancy
 * 
 * Provides actionable recommendations for improving GPU utilization.
 */
class GPUUtilizationMonitor {
public:
    /**
     * @brief GPU performance metrics
     */
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
     * @brief Construct GPU utilization monitor
     * @param device Target GPU device to monitor
     */
    explicit GPUUtilizationMonitor(const Device& device);
    
    ~GPUUtilizationMonitor();
    
    /**
     * @brief Query current GPU utilization
     * 
     * Uses backend-specific APIs:
     * - CUDA: NVML (NVIDIA Management Library)
     * - HIP: ROCm SMI
     * - Vulkan: Performance queries
     * - DirectX: D3D12 performance counters
     * 
     * @return Current GPU metrics
     */
    Metrics queryMetrics();
    
    /**
     * @brief Check if GPU is underutilized
     * @param threshold Utilization threshold (default: 0.8 = 80%)
     * @return true if GPU utilization < threshold
     */
    bool isUnderutilized(float threshold = 0.8f) const;
    
    /**
     * @brief Get recommendations for better utilization
     * @return Vector of optimization suggestions
     */
    std::vector<std::string> getOptimizationRecommendations() const;
    
    /**
     * @brief Get average metrics over recent history
     * @param num_samples Number of recent samples to average (default: 10)
     * @return Average metrics
     */
    Metrics getAverageMetrics(size_t num_samples = 10) const;
    
    /**
     * @brief Check if monitoring is available for this device
     */
    bool isAvailable() const { return is_available_; }
    
    /**
     * @brief Get device information
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
    bool initializeNVML();
    bool initializeROCm();
    bool initializeVulkan();
    bool initializeDirectX();
    
    // Cleanup
    void shutdownNVML();
    void shutdownROCm();
    void shutdownVulkan();
    void shutdownDirectX();
    
    // Query methods
    Metrics queryNVML();
    Metrics queryROCm();
    Metrics queryVulkan();
    Metrics queryDirectX();
    
    // Fallback when monitoring not available
    Metrics getFallbackMetrics() const;
};

} // namespace lora
} // namespace llm
} // namespace themis
