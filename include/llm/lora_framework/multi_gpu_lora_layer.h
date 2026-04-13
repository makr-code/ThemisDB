/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_gpu_lora_layer.h                             ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Communication backend type for multi-GPU
 */
enum class CommBackend {
    AUTO,        // Automatically select best available backend
    NCCL,        // NVIDIA NCCL (CUDA only)
    RCCL,        // AMD RCCL (HIP only)
    CUSTOM       // Custom ring all-reduce (fallback)
};

/**
 * @brief Multi-GPU LoRA Layer for data-parallel training
 * 
 * Wraps multiple GPULoRALayer instances, one per GPU.
 * Implements data parallelism:
 * - Each GPU has full model replica
 * - Data batches are sharded across GPUs
 * - Gradients are synchronized via all-reduce after backward
 * - All GPUs update with averaged gradients
 * 
 * Example:
 * ```cpp
 * MultiGPUContext ctx(4);  // Use 4 GPUs
 * MultiGPULoRALayer layer(768, 768, 8, 1.0f, ctx);
 * 
 * // Each GPU processes its batch shard
 * auto outputs = layer.forward(inputs);  // inputs[i] on GPU i
 * layer.backward(grad_outputs);
 * layer.synchronize_gradients();  // All-reduce gradients
 * optimizer.step();  // Update with averaged gradients
 * ```
 */
class MultiGPULoRALayer {
public:
    /**
     * @brief Construct multi-GPU LoRA layer
     * @param in_dim Input dimension
     * @param out_dim Output dimension
     * @param rank LoRA rank
     * @param scaling LoRA scaling factor
     * @param ctx Multi-GPU context
     * @param backend Communication backend (AUTO, NCCL, RCCL, CUSTOM)
     * @param use_fused_kernels Enable kernel fusion
     */
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
     * @brief Forward pass on all GPUs
     * @param inputs Input tensors, one per GPU (must already be on correct device)
     * @return Output tensors, one per GPU
     */
    std::vector<GPUTensor> forward(const std::vector<GPUTensor>& inputs);
    
    /**
     * @brief Backward pass on all GPUs
     * @param grad_outputs Gradient tensors, one per GPU
     * @return Gradient w.r.t. inputs, one per GPU
     */
    std::vector<GPUTensor> backward(const std::vector<GPUTensor>& grad_outputs);
    
    /**
     * @brief Synchronize gradients across all GPUs (all-reduce)
     * 
     * Must be called after backward() and before optimizer step.
     * Averages gradients across all GPUs so each GPU has identical gradients.
     */
    bool synchronize_gradients();
    
    /**
     * @brief Zero gradients on all GPUs
     */
    void zero_grad();
    
    /**
     * @brief Get layer on specific GPU rank
     * @param rank GPU rank (0 to num_gpus-1)
     */
    GPULoRALayer& get_layer(int rank);
    
    /**
     * @brief Get all layers
     */
    std::vector<GPULoRALayer*> get_layers();
    
    /**
     * @brief Get multi-GPU context
     */
    const MultiGPUContext& context() const { return ctx_; }
    
    /**
     * @brief Get number of GPUs
     */
    int num_gpus() const { return ctx_.num_gpus(); }
    
    /**
     * @brief Get communication backend type
     */
    CommBackend backend_type() const { return backend_type_; }
    
    /**
     * @brief Check if gradients are synchronized
     */
    bool are_gradients_synced() const { return gradients_synced_; }
    
    /**
     * @brief Broadcast parameters from rank 0 to all other GPUs
     * 
     * Ensures all GPUs start with identical parameters.
     * Call this after initialization or loading checkpoint.
     */
    bool broadcast_parameters();
    
    /**
     * @brief Get statistics (communication time, etc.)
     */
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
    void reset_stats() { stats_ = Stats{}; }
    
private:
    const MultiGPUContext& ctx_;
    std::vector<std::unique_ptr<GPULoRALayer>> layers_;
    
    CommBackend backend_type_;
    std::unique_ptr<NCCLBackend> nccl_backend_;
    std::unique_ptr<RCCLBackend> rccl_backend_;
    std::unique_ptr<CustomAllReduce> custom_backend_;
    
    bool gradients_synced_;
    Stats stats_;
    
    void initialize_backend(CommBackend backend);
    bool allreduce_gradients();
};

} // namespace lora
} // namespace llm
} // namespace themis
