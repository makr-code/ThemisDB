/**
 * @file gradient_checkpointing.h
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
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <functional>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Checkpoint strategy for gradient checkpointing
 * 
 * Research-based strategies from:
 * - Chen et al. (2016): "Training Deep Nets with Sublinear Memory Cost"
 * - Jain et al. (2020): "Checkmate: Breaking the Memory Wall"
 */
enum class CheckpointStrategy {
    NONE,           ///< No checkpointing
    UNIFORM,        ///< Checkpoint every N layers
    SQRT_N,         ///< Checkpoint every √n layers (optimal for memory)
    ATTENTION_ONLY, ///< Checkpoint attention layers only
    CUSTOM          ///< User-defined checkpoint points
};

/**
 * @brief Layer type for selective checkpointing
 */
enum class LayerType {
    UNKNOWN,
    ATTENTION,
    FFN,
    LORA,
    EMBEDDING
};

/**
 * @brief Configuration for gradient checkpointing
 */
struct CheckpointConfig {
    CheckpointStrategy strategy = CheckpointStrategy::SQRT_N;
    int checkpoint_frequency = 0;  ///< For UNIFORM strategy
    bool checkpoint_attention = true;
    bool checkpoint_ffn = false;
    bool checkpoint_lora = true;  ///< Checkpoint LoRA layers
    
    // Total layers (required for SQRT_N strategy)
    int total_layers = 0;
};

/**
 * @brief Statistics for gradient checkpointing
 */
struct CheckpointStats {
    size_t memory_saved_bytes = 0;
    size_t recomputation_time_ms = 0;
    float memory_reduction_pct = 0.0f;
    size_t num_checkpoints = 0;
    size_t total_layers = 0;
    float compute_overhead_pct = 0.0f;
};

/**
 * @brief Forward computation function for recomputation
 */
using ForwardFunction = std::function<GPUTensor(const GPUTensor&)>;

/**
 * @brief Checkpoint data for a layer
 */
struct CheckpointData {
    CheckpointData() = default;
    ~CheckpointData() = default;
    CheckpointData(const CheckpointData&) = delete;
    CheckpointData& operator=(const CheckpointData&) = delete;
    CheckpointData(CheckpointData&&) noexcept noexcept = default;
    CheckpointData& operator=(CheckpointData&&) noexcept noexcept = default;
    GPUTensor input;                  ///< Input tensor to the layer
    ForwardFunction forward_fn;       ///< Function to recompute forward pass
    size_t activation_size_bytes = 0; ///< Size of activation memory saved
};

/**
 * @brief Gradient Checkpointer for memory-efficient training
 * 
 * Implements gradient checkpointing (activation checkpointing) to reduce
 * memory usage during training by selectively discarding and recomputing
 * activations during the backward pass.
 * 
 * Trade-off:
 * - Memory: 50-80% reduction in activation memory
 * - Compute: 20-30% increase in training time
 * 
 * Research Background:
 * - Chen et al. (2016): Sublinear memory cost with O(√n) checkpoints
 * - Jain et al. (2020): Optimal tensor rematerialization strategies
 * 
 * Usage:
 * ```cpp
 * CheckpointConfig config;
 * config.strategy = CheckpointStrategy::SQRT_N;
 * config.total_layers = 32;
 * 
 * GradientCheckpointer checkpointer(config);
 * 
 * // In forward pass:
 * if (checkpointer.shouldCheckpoint(layer_id)) {
 *     checkpointer.saveCheckpoint(layer_id, input, forward_fn);
 * }
 * 
 * // In backward pass:
 * if (checkpointer.hasCheckpoint(layer_id)) {
 *     GPUTensor recomputed = checkpointer.recomputeActivation(layer_id);
 * }
 * ```
 */
class GradientCheckpointer {
public:
    /**
     * @brief Construct gradient checkpointer
     * @param config Checkpoint configuration
     */
    explicit GradientCheckpointer(const CheckpointConfig& config);
    
    /**
     * @brief Determine if a layer should be checkpointed
     * @param layer_id Layer identifier (0-indexed)
     * @param layer_type Optional layer type for strategy-specific decisions
     * @return true if layer should be checkpointed
     */
    bool shouldCheckpoint(int layer_id, LayerType layer_type = LayerType::UNKNOWN) const;
    
    /**
     * @brief Save checkpoint for a layer
     * @param layer_id Layer identifier
     * @param input Input tensor to the layer
     * @param forward_fn Function to recompute forward pass
     */
    void saveCheckpoint(int layer_id, const GPUTensor& input, ForwardFunction forward_fn);
    
    /**
     * @brief Check if layer has a checkpoint
     * @param layer_id Layer identifier
     * @return true if checkpoint exists
     */
    bool hasCheckpoint(int layer_id) const;
    
    /**
     * @brief Recompute activation for a checkpointed layer
     * @param layer_id Layer identifier
     * @return Recomputed activation tensor
     */
    GPUTensor recomputeActivation(int layer_id);
    
    /**
     * @brief Clear checkpoint for a layer (free memory)
     * @param layer_id Layer identifier
     */
    void clearCheckpoint(int layer_id);
    
    /**
     * @brief Clear all checkpoints
     */
    void clearAll();
    
    /**
     * @brief Add custom checkpoint layer
     * @param layer_id Layer to checkpoint
     */
    void addCustomCheckpoint(int layer_id);
    
    /**
     * @brief Set layer type information
     * @param layer_id Layer identifier
     * @param type Layer type
     */
    void setLayerType(int layer_id, LayerType type);
    
    /**
     * @brief Get checkpoint statistics
     * @return Current statistics
     */
    CheckpointStats getStats() const;
    
    /**
     * @brief Estimate memory savings
     * @param avg_activation_size Average activation size per layer in bytes
     * @return Estimated memory saved in bytes
     */
    size_t estimateMemorySavings(size_t avg_activation_size) const;
    
    /**
     * @brief Estimate compute overhead
     * @return Estimated overhead as percentage (0.0 - 100.0)
     */
    float estimateComputeOverhead() const;
    
    /**
     * @brief Update statistics after recomputation
     * @param recompute_time_ms Time spent recomputing in milliseconds
     */
    void updateRecomputeTime(size_t recompute_time_ms);
    
    /**
     * @brief Get configuration
     */
    const CheckpointConfig& config() const { return config_; }
    
private:
    CheckpointConfig config_;
    std::unordered_map<int, CheckpointData> checkpoints_;
    std::unordered_map<int, LayerType> layer_types_;
    std::unordered_set<int> custom_checkpoints_;
    mutable CheckpointStats stats_;
    
    /**
     * @brief Calculate checkpoint interval for SQRT_N strategy
     */
    int calculateSqrtNInterval() const;
};

} // namespace lora
} // namespace llm
} // namespace themis

