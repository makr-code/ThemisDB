/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ada_lora_adapter.h                                 ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:21:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     293                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7811d1486a  2026-03-27  feat: Enhance backward compatibility and legacy support a... ║
    • e25b25ef58  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace training {

/**
 * @file ada_lora_adapter.h
 * @brief AdaLoRA: Adaptive Budget Allocation via Singular Value Decomposition
 *
 * AdaLoRA (Zhang et al., 2023, arXiv:2303.10512) adaptively reallocates the
 * parameter budget among weight matrices during fine-tuning based on their
 * estimated importance, rather than using a fixed rank for all layers.
 *
 * This implementation approximates importance as the squared Frobenius norm of
 * the incremental weight matrix ΔW = (B @ A) × scaling.  Layers with higher
 * importance retain more rank; low-importance rank components are masked out
 * (pruned) during training.
 *
 * Key design points:
 *  - Each layer has an effective rank in [0, max_rank].
 *  - `updateImportance()` recomputes importance scores from the current weights.
 *  - `reallocateRanks(target_budget)` redistributes the global rank budget.
 *  - The forward pass applies only the active (non-pruned) rank components.
 *  - Pimpl pattern for ABI stability.
 *
 * Thread-safety: NOT guaranteed. Callers must provide external synchronisation.
 */

/**
 * @brief Per-layer rank allocation and importance score snapshot.
 */
struct AdaLoRALayerStats {
    std::string layer_name;     ///< Layer identifier
    size_t      max_rank   = 0; ///< Maximum allowed rank for this layer
    size_t      active_rank = 0;///< Currently active (unpruned) rank
    float       importance = 0.0f; ///< Estimated importance score (higher = more important)
};

/**
 * @brief Result of a rank-reallocation pass.
 */
struct ReallocResult {
    bool success = true;           ///< Legacy compatibility flag
    size_t total_active_rank = 0;   ///< Sum of active ranks after reallocation
    size_t layers_pruned     = 0;   ///< Layers whose rank was reduced
    size_t layers_expanded   = 0;   ///< Layers whose rank was increased
};

/**
 * @brief AdaLoRA adapter with importance-based rank pruning.
 *
 * Usage:
 * @code
 * AdaLoRAAdapter ada(4 /*default_rank*\/, 8.0f, 128 /*total_rank_budget*\/);
 * ada.addLayer("q_proj", 768, 768, 8);
 * ada.addLayer("v_proj", 768, 768, 8);
 * ada.addLayer("k_proj", 768, 768, 4);
 *
 * // After a training step, update importance and reallocate:
 * ada.updateImportance("q_proj");
 * ada.updateImportance("v_proj");
 * ada.updateImportance("k_proj");
 * ada.reallocateRanks(20);  // total budget of 20
 *
 * // Forward pass using active rank only:
 * auto out = ada.forward("q_proj", input, batch_size);
 * @endcode
 */
class AdaLoRAAdapter {
public:
    /**
     * @brief Construct AdaLoRA adapter.
     * @param default_rank   Default maximum rank per layer (> 0)
     * @param default_alpha  Default LoRA alpha (> 0)
     * @param rank_budget    Global rank budget used by reallocateRanks() (> 0)
     */
    explicit AdaLoRAAdapter(size_t default_rank   = 4,
                            float  default_alpha  = 8.0f,
                            size_t rank_budget    = 64);

    ~AdaLoRAAdapter();

    AdaLoRAAdapter(const AdaLoRAAdapter&)            = delete;
    AdaLoRAAdapter& operator=(const AdaLoRAAdapter&) = delete;

    // -------------------------------------------------------------------------
    // Layer management
    // -------------------------------------------------------------------------

    /**
     * @brief Register a new adapter layer.
     *
     * B is Kaiming-uniform initialised; A is zero-initialised.  The effective
     * rank starts at @p max_rank (or the adapter default if 0).
     *
     * @param layer_name  Unique identifier (must not already exist)
     * @param in_dim      Input feature dimension (> 0)
     * @param out_dim     Output feature dimension (> 0)
     * @param max_rank    Maximum rank for this layer (0 = use adapter default)
     * @param alpha       LoRA alpha override (0.0 = use adapter default)
     * @throws std::invalid_argument on bad dimensions or duplicate name
     */
    void addLayer(const std::string& layer_name,
                  size_t in_dim,
                  size_t out_dim,
                  size_t max_rank = 0,
                  float  alpha    = 0.0f);

    /**
     * @brief Remove a registered layer.
     * @return true if the layer existed and was removed; false otherwise.
     */
    bool removeLayer(const std::string& layer_name);

    /** @brief Whether a layer with the given name is registered. */
    bool hasLayer(const std::string& layer_name) const;

    /** @brief Names of all registered layers (order unspecified). */
    std::vector<std::string> layerNames() const;

    /** @brief Number of registered layers. */
    size_t layerCount() const;

    // -------------------------------------------------------------------------
    // Importance scoring and rank reallocation
    // -------------------------------------------------------------------------

    /**
     * @brief Recompute the importance score for a layer.
     *
     * Importance is estimated as the mean squared Frobenius norm of the rank-
     * component contributions: sum_i( ||B[:,i]||^2 * ||A[i,:]||^2 ) / active_rank.
     * This approximates the singular-value importance without a full SVD.
     *
     * @param layer_name  Target layer
     * @throws std::out_of_range if the layer does not exist
     */
    void updateImportance(const std::string& layer_name);

    /**
     * @brief Update importance for all registered layers.
     */
    void updateAllImportances();

    /**
     * @brief Reallocate rank budget across layers based on current importance scores.
     *
     * Distributes @p total_budget rank slots proportionally to each layer's
     * normalised importance score, subject to [1, max_rank] per-layer bounds.
     * Layers with zero importance receive rank 1 (minimum).
     *
     * @param total_budget  Global rank budget to distribute (> 0)
     * @return ReallocResult describing the changes made
     * @throws std::invalid_argument if total_budget is zero
     */
    ReallocResult reallocateRanks(size_t total_budget);

    /**
     * @brief Reallocate using the adapter's configured rank_budget.
     */
    ReallocResult reallocateRanks();

    // -------------------------------------------------------------------------
    // Rank and importance access
    // -------------------------------------------------------------------------

    /**
     * @brief Current active rank for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    size_t getActiveRank(const std::string& layer_name) const;

    /**
     * @brief Maximum rank for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    size_t getMaxRank(const std::string& layer_name) const;

    /**
     * @brief Current importance score for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    float getImportance(const std::string& layer_name) const;

    /**
     * @brief Snapshot of all layer statistics.
     * @return Vector of AdaLoRALayerStats (one per registered layer)
     */
    std::vector<AdaLoRALayerStats> getLayerStats() const;

    /**
     * @brief Total active parameter count (sum over layers of 2 * active_rank *
     *        (in_dim + out_dim) / 2).
     */
    size_t totalActiveParameterCount() const;

    // -------------------------------------------------------------------------
    // Weight access
    // -------------------------------------------------------------------------

    /**
     * @brief Overwrite B and A matrices for a layer.
     *
     * @param layer_name  Target layer
     * @param B           Flat row-major B data (in_dim × max_rank floats)
     * @param A           Flat row-major A data (max_rank × out_dim floats)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if the vector sizes do not match
     */
    void setWeights(const std::string&        layer_name,
                    const std::vector<float>& B,
                    const std::vector<float>& A);

    /**
     * @brief Read B and A weight matrices for a layer.
     * @throws std::out_of_range if the layer does not exist
     */
    std::pair<std::vector<float>, std::vector<float>>
    getWeights(const std::string& layer_name) const;

    // -------------------------------------------------------------------------
    // Forward pass
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the LoRA contribution using only the active rank components.
     *
     * Performs (using only the first active_rank columns of B / rows of A):
     *   hidden = input @ B[:, :active_rank]     (batch_size × active_rank)
     *   output = hidden @ A[:active_rank, :]    (batch_size × out_dim)
     *   return  output × scaling               where scaling = alpha / max_rank
     *
     * @param layer_name  Layer whose weights are applied
     * @param input       Flat row-major input (batch_size × in_dim)
     * @param batch_size  Number of rows in @p input
     * @return Flat row-major output (batch_size × out_dim)
     * @throws std::out_of_range    if the layer does not exist
     * @throws std::invalid_argument if input size != batch_size × in_dim
     */
    std::vector<float> forward(const std::string&        layer_name,
                               const std::vector<float>& input,
                               size_t                    batch_size) const;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** @brief Global rank budget used by reallocateRanks(). */
    size_t rankBudget() const;

    /** @brief Update the global rank budget. */
    void setRankBudget(size_t budget);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
