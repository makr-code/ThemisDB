/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_memory_manager.h                               ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:39:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     223                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <memory>

namespace themis {
namespace llm {

// Forward declaration for internal memory holder
namespace detail {
    class MemoryHolder;
}

/**
 * @brief GPU Memory Manager for multi-model and multi-GPU serving
 * 
 * Week 6 Implementation: Tracks GPU memory usage across multiple models
 * and integrates with LazyModelLoader for intelligent eviction decisions.
 * 
 * v1.4.0 Enhancement: Added multi-GPU support for distributed LoRA adapters.
 */
class GPUMemoryManager {
public:
    struct MemoryAllocation {
        std::string model_id;
        size_t vram_bytes = 0;
        size_t ram_bytes = 0;
        void* gpu_ptr = nullptr;
        void* cpu_ptr = nullptr;
        bool is_pinned = false;
        int gpu_device_id = 0;  // v1.4.0: Track which GPU owns this allocation
        
        // Internal RAII holder for exception safety
        // This ensures memory is freed even if exceptions occur during cleanup
        std::shared_ptr<detail::MemoryHolder> holder;
    };
    
    struct Config {
        size_t max_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24 GB (per GPU in multi-GPU mode)
        size_t max_ram_bytes = 64ULL * 1024 * 1024 * 1024;   // 64 GB
        size_t min_free_vram_bytes = 2ULL * 1024 * 1024 * 1024;  // 2 GB reserve
        bool enable_memory_pooling = true;
        bool enable_defragmentation = true;
        
        // Multi-GPU support (v1.4.0)
        bool enable_multi_gpu = false;
        std::vector<int> gpu_devices;  // GPU device IDs to use
        bool enable_peer_access = false;  // Enable CUDA peer-to-peer access
    };
    
    explicit GPUMemoryManager(const Config& config);
    ~GPUMemoryManager();
    
    // Memory allocation
    void* allocateGPU(const std::string& model_id, size_t bytes);
    void* allocateCPU(const std::string& model_id, size_t bytes, bool pinned = false);
    
    // Multi-GPU memory allocation (v1.4.0)
    void* allocateGPU(const std::string& model_id, size_t bytes, int gpu_device_id);
    
    // Memory deallocation
    bool freeGPU(const std::string& model_id, void* ptr);
    bool freeCPU(const std::string& model_id, void* ptr);
    
    // Free all memory for a model
    bool freeModel(const std::string& model_id);
    
    // Free model memory on specific GPU (v1.4.0)
    bool freeModel(const std::string& model_id, int gpu_device_id);
    
    // Memory queries
    size_t getModelVRAM(const std::string& model_id) const;
    size_t getModelRAM(const std::string& model_id) const;
    size_t getTotalVRAM() const;
    size_t getTotalRAM() const;
    size_t getFreeVRAM() const;
    size_t getFreeRAM() const;
    
    // Multi-GPU memory queries (v1.4.0)
    size_t getGPUVRAM(int gpu_device_id) const;  // Used VRAM on specific GPU
    size_t getFreeGPUVRAM(int gpu_device_id) const;  // Free VRAM on specific GPU
    std::vector<int> getAvailableGPUs() const;  // List of available GPU IDs
    bool isGPUAvailable(int gpu_device_id) const;  // Check if GPU is available
    
    // Capacity checks
    bool canAllocate(size_t vram_bytes, size_t ram_bytes) const;
    size_t getMemoryFragmentation() const;  // Returns fragmentation %
    
    // Defragmentation
    bool defragment();
    
    // Statistics
    struct Stats {
        size_t total_vram_bytes;
        size_t used_vram_bytes;
        size_t free_vram_bytes;
        size_t total_ram_bytes;
        size_t used_ram_bytes;
        size_t free_ram_bytes;
        size_t num_allocations;
        size_t num_models;
        size_t fragmentation_pct;
    };
    
    // Per-GPU statistics
    struct GPUStats {
        int device_id;
        size_t total_vram_bytes;
        size_t used_vram_bytes;
        size_t free_vram_bytes;
        size_t num_allocations;
        float utilization_percent;  // 0.0 - 100.0
        float temperature_celsius;
        bool is_healthy;
        std::vector<std::string> loaded_models;
        std::vector<std::string> loaded_adapters;
    };
    
    // GPU Health status
    struct GPUHealth {
        int device_id;
        bool is_available;
        bool is_healthy;
        float temperature_celsius;
        float utilization_percent;
        size_t error_count;
        std::string last_error;
        int64_t last_check_timestamp_ms;
    };
    
    Stats getStats() const;
    std::vector<std::string> getLoadedModels() const;
    
    // Per-GPU statistics and monitoring
    GPUStats getGPUStats(int gpu_device_id) const;
    std::vector<GPUStats> getAllGPUStats() const;
    
    // GPU Health monitoring
    GPUHealth getGPUHealth(int gpu_device_id) const;
    std::vector<GPUHealth> getAllGPUHealth() const;
    bool isGPUHealthy(int gpu_device_id) const;
    void markGPUUnhealthy(int gpu_device_id, const std::string& reason);
    void markGPUHealthy(int gpu_device_id);
    
    // Load balancing queries
    int getLeastLoadedGPU() const;  // Returns GPU with lowest utilization
    std::vector<int> getHealthyGPUs() const;  // Returns list of healthy GPUs
    float getAverageGPULoad() const;  // Average utilization across all GPUs
    bool needsLoadRebalancing(float threshold) const;  // Check if rebalancing needed
    
    // Multi-GPU peer access (v1.4.0)
    bool enablePeerAccess(int src_gpu, int dst_gpu);
    bool disablePeerAccess(int src_gpu, int dst_gpu);
    bool canAccessPeer(int src_gpu, int dst_gpu) const;
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::vector<MemoryAllocation>> allocations_;
    mutable std::mutex mutex_;
    
    size_t total_vram_used_ = 0;
    size_t total_ram_used_ = 0;
    
    // GPU device information
    int gpu_device_id_ = 0;  // Primary GPU (for single-GPU mode)
    bool gpu_available_ = false;
    
    // Multi-GPU support (v1.4.0)
    std::unordered_map<int, size_t> per_gpu_vram_used_;  // Per-GPU VRAM tracking
    std::unordered_map<int, bool> gpu_health_status_;    // GPU health tracking
    std::vector<int> available_gpus_;                    // List of available GPUs
    
    // Enhanced GPU health monitoring
    std::unordered_map<int, GPUHealth> gpu_health_data_;  // Detailed health data per GPU
    std::unordered_map<int, float> gpu_temperatures_;     // Temperature tracking
    std::unordered_map<int, float> gpu_utilizations_;     // Utilization tracking
    std::unordered_map<int, size_t> gpu_error_counts_;    // Error count per GPU
    
    // Adapter tracking for load balancing
    std::unordered_map<int, std::vector<std::string>> gpu_adapters_;  // Adapters per GPU
    std::unordered_map<int, std::vector<std::string>> gpu_models_;    // Models per GPU
    
    void initializeGPU();
    void shutdownGPU();
    void updateMemoryStats();
    void updateGPUHealth(int gpu_device_id);  // Update health metrics
    void checkGPUHealth(int gpu_device_id);   // Perform health check
    
    // Defragmentation helper methods
    bool defragmentModelGPU(const std::string& model_id, const std::vector<MemoryAllocation>& gpu_allocs);
    bool defragmentModelCPU(const std::string& model_id, const std::vector<MemoryAllocation>& cpu_allocs);
};

} // namespace llm
} // namespace themis
