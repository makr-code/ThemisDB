#pragma once

#include <llama.h>
#include <memory>
#include <string>
#include <vector>
#include "acceleration/compute_backend.h"
#include "llm/gpu_memory_manager.h"

namespace themis {
namespace llm {

/**
 * @brief GPU Backend Configuration for llama.cpp
 * 
 * Integrates ThemisDB Backend-Infrastructure with llama.cpp.
 * Vulkan backend is prioritized for cross-platform compatibility.
 */
struct GPUBackendConfig {
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
    ~LlamaModelHandle();
    
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
    ~LlamaContextHandle();
    
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
    ~BackendAwareLlamaModelHandle();
    
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
