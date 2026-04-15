/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_adapter_merger.h                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 05:38:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     215                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e25b25ef58  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/lora_adapter.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace training {

/**
 * @file lora_adapter_merger.h
 * @brief LoRA adapter composition: linear merge and TIES merging.
 *
 * Two merge strategies are provided:
 *
 * ### Linear Merge
 * Computes ΔW_merged = Σ_i( w_i × (B_i @ A_i) × scaling_i ) and then
 * factorises the result back into a single B and A via a rank-1 SVD
 * approximation (outer product of the dominant left/right singular vectors
 * scaled by the largest singular value).  This matches the linear combination
 * approach used in practice for adapter stacking.
 *
 * ### TIES Merge (Yadav et al., 2023)
 * "TIES-Merging: Resolving Interference When Merging Models"
 * arXiv:2306.01708
 *
 * Three-step algorithm applied to each weight element across adapters:
 *  1. **Trim**: Zero out weight deltas whose absolute value is below a
 *     per-adapter threshold (fraction of the max absolute value).
 *  2. **Resolve**: For each element, take the sign that appears in the
 *     majority of adapters (element-wise majority vote).
 *  3. **Merge**: Average only the weight values that agree with the resolved
 *     sign; ignore the others.
 *
 * Both strategies produce a new `LoRAAdapter` with a single merged layer per
 * input layer name.
 *
 * Thread-safety: NOT guaranteed. Callers must provide external synchronisation.
 */

/**
 * @brief Descriptor of one adapter participating in a merge.
 */
struct AdapterDescriptor {
    const LoRAAdapter* adapter = nullptr; ///< Pointer to the adapter (non-owning)
    std::string        layer_name;        ///< Layer name within the adapter
    float              weight = 1.0f;     ///< Blend weight (used by linear merge)
};

/**
 * @brief Result of a merge operation for a single output layer.
 */
struct MergeLayerResult {
    std::string        layer_name;
    std::vector<float> B;           ///< Merged B matrix (in_dim × rank)
    std::vector<float> A;           ///< Merged A matrix (rank × out_dim)
    bool               success = false;
    std::string        error_message;
};

/**
 * @brief Summary of a full merge pass.
 */
struct MergeResult {
    std::vector<MergeLayerResult> layers;
    size_t layers_merged  = 0;
    size_t layers_failed  = 0;
    bool   success        = false;
    std::string error_message;
};

/**
 * @brief LoRA adapter merger supporting linear and TIES merge strategies.
 *
 * Usage (linear merge):
 * @code
 * LoRAAdapter a1(4, 8.0f), a2(4, 8.0f);
 * a1.addLayer("q_proj", 768, 768, 4, 8.0f);
 * a2.addLayer("q_proj", 768, 768, 4, 8.0f);
 * // ... populate weights ...
 *
 * LoRAAdapterMerger merger;
 * auto result = merger.mergeLinear(
 *     {{"q_proj", &a1, "q_proj", 0.6f},
 *      {"q_proj", &a2, "q_proj", 0.4f}},
 *     "q_proj", 768, 768, 4, 8.0f);
 * @endcode
 *
 * Usage (TIES merge):
 * @code
 * auto result = merger.mergeTIES(
 *     {{"q_proj", &a1, "q_proj", 1.0f},
 *      {"q_proj", &a2, "q_proj", 1.0f}},
 *     "q_proj", 768, 768, 4, 8.0f,
 *     0.2f /*trim_threshold*\/);
 * @endcode
 */
class LoRAAdapterMerger {
public:
    LoRAAdapterMerger() = default;
    ~LoRAAdapterMerger() = default;

    // -------------------------------------------------------------------------
    // Linear merge
    // -------------------------------------------------------------------------

    /**
     * @brief Linearly combine multiple LoRA adapters for a single output layer.
     *
     * Computes the merged weight delta as a weighted sum of the individual
     * ΔW = (B @ A) × scaling contributions, then factorises back to (B', A').
     *
     * @param adapters      Adapters to merge (all must have the given layer name)
     * @param out_layer     Name for the output layer in the returned result
     * @param in_dim        Input dimension of the layer
     * @param out_dim       Output dimension of the layer
     * @param rank          Rank for the output B' and A' matrices
     * @param alpha         LoRA alpha for the output layer
     * @return MergeLayerResult with merged B and A (in_dim×rank, rank×out_dim)
     */
    MergeLayerResult mergeLinear(const std::vector<AdapterDescriptor>& adapters,
                                 const std::string& out_layer,
                                 size_t in_dim,
                                 size_t out_dim,
                                 size_t rank,
                                 float  alpha = 8.0f) const;

    /**
     * @brief Linearly merge all layers shared by all given adapters.
     *
     * Iterates over the layer names present in the first adapter and merges
     * each layer that exists in all adapters.
     *
     * @param adapters       Collection of adapters to merge (all non-null)
     * @param weights        Per-adapter blend weights (must match adapters.size())
     * @param output_rank    Rank for all output layers
     * @return MergeResult describing the merged layers
     */
    MergeResult mergeLinearAll(const std::vector<const LoRAAdapter*>& adapters,
                               const std::vector<float>&               weights,
                               size_t                                  output_rank) const;

    // -------------------------------------------------------------------------
    // TIES merge
    // -------------------------------------------------------------------------

    /**
     * @brief TIES-merge multiple LoRA adapters for a single output layer.
     *
     * Implements the Trim–Resolve–Merge algorithm on the flattened ΔW matrices.
     * The output B' and A' represent the merged delta refactored to rank 1
     * (or the requested rank via SVD approximation).
     *
     * @param adapters       Adapters to merge
     * @param out_layer      Name for the output layer
     * @param in_dim         Input dimension
     * @param out_dim        Output dimension
     * @param rank           Rank for the output matrices
     * @param alpha          LoRA alpha
     * @param trim_threshold Fraction of each adapter's max-abs-value used as
     *                       the trimming threshold (default: 0.2 → 20%)
     * @return MergeLayerResult with TIES-merged B and A
     */
    MergeLayerResult mergeTIES(const std::vector<AdapterDescriptor>& adapters,
                               const std::string& out_layer,
                               size_t in_dim,
                               size_t out_dim,
                               size_t rank,
                               float  alpha          = 8.0f,
                               float  trim_threshold = 0.2f) const;

    /**
     * @brief TIES-merge all layers shared by all given adapters.
     *
     * @param adapters       Collection of adapters
     * @param output_rank    Rank for all output layers
     * @param trim_threshold Trimming threshold fraction (default 0.2)
     * @return MergeResult describing the merged layers
     */
    MergeResult mergeTIESAll(const std::vector<const LoRAAdapter*>& adapters,
                             size_t                                  output_rank,
                             float                                   trim_threshold = 0.2f) const;
};

} // namespace training
} // namespace themis
