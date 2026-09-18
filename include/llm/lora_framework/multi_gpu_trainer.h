/**
 * @file multi_gpu_trainer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class MultiGPULoRATrainer {
public:
    struct Config {
        float learning_rate = 0.001f;
        int gradient_accumulation_steps = 1;
        bool sync_every_step = true;  // If false, sync after accumulation
        int checkpoint_every_n_steps = 1000;
        std::string checkpoint_dir = "./checkpoints";
        bool enable_profiling = false;
    };
    
    /**
     * @brief Multi GPULo RATrainer.
     * @param[in] ctx Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MultiGPULoRATrainer(const MultiGPUContext& ctx, 
                                 const Config& config);
    /**
     * @brief Multi GPULo RATrainer.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    explicit MultiGPULoRATrainer(const MultiGPUContext& ctx);
    
    ~MultiGPULoRATrainer() = default;
    
    std::shared_ptr<MultiGPULoRALayer> create_layer(
        size_t in_dim, size_t out_dim, size_t rank, float scaling,
        CommBackend backend = CommBackend::AUTO);
    
    /**
     * @brief Train step.
     * @param[in,out] layer Input/output parameter.
     * @param[in] inputs Input parameter.
     * @param[in] targets Input parameter.
     * @return Return value.
     */
    float train_step(
        MultiGPULoRALayer& layer,
        const std::vector<GPUTensor>& inputs,
        const std::vector<GPUTensor>& targets);
    
    /**
     * @brief Eval step.
     * @param[in,out] layer Input/output parameter.
     * @param[in] inputs Input parameter.
     * @param[in] targets Input parameter.
     * @return Return value.
     */
    float eval_step(
        MultiGPULoRALayer& layer,
        const std::vector<GPUTensor>& inputs,
        const std::vector<GPUTensor>& targets);
    
    /**
     * @brief Shard batch.
     * @param[in] batch Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    static std::vector<GPUTensor> shard_batch(
        const GPUTensor& batch,
        const MultiGPUContext& ctx);
    
    /**
     * @brief Gather to cpu.
     * @param[in] tensors Input parameter.
     * @return Return value.
     */
    static GPUTensor gather_to_cpu(const std::vector<GPUTensor>& tensors);
    
    /**
     * @brief Save checkpoint.
     * @param[in,out] layer Input/output parameter.
     * @param[in] path Input parameter.
     * @param[in] step Input parameter.
     * @return True when the operation succeeds.
     */
    bool save_checkpoint(
        MultiGPULoRALayer& layer,
        const std::string& path,
        int step);
    
    /**
     * @brief Load checkpoint.
     * @param[in,out] layer Input/output parameter.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool load_checkpoint(
        MultiGPULoRALayer& layer,
        const std::string& path);
    
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
    /**
     * @brief Reset stats.
     */
    void reset_stats();
    
    const MultiGPUContext& context() const { return ctx_; }
    
private:
    const MultiGPUContext& ctx_;
    Config config_;
    Stats stats_;
    
    int current_step_ = 0;
    int accumulation_counter_ = 0;
    
    // Compute loss (MSE)
    /**
     * @brief Compute loss.
     * @param[in] output Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    float compute_loss(const GPUTensor& output, const GPUTensor& target);
    
    // Gradient descent step
    /**
     * @brief Update parameters.
     * @param[in,out] layer Input/output parameter.
     */
    void update_parameters(MultiGPULoRALayer& layer);
};

} // namespace lora
} // namespace llm
} // namespace themis
