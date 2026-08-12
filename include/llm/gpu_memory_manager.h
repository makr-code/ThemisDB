/**
 * @file gpu_memory_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=8; TODO=1, Stub=4, Unimpl=0, Mock=2, Sim=0, Debt=1, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>

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
    using GPUTemperatureProviderFn = std::function<bool(int gpu_device_id, float& temperature_celsius)>;

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
        GPUTemperatureProviderFn temperature_provider_fn;  // Optional temperature callback
    };
    
    explicit GPUMemoryManager(const Config& config);
    ~GPUMemoryManager();

    /**
     * @brief Function type for per-device GPU temperature query (stub #309).
     *
     * When injected via setNvmlTemperatureFn(), updateGPUHealth() calls this
     * function instead of returning a hardcoded 0.0 °C placeholder.  The
     * function receives the device ID and must return the current temperature
     * in degrees Celsius, or throw on error.
     */
    using NvmlTemperatureFn = std::function<float(int /*device_id*/)>;

    /**
     * @brief Inject an NVML-based (or mock) GPU temperature provider.
     *
     * Thread-safe.  Passing nullptr reverts to the 0.0 °C placeholder.
     *
     * @param fn Temperature query callback.
     */
    static void setNvmlTemperatureFn(NvmlTemperatureFn fn);
    
    // Memory allocation
    [[nodiscard]] void* allocateGPU(const std::string& model_id, size_t bytes);
    [[nodiscard]] void* allocateCPU(const std::string& model_id, size_t bytes, bool pinned = false);
    
    // Multi-GPU memory allocation (v1.4.0)
    [[nodiscard]] void* allocateGPU(const std::string& model_id, size_t bytes, int gpu_device_id);
    
    // Memory deallocation
    [[nodiscard]] bool freeGPU(const std::string& model_id, void* ptr);
    [[nodiscard]] bool freeCPU(const std::string& model_id, void* ptr);
    
    // Free all memory for a model
    [[nodiscard]] bool freeModel(const std::string& model_id);
    
    // Free model memory on specific GPU (v1.4.0)
    [[nodiscard]] bool freeModel(const std::string& model_id, int gpu_device_id);
    
    // Memory queries
    [[nodiscard]] size_t getModelVRAM(const std::string& model_id) const;
    [[nodiscard]] size_t getModelRAM(const std::string& model_id) const;
    [[nodiscard]] size_t getTotalVRAM() const;
    [[nodiscard]] size_t getTotalRAM() const;
    [[nodiscard]] size_t getFreeVRAM() const;
    [[nodiscard]] size_t getFreeRAM() const;
    
    // Multi-GPU memory queries (v1.4.0)
    [[nodiscard]] size_t getGPUVRAM(int gpu_device_id) const;  // Used VRAM on specific GPU
    [[nodiscard]] size_t getFreeGPUVRAM(int gpu_device_id) const;  // Free VRAM on specific GPU
    [[nodiscard]] std::vector<int> getAvailableGPUs() const;  // List of available GPU IDs
    [[nodiscard]] bool isGPUAvailable(int gpu_device_id) const;  // Check if GPU is available
    
    // Capacity checks
    [[nodiscard]] bool canAllocate(size_t vram_bytes, size_t ram_bytes) const;
    [[nodiscard]] size_t getMemoryFragmentation() const;  // Returns fragmentation %
    
    // Defragmentation
    [[nodiscard]] bool defragment();
    
    // Statistics
    struct Stats {
        size_t total_vram_bytes = 0;
        size_t used_vram_bytes = 0;
        size_t free_vram_bytes = 0;
        size_t total_ram_bytes = 0;
        size_t used_ram_bytes = 0;
        size_t free_ram_bytes = 0;
        size_t num_allocations = 0;
        size_t num_models = 0;
        size_t fragmentation_pct = 0;
    };
    
    // Per-GPU statistics
    struct GPUStats {
        int device_id = 0;
        size_t total_vram_bytes = 0;
        size_t used_vram_bytes = 0;
        size_t free_vram_bytes = 0;
        size_t num_allocations = 0;
        float utilization_percent = 0.0f;  // 0.0 - 100.0
        float temperature_celsius = 0.0f;
        bool is_healthy = false;
        std::vector<std::string> loaded_models;
        std::vector<std::string> loaded_adapters;
    };
    
    // GPU Health status
    struct GPUHealth {
        int device_id = 0;
        bool is_available = false;
        bool is_healthy = false;
        float temperature_celsius = 0.0f;
        float utilization_percent = 0.0f;
        size_t error_count = 0;
        std::string last_error;
        int64_t last_check_timestamp_ms = 0;
    };
    
    [[nodiscard]] Stats getStats() const;
    [[nodiscard]] std::vector<std::string> getLoadedModels() const;
    
    // Per-GPU statistics and monitoring
    [[nodiscard]] GPUStats getGPUStats(int gpu_device_id) const;
    [[nodiscard]] std::vector<GPUStats> getAllGPUStats() const;
    
    // GPU Health monitoring
    [[nodiscard]] GPUHealth getGPUHealth(int gpu_device_id) const;
    [[nodiscard]] std::vector<GPUHealth> getAllGPUHealth() const;
    [[nodiscard]] bool isGPUHealthy(int gpu_device_id) const;
    void markGPUUnhealthy(int gpu_device_id, const std::string& reason);
    void markGPUHealthy(int gpu_device_id);
    
    // Load balancing queries
    [[nodiscard]] int getLeastLoadedGPU() const;  // Returns GPU with lowest utilization
    [[nodiscard]] std::vector<int> getHealthyGPUs() const;  // Returns list of healthy GPUs
    [[nodiscard]] float getAverageGPULoad() const;  // Average utilization across all GPUs
    [[nodiscard]] bool needsLoadRebalancing(float threshold) const;  // Check if rebalancing needed
    
    // Multi-GPU peer access (v1.4.0)
    [[nodiscard]] bool enablePeerAccess(int src_gpu, int dst_gpu);
    [[nodiscard]] bool disablePeerAccess(int src_gpu, int dst_gpu);
    [[nodiscard]] bool canAccessPeer(int src_gpu, int dst_gpu) const;

    /**
     * @brief Install runtime GPU temperature provider.
     * @param fn Provider callable that sets temperature_celsius and returns true on success.
     *           When set, this overrides the construction-time Config::temperature_provider_fn.
     */
    void setGPUTemperatureProviderFn(GPUTemperatureProviderFn fn);

    /**
     * @brief Remove runtime GPU temperature provider and use built-in fallback.
     */
    void clearGPUTemperatureProviderFn();
    
private:
    Config config_;
    
    std::unordered_map<std::string, std::vector<MemoryAllocation>> allocations_;
    mutable std::mutex mutex_;
    
    std::atomic<size_t> total_vram_used_ = 0;
    std::atomic<size_t> total_ram_used_ = 0;
    
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
    GPUTemperatureProviderFn temperature_provider_fn_;
    
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
