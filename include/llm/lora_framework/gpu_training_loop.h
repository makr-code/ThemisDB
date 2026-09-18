/**
 * @file gpu_training_loop.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct GPUTrainingMetrics {
    /**
     * @brief GPUTraining Metrics.
     * @return Return value.
     */
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

using GPUTrainingCallback = std::function<void(const GPUTrainingMetrics&)>;

struct GPUTrainingConfig {
    /**
     * @brief GPUTraining Config.
     * @return Return value.
     */
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

class GPUTrainingLoop {
public:
    /**
     * @brief GPUTraining Loop.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GPUTrainingLoop(const GPUTrainingConfig& config);
    
    ~GPUTrainingLoop();
    
    // Disable copy, allow move
    GPUTrainingLoop(const GPUTrainingLoop&) = delete;
    GPUTrainingLoop& operator=(const GPUTrainingLoop&) = delete;
    GPUTrainingLoop(GPUTrainingLoop&&) noexcept;
    GPUTrainingLoop& operator=(GPUTrainingLoop&&) noexcept;
    
    /**
     * @brief Set Data Loader.
     * @param[in] loader Input parameter.
     */
    void setDataLoader(std::unique_ptr<GPUDataLoader> loader);
    
    /**
     * @brief Add Layer.
     * @param[in,out] layer Input/output parameter.
     */
    void addLayer(GPULoRALayer* layer);
    
    /**
     * @brief Set Multi GPULayer.
     * @param[in,out] layer Input/output parameter.
     */
    void setMultiGPULayer(MultiGPULoRALayer* layer);
    
    /**
     * @brief Set Mixed Precision Trainer.
     * @param[in,out] trainer Input/output parameter.
     */
    void setMixedPrecisionTrainer(MixedPrecisionTrainer* trainer);
    
    /**
     * @brief Register Callback.
     * @param[in] callback Input parameter.
     */
    void registerCallback(GPUTrainingCallback callback);
    
    /**
     * @brief Train.
     * @return True when the operation succeeds.
     */
    bool train();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    bool isTraining() const { return is_training_.load(); }
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    GPUTrainingMetrics getMetrics() const;
    
    float getFinalLoss() const { return final_loss_; }
    
    /**
     * @brief Set Base Model.
     * @param[in] base_model Input parameter.
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
    /**
     * @brief Initialize Optimizer.
     */
    void initializeOptimizer();
    /**
     * @brief Initialize Memory Management.
     */
    void initializeMemoryManagement();
    /**
     * @brief Initialize Adaptive Batching.
     */
    void initializeAdaptiveBatching();
    /**
     * @brief Initialize Checkpointing.
     */
    void initializeCheckpointing();
    /**
     * @brief Train Epoch.
     * @param[in] epoch Input parameter.
     * @return Return value.
     */
    float trainEpoch(int epoch);
    /**
     * @brief Train Step.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    float trainStep(const GPUBatch& batch);
    /**
     * @brief Update Metrics.
     * @param[in] epoch Input parameter.
     * @param[in] step Input parameter.
     * @param[in] loss Input parameter.
     */
    void updateMetrics(int epoch, int step, float loss);
    /**
     * @brief Check Memory Usage.
     */
    void checkMemoryUsage();
};

GPUTensor createEmbeddingsOnGPU(
    const GPUTensor& token_ids,
    size_t hidden_dim,
    const Device& device,
    GPUEmbeddingLayer* embedding_layer = nullptr
);

/**
 * @brief Compute MSELoss GPU.
 * @param[in] predictions Input parameter.
 * @param[in] targets Input parameter.
 * @return Return value.
 */
float computeMSELossGPU(const GPUTensor& predictions, const GPUTensor& targets);

/**
 * @brief Compute MSEGradient GPU.
 * @param[in] predictions Input parameter.
 * @param[in] targets Input parameter.
 * @return Return value.
 */
GPUTensor computeMSEGradientGPU(const GPUTensor& predictions, const GPUTensor& targets);

/**
 * @brief Compute Fused MSELoss Gradient GPU.
 * @param[in] predictions Input parameter.
 * @param[in] targets Input parameter.
 * @param[in,out] grad_output Input/output parameter.
 * @return Return value.
 */
float computeFusedMSELossGradientGPU(
    const GPUTensor& predictions, 
    const GPUTensor& targets,
    GPUTensor& grad_output
);

} // namespace lora
} // namespace llm
} // namespace themis

