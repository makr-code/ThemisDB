/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vllm_resource_manager.h                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     235                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 137d06cfed  2026-03-18  fix: handle dtotal==0 CPU cache-hit and guard perf test a... ║
    • 3254a3d48f  2026-03-18  feat(acceleration): VLLMResourceManager multi-GPU NVML mo... ║
    • 7a41ab30da  2026-03-18  feat(acceleration): implement CPU snapshot cache (200ms T... ║
    • 592b543821  2026-03-15  fix(scheduler,acceleration): remove stale TODOs, add VLLM... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <chrono>
#include <mutex>
#include <functional>
#include <string>
#include <memory>
#include <optional>
#include <vector>

namespace themis {
namespace acceleration {

/**
 * @brief Resource Manager for ThemisDB + vLLM Co-Location
 * 
 * Manages CPU/RAM/GPU resource allocation when ThemisDB runs alongside vLLM
 * for AI/ML workloads (RAG, semantic search, etc.)
 * 
 * v1.1.0 Feature: Optimizes resource sharing for maximum efficiency
 */
class VLLMResourceManager {
public:
    /**
     * @brief Resource configuration for co-location
     */
    struct Config {
        // System Resources
        size_t total_cpu_cores = 64;
        size_t total_ram_gb = 256;
        size_t total_gpu_count = 4;
        
        // vLLM Reservation
        size_t vllm_cpu_cores = 14;       // vLLM needs minimal CPU
        size_t vllm_ram_gb = 56;          // Model loading + KV cache
        
        // ThemisDB Allocation (auto-calculated from remaining)
        size_t themis_cpu_cores = 50;     // 50 of 64 cores
        size_t themis_ram_gb = 200;       // 200 of 256 GB
        
        // ThemisDB Internal Allocation
        double rocksdb_thread_ratio = 0.3;  // 30% for RocksDB (15 cores)
        double tbb_thread_ratio = 0.6;      // 60% for TBB (30 cores)
        double system_reserve_ratio = 0.1;   // 10% reserve (5 cores)
        
        // GPU Sharing
        size_t max_gpu_vram_mb = 2048;      // 2 GB per GPU for ThemisDB
        size_t max_vector_batch_size = 1024; // Small batches to avoid blocking vLLM
        int gpu_priority = -1;               // Lower priority than vLLM

        // GPU device selection for NVML monitoring.
        // If gpu_device_indices is non-empty it takes precedence over gpu_device_index
        // and queryGPUUtilization() returns the *maximum* utilization across all listed
        // devices (so a single busy GPU blocks new ThemisDB work on any device).
        //
        // Example (4-GPU node, GPUs 0-1 reserved for vLLM, 2-3 for ThemisDB):
        //   config.gpu_device_indices = {2, 3};
        //
        // When both fields are at their defaults the manager monitors device 0 only
        // (backward-compatible behaviour).
        uint32_t gpu_device_index = 0;                        ///< Primary device to monitor (default: 0)
        std::vector<uint32_t> gpu_device_indices;             ///< Explicit multi-device override; empty = use gpu_device_index
    };
    
    /**
     * @brief Current resource usage statistics
     */
    struct Stats {
        // CPU Usage
        double cpu_utilization = 0.0;       // 0-100%
        size_t active_threads = 0;
        
        // RAM Usage
        size_t ram_used_mb = 0;
        double ram_utilization = 0.0;       // 0-100%
        
        // GPU Usage
        size_t gpu_vram_used_mb = 0;
        double gpu_utilization = 0.0;       // 0-100%
        bool gpu_available = false;
        
        // vLLM Interference
        bool vllm_detected = false;
        double vllm_gpu_usage = 0.0;        // Estimated vLLM GPU usage
    };
    
    /**
     * @brief Construct resource manager with configuration
     */
    explicit VLLMResourceManager(const Config& config);
    ~VLLMResourceManager();
    
    // Disable copy and move (std::mutex member is not movable/copyable)
    VLLMResourceManager(const VLLMResourceManager&) = delete;
    VLLMResourceManager& operator=(const VLLMResourceManager&) = delete;
    VLLMResourceManager(VLLMResourceManager&&) = delete;
    VLLMResourceManager& operator=(VLLMResourceManager&&) = delete;
    
    /**
     * @brief Initialize resource manager and detect hardware
     */
    bool initialize();
    
    /**
     * @brief Shutdown and release resources
     */
    void shutdown();
    
    /**
     * @brief Check if GPU can be used (vLLM not busy)
     * 
     * Uses NVML to check GPU utilization. Only allows GPU use if vLLM
     * is below threshold (< 80% GPU usage).
     * 
     * @return true if GPU can be used
     */
    bool canUseGPU();
    
    /**
     * @brief Get recommended thread count for operation type
     * 
     * @param operation_type "rocksdb", "tbb", "general"
     * @return Recommended thread count
     */
    size_t getRecommendedThreadCount(const std::string& operation_type) const;
    
    /**
     * @brief Get current resource usage statistics
     */
    Stats getStats() const;
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration (requires reinitialization)
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Inject a GPU utilization provider for testing (bypasses NVML).
     *
     * When set, canUseGPU() and queryGPUUtilization() call this function instead
     * of querying the real NVML stack. Allows CI tests to simulate any GPU
     * utilization level without physical GPU hardware.
     *
     * Pass an empty std::function to clear the override.
     *
     * @param provider Returns the simulated GPU utilization (0–100), or nullopt
     *                 if the GPU cannot be queried (treated as GPU busy).
     */
    void setGpuUtilizationProviderForTesting(
        std::function<std::optional<double>()> provider);

private:
    Config config_;
    bool initialized_ = false;
    
    // CPU snapshot cache: avoids a blocking 100 ms sleep when getStats() is called
    // repeatedly within the 200 ms TTL window.  Three uint64_t fields store the
    // most recent raw OS counters without requiring platform-specific types:
    //   Linux   – v0 = total jiffies, v1 = idle jiffies, v2 = (unused)
    //   Windows – v0 = idle FILETIME, v1 = kernel FILETIME, v2 = user FILETIME
    struct CpuSnapshot {
        uint64_t v0 = 0;          // Linux: total; Windows: idle
        uint64_t v1 = 0;          // Linux: idle;  Windows: kernel
        uint64_t v2 = 0;          // Linux: (unused); Windows: user
        double last_cpu_util = 0.0; // last successfully computed utilization [0,100]
        std::chrono::steady_clock::time_point ts;
        bool valid = false;
    };
    mutable CpuSnapshot cpu_snapshot_cache_;
    mutable std::mutex  cpu_cache_mutex_;

    // Test-only GPU utilization override (see setGpuUtilizationProviderForTesting).
    std::function<std::optional<double>()> gpu_util_provider_for_testing_;

    // NVML handles for GPU monitoring (opaque pointers to nvmlDevice_t).
    // nvml_devices_ is the authoritative list; nvml_device_ is a convenience
    // alias to nvml_devices_.front() used only by canUseGPU() for the
    // timeout-guarded primary-device utilization query.
    // Both fields are always kept in sync by initializeNVML()/shutdownNVML().
    void* nvml_device_ = nullptr;
    std::vector<void*> nvml_devices_;
    
    /**
     * @brief Initialize NVML for GPU monitoring
     */
    bool initializeNVML();
    
    /**
     * @brief Shutdown NVML
     */
    void shutdownNVML();
    
    /**
     * @brief Query GPU utilization via NVML
     *
     * When multiple devices are monitored (gpu_device_indices), returns the
     * maximum utilization across all of them.
     */
    std::optional<double> queryGPUUtilization();
};

} // namespace acceleration
} // namespace themis
