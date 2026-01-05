#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace themis {
namespace llm {

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
    
    Stats getStats() const;
    std::vector<std::string> getLoadedModels() const;
    
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
    
    void initializeGPU();
    void shutdownGPU();
    void updateMemoryStats();
};

} // namespace llm
} // namespace themis
