/**
 * @file gradient_checkpointing.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

enum class CheckpointStrategy {
    NONE,           ///< No checkpointing
    UNIFORM,        ///< Checkpoint every N layers
    SQRT_N,         ///< Checkpoint every √n layers (optimal for memory)
    ATTENTION_ONLY, ///< Checkpoint attention layers only
    CUSTOM          ///< User-defined checkpoint points
};

enum class LayerType {
    UNKNOWN,
    ATTENTION,
    FFN,
    LORA,
    EMBEDDING
};

struct CheckpointConfig {
    CheckpointStrategy strategy = CheckpointStrategy::SQRT_N;
    int checkpoint_frequency = 0;  ///< For UNIFORM strategy
    bool checkpoint_attention = true;
    bool checkpoint_ffn = false;
    bool checkpoint_lora = true;  ///< Checkpoint LoRA layers
    
    // Total layers (required for SQRT_N strategy)
    int total_layers = 0;
};

struct CheckpointStats {
    size_t memory_saved_bytes = 0;
    size_t recomputation_time_ms = 0;
    float memory_reduction_pct = 0.0f;
    size_t num_checkpoints = 0;
    size_t total_layers = 0;
    float compute_overhead_pct = 0.0f;
};

using ForwardFunction = std::function<GPUTensor(const GPUTensor&)>;

struct CheckpointData {
    CheckpointData() = default;
    ~CheckpointData() = default;
    CheckpointData(const CheckpointData&) = delete;
    CheckpointData& operator=(const CheckpointData&) = delete;
    CheckpointData(CheckpointData&&) noexcept = default;
    CheckpointData& operator=(CheckpointData&&) noexcept = default;
    GPUTensor input;                  ///< Input tensor to the layer
    ForwardFunction forward_fn;       ///< Function to recompute forward pass
    size_t activation_size_bytes = 0; ///< Size of activation memory saved
};

class GradientCheckpointer {
public:
    /**
     * @brief Gradient Checkpointer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GradientCheckpointer(const CheckpointConfig& config);
    
    bool shouldCheckpoint(int layer_id, LayerType layer_type = LayerType::UNKNOWN) const;
    
    /**
     * @brief Save Checkpoint.
     * @param[in] layer_id Identifier of the layer.
     * @param[in] input Input parameter.
     * @param[in] forward_fn Input parameter.
     */
    void saveCheckpoint(int layer_id, const GPUTensor& input, ForwardFunction forward_fn);
    
    /**
     * @brief Has Checkpoint.
     * @param[in] layer_id Identifier of the layer.
     * @return True when the operation succeeds.
     */
    bool hasCheckpoint(int layer_id) const;
    
    /**
     * @brief Recompute Activation.
     * @param[in] layer_id Identifier of the layer.
     * @return Return value.
     */
    GPUTensor recomputeActivation(int layer_id);
    
    /**
     * @brief Clear Checkpoint.
     * @param[in] layer_id Identifier of the layer.
     */
    void clearCheckpoint(int layer_id);
    
    /**
     * @brief Clear All.
     */
    void clearAll();
    
    /**
     * @brief Add Custom Checkpoint.
     * @param[in] layer_id Identifier of the layer.
     */
    void addCustomCheckpoint(int layer_id);
    
    /**
     * @brief Set Layer Type.
     * @param[in] layer_id Identifier of the layer.
     * @param[in] type Input parameter.
     */
    void setLayerType(int layer_id, LayerType type);
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    CheckpointStats getStats() const;
    
    /**
     * @brief Estimate Memory Savings.
     * @param[in] avg_activation_size Input parameter.
     * @return Return value.
     */
    size_t estimateMemorySavings(size_t avg_activation_size) const;
    
    /**
     * @brief Estimate Compute Overhead.
     * @return Return value.
     */
    float estimateComputeOverhead() const;
    
    /**
     * @brief Update Recompute Time.
     * @param[in] recompute_time_ms Input parameter.
     */
    void updateRecomputeTime(size_t recompute_time_ms);
    
    const CheckpointConfig& config() const { return config_; }
    
private:
    CheckpointConfig config_;
    std::unordered_map<int, CheckpointData> checkpoints_;
    std::unordered_map<int, LayerType> layer_types_;
    std::unordered_set<int> custom_checkpoints_;
    mutable CheckpointStats stats_;
    
    /**
     * @brief Calculate Sqrt NInterval.
     * @return Return value.
     */
    int calculateSqrtNInterval() const;
};

} // namespace lora
} // namespace llm
} // namespace themis

