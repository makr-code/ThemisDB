/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vllm_resource_manager.h                            ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <optional>

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
    
    // Disable copy, allow move
    VLLMResourceManager(const VLLMResourceManager&) = delete;
    VLLMResourceManager& operator=(const VLLMResourceManager&) = delete;
    VLLMResourceManager(VLLMResourceManager&&) = default;
    VLLMResourceManager& operator=(VLLMResourceManager&&) = default;
    
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
    
private:
    Config config_;
    bool initialized_ = false;
    
    // NVML handle for GPU monitoring (opaque pointer)
    void* nvml_device_ = nullptr;
    
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
     */
    std::optional<double> queryGPUUtilization();
};

} // namespace acceleration
} // namespace themis
