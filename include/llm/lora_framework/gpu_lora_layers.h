/**
 * @file gpu_lora_layers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief GPU-accelerated LoRA Layer
 * 
 * Implements Low-Rank Adaptation with GPU support.
 * All tensors reside in VRAM for maximum performance.
 * 
 * Forward: output = input @ (B @ A) * scaling
 * Backward: Computes gradients for B, A, and input
 * 
 * B: (in_dim, rank) - Trainable
 * A: (rank, out_dim) - Trainable
 */
class GPULoRALayer {
public:
    /**
     * @brief Construct GPU LoRA layer
     * @param in_dim Input dimension
     * @param out_dim Output dimension
     * @param rank Rank of low-rank decomposition (typically 4, 8, 16, 32)
     * @param scaling Scaling factor (default: 1.0)
     * @param device Target device (CPU, CUDA, HIP, Vulkan, DirectX)
     * @param use_fused_kernels Enable kernel fusion optimization (default: true)
     * @param use_flash_lora Enable FlashLoRA memory-efficient computation (default: false, CUDA only)
     */
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
    
    // ========== Forward/Backward ==========
    
    /**
     * @brief Forward pass (GPU-accelerated)
     * @param input Input tensor (batch_size, in_dim)
     * @return Output tensor (batch_size, out_dim)
     * 
     * Computation: output = input @ B @ A * scaling
     * All operations execute on GPU if input is on GPU
     */
    GPUTensor forward(const GPUTensor& input);
    
    /**
     * @brief Backward pass (GPU-accelerated)
     * @param grad_output Gradient from next layer (batch_size, out_dim)
     * @return Gradient w.r.t. input (batch_size, in_dim)
     * 
     * Computes:
     * - grad_A = cached_h^T @ grad_output
     * - grad_B = cached_input^T @ (grad_output @ A^T)
     * - grad_input = (grad_output @ A^T) @ B^T
     */
    GPUTensor backward(const GPUTensor& grad_output);
    
    // ========== Parameter Access ==========
    
    /**
     * @brief Get trainable parameters
     * @return Vector of pointers to B and A tensors
     */
    std::vector<GPUTensor*> parameters();
    
    /**
     * @brief Get parameter gradients
     * @return Vector of pointers to B.grad and A.grad
     */
    std::vector<GPUTensor*> gradients();
    
    /**
     * @brief Zero out all gradients
     */
    void zero_grad();
    
    // ========== Weight Export/Import ==========
    
    /**
     * @brief Export weights (for storage/checkpointing)
     * @return Pair of (B, A) tensors
     */
    std::pair<GPUTensor, GPUTensor> get_weights() const;
    
    /**
     * @brief Import weights (for loading checkpoints)
     * @param B B matrix (in_dim, rank)
     * @param A A matrix (rank, out_dim)
     */
    void set_weights(const GPUTensor& B, const GPUTensor& A);
    
    // ========== Device Management ==========
    
    /**
     * @brief Get current device
     */
    Device device() const { return device_; }
    
    /**
     * @brief Move layer to different device
     * @param target_device Destination device
     */
    void to(const Device& target_device);
    
    // ========== Layer Metadata ==========
    
    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }
    
    size_t parameter_count() const { return in_dim_ * rank_ + rank_ * out_dim_; }
    size_t memory_bytes() const { return parameter_count() * sizeof(float); }
    
    size_t in_dim() const { return in_dim_; }
    size_t out_dim() const { return out_dim_; }
    size_t rank() const { return rank_; }
    float scaling() const { return scaling_; }
    bool use_fused_kernels() const { return use_fused_kernels_; }
    void set_use_fused_kernels(bool use_fused) { use_fused_kernels_ = use_fused; }
    bool use_flash_lora() const { return use_flash_lora_; }
    void set_use_flash_lora(bool use_flash) { use_flash_lora_ = use_flash; }
    
    // ========== Gradient Checkpointing ==========
    
    /**
     * @brief Enable gradient checkpointing for this layer
     * @param enable Enable or disable checkpointing
     */
    void set_checkpointing(bool enable) { use_checkpointing_ = enable; }
    
    /**
     * @brief Check if checkpointing is enabled
     */
    bool use_checkpointing() const { return use_checkpointing_; }
    
    /**
     * @brief Set layer ID for checkpointing
     */
    void set_layer_id(int layer_id) { layer_id_ = layer_id; }
    
    /**
     * @brief Get layer ID
     */
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

/**
 * @brief GPU-accelerated SGD Optimizer
 * 
 * Performs parameter updates directly in GPU VRAM.
 * Supports momentum and weight decay.
 */
class GPUSGDOptimizer {
public:
    /**
     * @brief Construct GPU SGD optimizer
     * @param learning_rate Learning rate (default: 0.001)
     * @param momentum Momentum factor (default: 0.0, range: [0, 1))
     * @param weight_decay Weight decay (L2 penalty, default: 0.0)
     */
    explicit GPUSGDOptimizer(float learning_rate = 0.001f, 
                             float momentum = 0.0f, 
                             float weight_decay = 0.0f);
    
    /**
     * @brief Register parameters to optimize
     * @param params Vector of parameter tensors
     * 
     * All parameters must be on the same device.
     */
    void add_parameters(const std::vector<GPUTensor*>& params);
    
    /**
     * @brief Perform optimization step (GPU-accelerated)
     * 
     * For each parameter p with gradient g:
     * If momentum > 0:
     *   v = momentum * v + (1 - momentum) * g
     *   p = p - lr * (v + weight_decay * p)
     * Else:
     *   p = p - lr * (g + weight_decay * p)
     * 
     * All operations execute on GPU if parameters are on GPU.
     */
    void step();
    
    /**
     * @brief Zero out all gradients
     */
    void zero_grad();
    
    // ========== Getters/Setters ==========
    
    float learning_rate() const { return learning_rate_; }
    void set_learning_rate(float lr) { learning_rate_ = lr; }
    
    float momentum() const { return momentum_; }
    void set_momentum(float m) { momentum_ = m; }
    
    float weight_decay() const { return weight_decay_; }
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

/**
 * @brief GPU Training Loop Helper
 * 
 * Simplifies GPU-accelerated LoRA training.
 */
class GPULoRATrainer {
public:
    GPULoRATrainer(GPULoRALayer* layer, GPUSGDOptimizer* optimizer);
    
    /**
     * @brief Single training step
     * @param input Input batch (batch_size, in_dim)
     * @param target Target batch (batch_size, out_dim)
     * @return Loss value
     * 
     * Performs: forward → loss → backward → optimizer.step()
     */
    float train_step(const GPUTensor& input, const GPUTensor& target);
    
    /**
     * @brief Validation step (no gradient computation)
     * @param input Input batch
     * @param target Target batch
     * @return Loss value
     */
    float eval_step(const GPUTensor& input, const GPUTensor& target);

private:
    GPULoRALayer* layer_;
    GPUSGDOptimizer* optimizer_;
    
    // MSE loss computation (GPU-accelerated)
    float compute_mse_loss(const GPUTensor& output, const GPUTensor& target);
    GPUTensor compute_mse_grad(const GPUTensor& output, const GPUTensor& target);
};

} // namespace lora
} // namespace llm
} // namespace themis
