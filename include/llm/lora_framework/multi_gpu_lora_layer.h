/**
 * @file multi_gpu_lora_layer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/nccl_backend.h"
#include "llm/lora_framework/rccl_backend.h"
#include "llm/lora_framework/custom_allreduce.h"
#include <vector>
#include <memory>

namespace themis {
namespace llm {
namespace lora {

enum class CommBackend {
    AUTO,        // Automatically select best available backend
    NCCL,        // NVIDIA NCCL (CUDA only)
    RCCL,        // AMD RCCL (HIP only)
    CUSTOM       // Custom ring all-reduce (fallback)
};

class MultiGPULoRALayer {
public:
    MultiGPULoRALayer(size_t in_dim, size_t out_dim, size_t rank,
                      float scaling,
                      const MultiGPUContext& ctx,
                      CommBackend backend = CommBackend::AUTO,
                      bool use_fused_kernels = true);
    
    ~MultiGPULoRALayer() = default;
    
    // Disable copy, enable move
    MultiGPULoRALayer(const MultiGPULoRALayer&) = delete;
    MultiGPULoRALayer& operator=(const MultiGPULoRALayer&) = delete;
    MultiGPULoRALayer(MultiGPULoRALayer&&) noexcept = default;
    MultiGPULoRALayer& operator=(MultiGPULoRALayer&&) noexcept = default;
    
    /**
     * @brief Forward.
     * @param[in] inputs Input parameter.
     * @return Return value.
     */
    std::vector<GPUTensor> forward(const std::vector<GPUTensor>& inputs);
    
    /**
     * @brief Backward.
     * @param[in] grad_outputs Input parameter.
     * @return Return value.
     */
    std::vector<GPUTensor> backward(const std::vector<GPUTensor>& grad_outputs);
    
    /**
     * @brief Synchronize gradients.
     * @return True when the operation succeeds.
     */
    bool synchronize_gradients();
    
    /**
     * @brief Zero grad.
     */
    void zero_grad();
    
    /**
     * @brief Get layer.
     * @param[in] rank Input parameter.
     * @return Return value.
     */
    GPULoRALayer& get_layer(int rank);
    
    /**
     * @brief Get layers.
     * @return Return value.
     */
    std::vector<GPULoRALayer*> get_layers();
    
    const MultiGPUContext& context() const { return ctx_; }
    
    int num_gpus() const { return ctx_.num_gpus(); }
    
    CommBackend backend_type() const { return backend_type_; }
    
    bool are_gradients_synced() const { return gradients_synced_; }
    
    /**
     * @brief Broadcast parameters.
     * @return True when the operation succeeds.
     */
    bool broadcast_parameters();
    
    struct Stats {
        float communication_time_ms = 0.0f;
        float computation_time_ms = 0.0f;
        size_t bytes_communicated = 0;
        int num_syncs = 0;
        
        float efficiency() const {
            return computation_time_ms / (computation_time_ms + communication_time_ms + 1e-6f);
        }
    };
    
    Stats get_stats() const { return stats_; }
    /**
     * @brief Reset stats.
     * @details Implements reset_stats without additional internal calls.
     */
    void reset_stats() { stats_ = Stats{}; }
    
private:
    const MultiGPUContext& ctx_;
    std::vector<std::unique_ptr<GPULoRALayer>> layers_;
    
    CommBackend backend_type_;
    std::unique_ptr<NCCLBackend> nccl_backend_;
    std::unique_ptr<RCCLBackend> rccl_backend_;
    std::unique_ptr<CustomAllReduce> custom_backend_;
    
    bool gradients_synced_ = false;
    Stats stats_;
    
    /**
     * @brief Initialize backend.
     * @param[in] backend Input parameter.
     */
    void initialize_backend(CommBackend backend);
    /**
     * @brief Allreduce gradients.
     * @return True when the operation succeeds.
     */
    bool allreduce_gradients();
};

} // namespace lora
} // namespace llm
} // namespace themis
