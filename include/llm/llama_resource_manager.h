/**
 * @file llama_resource_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include "acceleration/compute_backend.h"
#include "llm/gpu_memory_manager.h"

// Forward declarations keep this public header independent from llama.cpp include paths.
struct llama_model;
struct llama_context;
struct llama_model_params;
struct llama_context_params;

namespace themis {
namespace llm {

/**
 * @brief GPU Backend Configuration for llama.cpp
 * 
 * Integrates ThemisDB Backend-Infrastructure with llama.cpp.
 * Vulkan backend is prioritized for cross-platform compatibility.
 * 
 * Enhanced with multi-GPU distribution, load balancing, health checks,
 * and persistent pinning support.
 */
struct GPUBackendConfig {
    virtual ~GPUBackendConfig() = default;
    // Backend Selection (Vulkan prioritized)
    acceleration::BackendType preferred_backend = acceleration::BackendType::VULKAN;
    std::vector<acceleration::BackendType> fallback_backends = {
        acceleration::BackendType::VULKAN,
        acceleration::BackendType::CUDA,
        acceleration::BackendType::HIP,
        acceleration::BackendType::METAL,
        acceleration::BackendType::DIRECTX,
        acceleration::BackendType::OPENCL
    };
    
    // GPU Device Selection
    int primary_gpu_id = 0;
    std::vector<int> secondary_gpus;  // For Tensor Parallelism
    
    // Memory Management
    bool use_gpu_memory_manager = true;
    size_t max_vram_per_gpu = 24ULL * 1024 * 1024 * 1024;  // 24 GB
    size_t reserved_vram = 2ULL * 1024 * 1024 * 1024;      // 2 GB Reserve
    
    // Layer Offloading Strategy
    int n_gpu_layers = -1;  // -1 = all layers
    bool auto_detect_optimal_layers = true;
    
    // Advanced GPU Features
    bool enable_flash_attention = true;
    bool enable_tensor_cores = true;
    bool enable_peer_to_peer = false;
    bool enable_unified_memory = false;
    
    // Multi-GPU Distribution (Tensor Parallelism)
    enum class TensorParallelismMode {
        NONE,           // Single GPU, no parallelism
        PIPELINE,       // Pipeline parallelism (layer-wise)
        TENSOR,         // Tensor parallelism (split tensors)
        HYBRID          // Hybrid pipeline + tensor parallelism
    };
    TensorParallelismMode tensor_parallel_mode = TensorParallelismMode::NONE;
    float tensor_split_ratio = 0.5f;  // Split ratio for tensor parallelism (0.0-1.0)
    
    // Dynamic Load Balancing
    bool enable_dynamic_load_balancing = true;
    float load_balance_threshold = 0.8f;  // Trigger rebalancing at 80% utilization
    int load_balance_interval_ms = 5000;  // Check interval for load balancing
    
    // JIT Eviction for LoRA Adapters
    bool enable_jit_eviction = true;
    size_t adapter_cache_size = 10;  // Max adapters in memory per GPU
    float eviction_threshold = 0.9f;  // Evict when VRAM usage > 90%
    
    // GPU Health Monitoring
    bool enable_health_checks = true;
    int health_check_interval_ms = 10000;  // Check interval for GPU health
    float max_gpu_temperature_celsius = 85.0f;  // Max safe temperature
    float max_gpu_utilization = 0.95f;  // Max safe utilization (95%)
    bool auto_failover_on_error = true;  // Automatic failover to healthy GPUs
    
    // Persistent Pinning
    bool enable_persistent_pinning = true;
    std::vector<std::string> pinned_model_ids;  // Model IDs to keep pinned
    std::vector<std::string> pinned_adapter_ids;  // Adapter IDs to keep pinned
    int pinned_resource_priority = 10;  // Priority for pinned resources (higher = more important)
};

/**
 * @brief RAII wrapper for llama.cpp Model-Handle
 * 
 * Design Pattern: RAII (Resource Acquisition Is Initialization)
 * Ensures automatic cleanup of llama resources
 */
class LlamaModelHandle {
public:
    explicit LlamaModelHandle(const std::string& model_path, 
                             const llama_model_params& params);
    ~LlamaModelHandle() noexcept;
    
    // Non-copyable, movable (Modern C++ Best Practice)
    LlamaModelHandle(const LlamaModelHandle&) = delete;
    LlamaModelHandle& operator=(const LlamaModelHandle&) = delete;
    LlamaModelHandle(LlamaModelHandle&& other) noexcept;
    LlamaModelHandle& operator=(LlamaModelHandle&& other) noexcept;
    
    llama_model* get() const noexcept { return model_.get(); }
    explicit operator bool() const noexcept { return model_ != nullptr; }
    
    // Metadata queries
    size_t n_vocab() const;
    size_t n_embd() const;
    std::string model_type() const;

private:
    struct ModelDeleter {
        void operator()(llama_model* model) const;
    };
    
    std::unique_ptr<llama_model, ModelDeleter> model_;
};

/**
 * @brief RAII wrapper for llama.cpp Context-Handle
 */
class LlamaContextHandle {
public:
    explicit LlamaContextHandle(llama_model* model,
                               const llama_context_params& params);
    ~LlamaContextHandle() noexcept;
    
    LlamaContextHandle(const LlamaContextHandle&) = delete;
    LlamaContextHandle& operator=(const LlamaContextHandle&) = delete;
    LlamaContextHandle(LlamaContextHandle&& other) noexcept;
    LlamaContextHandle& operator=(LlamaContextHandle&& other) noexcept;
    
    llama_context* get() const noexcept { return context_.get(); }
    explicit operator bool() const noexcept { return context_ != nullptr; }
    
    // KV-Cache Management
    void clear_kv_cache();
    size_t kv_cache_token_count() const;

private:
    struct ContextDeleter {
        void operator()(llama_context* ctx) const;
    };
    
    std::unique_ptr<llama_context, ContextDeleter> context_;
};

/**
 * @brief Backend-Aware llama.cpp Model Handle
 * 
 * Integrates ThemisDB GPU backend infrastructure with llama.cpp.
 * Vulkan is prioritized for maximum hardware compatibility.
 */
class BackendAwareLlamaModelHandle {
public:
    explicit BackendAwareLlamaModelHandle(
        const std::string& model_path,
        const llama_model_params& params,
        const GPUBackendConfig& gpu_config
    );
    ~BackendAwareLlamaModelHandle() noexcept;
    
    // Non-copyable, movable
    BackendAwareLlamaModelHandle(const BackendAwareLlamaModelHandle&) = delete;
    BackendAwareLlamaModelHandle& operator=(const BackendAwareLlamaModelHandle&) = delete;
    BackendAwareLlamaModelHandle(BackendAwareLlamaModelHandle&& other) noexcept;
    BackendAwareLlamaModelHandle& operator=(BackendAwareLlamaModelHandle&& other) noexcept;
    
    llama_model* get() const noexcept { return model_.get(); }
    explicit operator bool() const noexcept { return model_ != nullptr; }
    
    // Backend Information
    acceleration::BackendType active_backend() const { return active_backend_; }
    std::string backend_name() const;
    
    // GPU Memory Information
    size_t vram_usage() const { return vram_allocated_; }
    std::vector<int> gpu_devices() const { return gpu_devices_; }
    
    // Memory Transfer (for Multi-GPU)
    bool transferToGPU(int target_gpu_id);
    bool prefetchToGPU();

private:
    struct ModelDeleter {
        void operator()(llama_model* model) const;
    };
    
    std::unique_ptr<llama_model, ModelDeleter> model_;
    
    // Backend Management
    acceleration::BackendType active_backend_;
    acceleration::IComputeBackend* backend_ = nullptr;
    
    // GPU Memory Management
    std::shared_ptr<GPUMemoryManager> gpu_memory_manager_;
    std::vector<int> gpu_devices_;
    size_t vram_allocated_ = 0;
    
    // Helper methods
    acceleration::BackendType selectBestBackend(const GPUBackendConfig& config);
    int determineOptimalGPULayers(const GPUBackendConfig& config, size_t model_size);
    void allocateGPUMemory(const GPUBackendConfig& config);
    void configureBackendSpecificFeatures();
};

} // namespace llm
} // namespace themis

