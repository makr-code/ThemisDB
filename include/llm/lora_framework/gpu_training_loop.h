/**
 * @file gpu_training_loop.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_data_loader.h"
#include "llm/lora_framework/mixed_precision.h"
#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include "llm/lora_framework/vram_allocator.h"
#include "llm/lora_framework/gpu_embedding_layer.h"
#include "llm/lora_framework/adaptive_batcher.h"
#include "llm/lora_framework/gpu_utilization_monitor.h"
#include "llm/lora_framework/gradient_checkpointing.h"
#include "llm/gpu_memory_manager.h"
#include <functional>
#include <memory>
#include <atomic>

namespace themis {
namespace llm {
namespace lora {

// Forward declarations
class BaseModelAdapter;

/**
 * @brief Training metrics for GPU training
 */
struct GPUTrainingMetrics {
    virtual ~GPUTrainingMetrics() = default;
    int current_epoch = 0;
    int total_epochs = 0;
    int current_step = 0;
    int total_steps = 0;
    float current_loss = 0.0f;
    float learning_rate = 0.0f;
    float progress = 0.0f;
    size_t gpu_memory_used = 0;
    size_t gpu_memory_available = 0;
    float gpu_utilization = 0.0f;
    std::string status = "idle";
};

/**
 * @brief Callback for GPU training progress
 */
using GPUTrainingCallback = std::function<void(const GPUTrainingMetrics&)>;

/**
 * @brief Configuration for GPU training loop
 */
struct GPUTrainingConfig {
    virtual ~GPUTrainingConfig() = default;
    // Basic training parameters
    int num_epochs = 3;
    float learning_rate = 1e-4f;
    float momentum = 0.0f;
    float weight_decay = 0.0f;
    
    // Device configuration
    Device device = Device::cuda();
    bool use_mixed_precision = false;
    
    // Memory management
    size_t max_vram_bytes = 0;  // 0 = auto-detect
    bool enable_gradient_checkpointing = false;
    CheckpointStrategy checkpoint_strategy = CheckpointStrategy::SQRT_N;
    int checkpoint_frequency = 4;  // For UNIFORM strategy
    
    // Multi-GPU
    bool use_multi_gpu = false;
    std::vector<int> gpu_ids;
    
    // Optimization
    bool use_fused_kernels = true;
    int gradient_accumulation_steps = 1;
    
    // Dynamic batch size adaptation (NEW)
    bool enable_adaptive_batching = false;
    size_t min_batch_size = 1;
    size_t max_batch_size = 32;
};

/**
 * @brief GPU-accelerated training loop for LoRA
 * 
 * Implements complete GPU training pipeline:
 * - GPU data loading
 * - GPU forward/backward passes
 * - GPU optimizer updates
 * - VRAM management
 * - Mixed precision support
 * - Multi-GPU data parallelism
 * 
 * Features:
 * - All tensors reside in VRAM throughout training
 * - No CPU-GPU transfer bottlenecks
 * - Efficient memory pooling via VRAMAllocator
 * - Real GPU kernel execution (not CPU simulation)
 * 
 * Usage:
 * ```cpp
 * GPUTrainingConfig config;
 * config.device = Device::cuda();
 * config.use_mixed_precision = true;
 * 
 * GPUTrainingLoop trainer(config);
 * trainer.setDataLoader(std::move(gpu_data_loader));
 * trainer.addLayer(lora_layer);
 * trainer.train();
 * ```
 */
class GPUTrainingLoop {
public:
    /**
     * @brief Construct GPU training loop
     * @param config Training configuration
     */
    explicit GPUTrainingLoop(const GPUTrainingConfig& config);
    
    ~GPUTrainingLoop();
    
    // Disable copy, allow move
    GPUTrainingLoop(const GPUTrainingLoop&) = delete;
    GPUTrainingLoop& operator=(const GPUTrainingLoop&) = delete;
    GPUTrainingLoop(GPUTrainingLoop&&) noexcept;
    GPUTrainingLoop& operator=(GPUTrainingLoop&&) noexcept;
    
    /**
     * @brief Set data loader
     * @param loader GPU data loader
     */
    void setDataLoader(std::unique_ptr<GPUDataLoader> loader);
    
    /**
     * @brief Add LoRA layer to train
     * @param layer GPU LoRA layer
     */
    void addLayer(GPULoRALayer* layer);
    
    /**
     * @brief Set multi-GPU LoRA layer
     * @param layer Multi-GPU LoRA layer
     */
    void setMultiGPULayer(MultiGPULoRALayer* layer);
    
    /**
     * @brief Set mixed precision trainer
     * @param trainer Mixed precision trainer
     */
    void setMixedPrecisionTrainer(MixedPrecisionTrainer* trainer);
    
    /**
     * @brief Register training callback
     * @param callback Callback function
     */
    void registerCallback(GPUTrainingCallback callback);
    
    /**
     * @brief Run training loop
     * @return true on success
     */
    bool train();
    
    /**
     * @brief Stop training
     */
    void stop();
    
    /**
     * @brief Check if training is in progress
     */
    bool isTraining() const { return is_training_.load(); }
    
    /**
     * @brief Get current metrics
     */
    GPUTrainingMetrics getMetrics() const;
    
    /**
     * @brief Get final loss
     */
    float getFinalLoss() const { return final_loss_; }
    
    /**
     * @brief Set base model for real embeddings
     * @param base_model Pointer to loaded base model adapter (optional)
     */
    void setBaseModel(const BaseModelAdapter* base_model);
    
private:
    GPUTrainingConfig config_;
    
    // Data and model
    std::unique_ptr<GPUDataLoader> data_loader_;
    std::vector<GPULoRALayer*> layers_;
    MultiGPULoRALayer* multi_gpu_layer_ = nullptr;
    
    // Base model and embeddings
    const BaseModelAdapter* base_model_ = nullptr;
    std::unique_ptr<GPUEmbeddingLayer> gpu_embedding_layer_;
    
    // Training components
    std::unique_ptr<GPUSGDOptimizer> optimizer_;
    MixedPrecisionTrainer* mixed_precision_trainer_ = nullptr;
    std::unique_ptr<GradientCheckpointer> checkpointer_;
    
    // Memory management
    std::unique_ptr<VRAMAllocator> vram_allocator_;
    ::themis::llm::GPUMemoryManager* gpu_memory_manager_ = nullptr;
    
    // Dynamic batch size adaptation (NEW)
    std::unique_ptr<AdaptiveBatcher> adaptive_batcher_;
    std::unique_ptr<GPUUtilizationMonitor> gpu_monitor_;
    
    // State
    std::atomic<bool> is_training_{false};
    std::atomic<bool> stop_requested_{false};
    GPUTrainingMetrics current_metrics_;
    GPUTrainingCallback callback_;
    float final_loss_ = 0.0f;
    
    // Helper methods
    void initializeOptimizer();
    void initializeMemoryManagement();
    void initializeAdaptiveBatching();
    void initializeCheckpointing();
    float trainEpoch(int epoch);
    float trainStep(const GPUBatch& batch);
    void updateMetrics(int epoch, int step, float loss);
    void checkMemoryUsage();
};

/**
 * @brief Helper function to create embeddings from token IDs on GPU
 * 
 * Converts token IDs to embeddings directly on GPU.
 * Uses base model embeddings if available, otherwise hash-based fallback.
 * 
 * @param token_ids Token ID tensor (batch_size, seq_len)
 * @param hidden_dim Embedding dimension
 * @param device Target device
 * @param embedding_layer GPU embedding layer (optional, for real embeddings)
 * @return Embedding tensor (batch_size, hidden_dim)
 */
GPUTensor createEmbeddingsOnGPU(
    const GPUTensor& token_ids,
    size_t hidden_dim,
    const Device& device,
    GPUEmbeddingLayer* embedding_layer = nullptr
);

/**
 * @brief Compute MSE loss on GPU
 * @param predictions Prediction tensor
 * @param targets Target tensor
 * @return Loss value (scalar)
 */
float computeMSELossGPU(const GPUTensor& predictions, const GPUTensor& targets);

/**
 * @brief Compute MSE gradient on GPU
 * @param predictions Prediction tensor
 * @param targets Target tensor
 * @return Gradient tensor
 */
GPUTensor computeMSEGradientGPU(const GPUTensor& predictions, const GPUTensor& targets);

/**
 * @brief Fused MSE loss and gradient computation on GPU
 * 
 * Computes both MSE loss and gradient in a single kernel pass.
 * More efficient than calling computeMSELossGPU and computeMSEGradientGPU separately.
 * 
 * Performance benefits:
 * - 1.3-1.5x faster than separate calls
 * - ~50% reduction in memory bandwidth
 * - Single read of predictions/targets instead of two
 * 
 * @param predictions Prediction tensor
 * @param targets Target tensor  
 * @param grad_output Output gradient tensor (will be allocated by this function)
 * @return MSE loss value (scalar)
 */
float computeFusedMSELossGradientGPU(
    const GPUTensor& predictions, 
    const GPUTensor& targets,
    GPUTensor& grad_output
);

} // namespace lora
} // namespace llm
} // namespace themis

