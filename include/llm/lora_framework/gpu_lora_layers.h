/**
 * @file gpu_lora_layers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include <vector>
#include <memory>
#include <string>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

class GPULoRALayer {
public:
    GPULoRALayer(size_t in_dim, size_t out_dim, size_t rank, 
                 float scaling = 1.0f,
                 const Device& device = Device::cpu(),
                 bool use_fused_kernels = true,
                 bool use_flash_lora = false);
    
    ~GPULoRALayer() = default;
    
    // Disable copy, enable move
    GPULoRALayer(const GPULoRALayer&) = delete;
    GPULoRALayer& operator=(const GPULoRALayer&) = delete;
    GPULoRALayer(GPULoRALayer&&) noexcept = default;
    GPULoRALayer& operator=(GPULoRALayer&&) noexcept = default;
    
    /**
     * @brief ========== Forward/Backward ==========
     * @param[in] input Input parameter.
     * @return Return value.
     */
    
    GPUTensor forward(const GPUTensor& input);
    
    /**
     * @brief Backward.
     * @param[in] grad_output Input parameter.
     * @return Return value.
     */
    GPUTensor backward(const GPUTensor& grad_output);
    
    /**
     * @brief ========== Parameter Access ==========
     * @return Return value.
     */
    
    std::vector<GPUTensor*> parameters();
    
    /**
     * @brief Gradients.
     * @return Return value.
     */
    std::vector<GPUTensor*> gradients();
    
    /**
     * @brief Zero grad.
     */
    void zero_grad();
    
    // ========== Weight Export/Import ==========
    
    std::pair<GPUTensor, GPUTensor> get_weights() const;
    
    /**
     * @brief Set weights.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     */
    void set_weights(const GPUTensor& B, const GPUTensor& A);
    
    // ========== Device Management ==========
    
    Device device() const { return device_; }
    
    /**
     * @brief To.
     * @param[in] target_device Input parameter.
     */
    void to(const Device& target_device);
    
    // ========== Layer Metadata ==========
    
    const std::string& name() const { return name_; }
    /**
     * @brief Set name.
     * @param[in] name Input parameter.
     * @details Implements set_name without additional internal calls.
     */
    void set_name(const std::string& name) { name_ = name; }
    
    size_t parameter_count() const { return in_dim_ * rank_ + rank_ * out_dim_; }
    size_t memory_bytes() const { return parameter_count() * sizeof(float); }
    
    size_t in_dim() const { return in_dim_; }
    size_t out_dim() const { return out_dim_; }
    size_t rank() const { return rank_; }
    float scaling() const { return scaling_; }
    bool use_fused_kernels() const { return use_fused_kernels_; }
    /**
     * @brief Set use fused kernels.
     * @param[in] use_fused Input parameter.
     * @details Implements set_use_fused_kernels without additional internal calls.
     */
    void set_use_fused_kernels(bool use_fused) { use_fused_kernels_ = use_fused; }
    bool use_flash_lora() const { return use_flash_lora_; }
    /**
     * @brief Set use flash lora.
     * @param[in] use_flash Input parameter.
     * @details Implements set_use_flash_lora without additional internal calls.
     */
    void set_use_flash_lora(bool use_flash) { use_flash_lora_ = use_flash; }
    
    /**
     * @brief ========== Gradient Checkpointing ==========
     * @param[in] enable Input parameter.
     * @details Implements set_checkpointing without additional internal calls.
     */
    
    void set_checkpointing(bool enable) { use_checkpointing_ = enable; }
    
    bool use_checkpointing() const { return use_checkpointing_; }
    
    /**
     * @brief Set layer id.
     * @param[in] layer_id Identifier of the layer.
     * @details Implements set_layer_id without additional internal calls.
     */
    void set_layer_id(int layer_id) { layer_id_ = layer_id; }
    
    int layer_id() const { return layer_id_; }

private:
    std::string name_ = "GPULoRALayer";
    size_t in_dim_ = 0;
    size_t out_dim_ = 0;
    size_t rank_ = 0;
    float scaling_ = 0.0f;
    Device device_;
    bool use_fused_kernels_ = false;
    bool use_flash_lora_ = false;
    
    // Gradient checkpointing
    bool use_checkpointing_ = false;
    int layer_id_ = -1;
    
    // Trainable parameters (in VRAM)
    std::unique_ptr<GPUTensor> B_;  // (in_dim, rank)
    std::unique_ptr<GPUTensor> A_;  // (rank, out_dim)
    
    // Cached for backward pass (in VRAM)
    // Note: When checkpointing is enabled, these are NOT cached
    GPUTensor cached_input_;   // Input from forward pass
    GPUTensor cached_h_;       // Intermediate: input @ B
};

class GPUSGDOptimizer {
public:
    explicit GPUSGDOptimizer(float learning_rate = 0.001f, 
                             float momentum = 0.0f, 
                             float weight_decay = 0.0f);
    
    /**
     * @brief Add parameters.
     * @param[in] params Input parameter.
     */
    void add_parameters(const std::vector<GPUTensor*>& params);
    
    /**
     * @brief Step.
     */
    void step();
    
    /**
     * @brief Zero grad.
     */
    void zero_grad();
    
    // ========== Getters/Setters ==========
    
    float learning_rate() const { return learning_rate_; }
    /**
     * @brief Set learning rate.
     * @param[in] lr Input parameter.
     * @details Implements set_learning_rate without additional internal calls.
     */
    void set_learning_rate(float lr) { learning_rate_ = lr; }
    
    float momentum() const { return momentum_; }
    /**
     * @brief Set momentum.
     * @param[in] m Input parameter.
     * @details Implements set_momentum without additional internal calls.
     */
    void set_momentum(float m) { momentum_ = m; }
    
    float weight_decay() const { return weight_decay_; }
    /**
     * @brief Set weight decay.
     * @param[in] wd Input parameter.
     * @details Implements set_weight_decay without additional internal calls.
     */
    void set_weight_decay(float wd) { weight_decay_ = wd; }
    
    size_t num_parameters() const { return parameters_.size(); }

private:
    float learning_rate_ = 0.0f;
    float momentum_ = 0.0f;
    float weight_decay_ = 0.0f;
    std::vector<GPUTensor*> parameters_;
    
    // Momentum buffers (in VRAM, only allocated if momentum > 0)
    std::vector<std::unique_ptr<GPUTensor>> momentum_buffers_;
};

class GPULoRATrainer {
public:
    GPULoRATrainer(GPULoRALayer* layer, GPUSGDOptimizer* optimizer);
    
    /**
     * @brief Train step.
     * @param[in] input Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    float train_step(const GPUTensor& input, const GPUTensor& target);
    
    /**
     * @brief Eval step.
     * @param[in] input Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    float eval_step(const GPUTensor& input, const GPUTensor& target);

private:
    GPULoRALayer* layer_;
    GPUSGDOptimizer* optimizer_;
    
    /**
     * @brief MSE loss computation (GPU-accelerated)
     * @param[in] output Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    float compute_mse_loss(const GPUTensor& output, const GPUTensor& target);
    /**
     * @brief Compute mse grad.
     * @param[in] output Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    GPUTensor compute_mse_grad(const GPUTensor& output, const GPUTensor& target);
};

} // namespace lora
} // namespace llm
} // namespace themis
