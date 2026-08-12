/**
 * @file multi_gpu_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include "llm/lora_framework/lora_layers.h"
#include <vector>
#include <memory>
#include <functional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Multi-GPU trainer for data-parallel LoRA training
 * 
 * Coordinates training across multiple GPUs with:
 * - Data parallelism (each GPU processes different batch)
 * - Gradient synchronization via all-reduce
 * - Distributed checkpointing
 * - Automatic load balancing
 * 
 * Example:
 * ```cpp
 * MultiGPUContext ctx(4);
 * MultiGPULoRATrainer trainer(ctx);
 * 
 * auto layer = trainer.create_layer(768, 768, 8, 1.0f);
 * 
 * for (int epoch = 0; epoch < 10; ++epoch) {
 *     for (auto& batch : data_loader) {
 *         auto sharded_batches = trainer.shard_batch(batch, 4);
 *         float loss = trainer.train_step(layer, sharded_batches, targets);
 *     }
 * }
 * ```
 */
class MultiGPULoRATrainer {
public:
    /**
     * @brief Training configuration
     */
    struct Config {
        float learning_rate = 0.001f;
        int gradient_accumulation_steps = 1;
        bool sync_every_step = true;  // If false, sync after accumulation
        int checkpoint_every_n_steps = 1000;
        std::string checkpoint_dir = "./checkpoints";
        bool enable_profiling = false;
    };
    
    /**
     * @brief Construct multi-GPU trainer
     * @param ctx Multi-GPU context
     * @param config Training configuration
     */
    explicit MultiGPULoRATrainer(const MultiGPUContext& ctx, 
                                 const Config& config);
    explicit MultiGPULoRATrainer(const MultiGPUContext& ctx);
    
    ~MultiGPULoRATrainer() = default;
    
    /**
     * @brief Create multi-GPU LoRA layer
     */
    std::shared_ptr<MultiGPULoRALayer> create_layer(
        size_t in_dim, size_t out_dim, size_t rank, float scaling,
        CommBackend backend = CommBackend::AUTO);
    
    /**
     * @brief Single training step
     * 
     * @param layer Multi-GPU layer
     * @param inputs Input tensors, one per GPU
     * @param targets Target tensors, one per GPU
     * @return Average loss across all GPUs
     */
    float train_step(
        MultiGPULoRALayer& layer,
        const std::vector<GPUTensor>& inputs,
        const std::vector<GPUTensor>& targets);
    
    /**
     * @brief Evaluation step (no gradient update)
     */
    float eval_step(
        MultiGPULoRALayer& layer,
        const std::vector<GPUTensor>& inputs,
        const std::vector<GPUTensor>& targets);
    
    /**
     * @brief Shard a batch across GPUs.
     *
     * Splits a single large batch into N smaller batches, one per GPU.
     *
     * @param batch Full batch tensor (batch_size, features).
     * @param ctx Multi-GPU context describing available devices.
     * @return Vector of sharded tensors, one per GPU.
     */
    static std::vector<GPUTensor> shard_batch(
        const GPUTensor& batch,
        const MultiGPUContext& ctx);
    
    /**
     * @brief Gather results from all GPUs to CPU
     */
    static GPUTensor gather_to_cpu(const std::vector<GPUTensor>& tensors);
    
    /**
     * @brief Save checkpoint (distributed)
     * 
     * Saves model parameters from rank 0 GPU only.
     */
    bool save_checkpoint(
        MultiGPULoRALayer& layer,
        const std::string& path,
        int step);
    
    /**
     * @brief Load checkpoint (broadcast to all GPUs)
     */
    bool load_checkpoint(
        MultiGPULoRALayer& layer,
        const std::string& path);
    
    /**
     * @brief Get training statistics
     */
    struct Stats {
        int total_steps = 0;
        float avg_loss = 0.0f;
        float avg_step_time_ms = 0.0f;
        float avg_communication_time_ms = 0.0f;
        float throughput_samples_per_sec = 0.0f;
        
        float communication_overhead() const {
            return avg_communication_time_ms / (avg_step_time_ms + 1e-6f);
        }
    };
    
    Stats get_stats() const { return stats_; }
    void reset_stats();
    
    /**
     * @brief Get multi-GPU context
     */
    const MultiGPUContext& context() const { return ctx_; }
    
private:
    const MultiGPUContext& ctx_;
    Config config_;
    Stats stats_;
    
    int current_step_ = 0;
    int accumulation_counter_ = 0;
    
    // Compute loss (MSE)
    float compute_loss(const GPUTensor& output, const GPUTensor& target);
    
    // Gradient descent step
    void update_parameters(MultiGPULoRALayer& layer);
};

} // namespace lora
} // namespace llm
} // namespace themis
