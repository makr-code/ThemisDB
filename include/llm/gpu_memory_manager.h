#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace themis {
namespace llm {

/**
 * @brief GPU Memory Manager for multi-model serving
 * 
 * Week 6 Implementation: Tracks GPU memory usage across multiple models
 * and integrates with LazyModelLoader for intelligent eviction decisions.
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
    };
    
    struct Config {
        size_t max_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24 GB
        size_t max_ram_bytes = 64ULL * 1024 * 1024 * 1024;   // 64 GB
        size_t min_free_vram_bytes = 2ULL * 1024 * 1024 * 1024;  // 2 GB reserve
        bool enable_memory_pooling = true;
        bool enable_defragmentation = true;
    };
    
    explicit GPUMemoryManager(const Config& config);
    ~GPUMemoryManager();
    
    // Memory allocation
    void* allocateGPU(const std::string& model_id, size_t bytes);
    void* allocateCPU(const std::string& model_id, size_t bytes, bool pinned = false);
    
    // Memory deallocation
    bool freeGPU(const std::string& model_id, void* ptr);
    bool freeCPU(const std::string& model_id, void* ptr);
    
    // Free all memory for a model
    bool freeModel(const std::string& model_id);
    
    // Memory queries
    size_t getModelVRAM(const std::string& model_id) const;
    size_t getModelRAM(const std::string& model_id) const;
    size_t getTotalVRAM() const;
    size_t getTotalRAM() const;
    size_t getFreeVRAM() const;
    size_t getFreeRAM() const;
    
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
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::vector<MemoryAllocation>> allocations_;
    mutable std::mutex mutex_;
    
    size_t total_vram_used_ = 0;
    size_t total_ram_used_ = 0;
    
    // GPU device information
    int gpu_device_id_ = 0;
    bool gpu_available_ = false;
    
    void initializeGPU();
    void shutdownGPU();
    void updateMemoryStats();
};

} // namespace llm
} // namespace themis
