/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage/hierarchical_tucker_decomposer.h           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 5 (Q1 2028)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file storage/hierarchical_tucker_decomposer.h
 * @brief HierarchicalTuckerDecomposer — HOSVD-based HT tensor factorization.
 *
 * Implements the Hierarchical Tucker decomposition (Grasedyck 2010) of a dense
 * multi-dimensional tensor T ∈ ℝ^{n_0 × … × n_{d-1}} into an HTTrain.
 *
 * ## Algorithm (STUB #179 — HOSVD initialization, not HOOI)
 *
 * 1. **HOSVD leaves**: for each mode k compute the truncated SVD of the mode-k
 *    unfolding T_(k) ∈ ℝ^{n_k × (N/n_k)} → U_k ∈ ℝ^{n_k × r_k}.
 *
 * 2. **Tucker core**: G = T ×_0 U_0^T ×_1 U_1^T … ×_{d-1} U_{d-1}^T
 *    (multi-mode product; G ∈ ℝ^{r_0 × … × r_{d-1}}).
 *
 * 3. **HT transfer tensors** (top-down balanced binary split):
 *    Starting from the full Tucker core G (augmented with a trailing 1-dim to
 *    represent rank_out = 1 at the root), each internal node [L, R) with
 *    split M = (L+R)/2 runs two sequential SVDs:
 *    - SVD-1: unfold core along [L..M-1] vs [M..R-1, out_rank]
 *             → G_left (core for left subtree, rank r_inner)
 *             → G_right_raw (intermediate; shape [n_right, r_out, r_inner])
 *    - SVD-2: unfold G_right_raw along [M..R-1] vs [out_rank, r_inner]
 *             → G_right (core for right subtree, rank r_23)
 *             → B_node[r_inner, r_23, r_out] (transfer tensor stored at this node)
 *    Recursion terminates at d_sub == 2 (leaf-pair: B = core) or
 *    d_sub == 1 (single leaf: U_effective = U_k · core).
 *
 * ## Stubs
 * - STUB #287: HOSVD initialization (not HOOI alternating optimization).
 * - STUB #288: Symmetric Jacobi EVD for truncated SVD
 *   (O(r³ · iter) Jacobi sweeps for the small Gram matrix; r ≤ max_rank).
 */

#pragma once

#include "tensor/ht_train.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// HTConfig
// ============================================================================

/**
 * @brief Configuration for HierarchicalTuckerDecomposer.
 */
struct HTConfig {
    std::size_t max_rank = 16;  ///< Maximum rank per node (clamped to min(n_k, N/n_k))
    double      eps      = 0.01; ///< Relative reconstruction error tolerance
};

// ============================================================================
// HierarchicalTuckerDecomposer
// ============================================================================

/**
 * @brief Decomposes a dense tensor into Hierarchical Tucker (HT) format.
 *
 * Thread-safe: all methods are `const` and stateless after construction.
 */
class HierarchicalTuckerDecomposer {
public:
    explicit HierarchicalTuckerDecomposer(HTConfig cfg = {}) noexcept;

    /// Decomposition statistics returned alongside the HTTrain.
    struct Stats {
        std::size_t num_modes;      ///< d (order of the input tensor)
        std::size_t max_rank_used;  ///< Largest rank encountered in the tree
        double      achieved_eps;   ///< ‖T − T̃‖_F / ‖T‖_F
        double      original_norm;  ///< ‖T‖_F of the input tensor
        std::size_t total_params;   ///< Total float parameters in the HTTrain
    };

    /**
     * @brief Decompose a flat dense tensor into HT format.
     *
     * @param data   Flat row-major tensor data (length = ∏ shape[k]).
     * @param shape  Mode sizes; length d ≥ 2; each shape[k] ≥ 1.
     * @return {HTTrain, Stats}
     *
     * @throws std::invalid_argument if data.size() != ∏ shape[k], d < 2, or any
     *         shape[k] == 0.
     *
     * @note
     * // STUB/SIMULATION NOTE (STUB #287):
     * // Purpose: HOSVD-based HT initialization so that IHierarchicalTuckerIndex can
     * //          be exercised before HOOI alternating optimization is available.
     * // Activation: Always — no compile-time flag required.
     * // Production Delta: HOSVD is a suboptimal initialization; HOOI iterations
     * //   minimize ‖T − T̃‖_F to machine precision but require multiple tensor
     * //   passes.  Reconstruction error may be up to ε·√d higher than optimal.
     * // Removal Plan: Q2 2028 — add HOOI iteration loop after HOSVD initialization;
     * //   iterate until ‖T − T̃‖_F / ‖T‖_F < eps or max_iter (default 20) reached.
     */
    std::pair<tensor::HTTrain, Stats>
    decompose(const std::vector<float>&        data,
              const std::vector<std::size_t>&  shape) const;

private:
    HTConfig cfg_;

    // ── Static internal helpers ────────────────────────────────────────────────

    /// Mode-k unfolding of a flat tensor: returns matrix T_(k) ∈ ℝ^{n_k × (N/n_k)}.
    static std::vector<float> modeKUnfolding(
        const std::vector<float>&        data,
        const std::vector<std::size_t>&  shape,
        std::size_t                      mode_k);

    /**
     * @brief Truncated SVD of an m×n matrix A.
     *
     * Returns U (m × r), S (r), Vt (r × n) where r is chosen such that
     * sigma[r] < delta (or r = max_rank if the threshold is never reached).
     *
     * Uses symmetric Jacobi EVD on the smaller Gram matrix (A·A^T or A^T·A)
     * for accuracy on dense float32 inputs.
     *
     * @note
     * // STUB/SIMULATION NOTE (STUB #288):
     * // Purpose: Provide a self-contained truncated SVD without linking LAPACK.
     * // Activation: Always — no build flag required.
     * // Production Delta: O(r³ · iter) Jacobi sweeps vs. O(m·n·r) LAPACK dgesdd.
     * //   For max_rank ≤ 64 and small Gram matrices the cost is negligible.
     * //   For large r or ill-conditioned matrices (high dynamic range), Golub-
     * //   Reinsch bidiagonalization (as in TensorTrainDecomposer) is preferred.
     * // Removal Plan: Q2 2028 — expose TensorTrainDecomposer::truncatedSVD() as a
     * //   protected/friend static; reuse across all decomposers.
     */
    static void truncatedSVD(
        const std::vector<float>&  mat,
        std::size_t                m,
        std::size_t                n,
        double                     delta,
        std::size_t                max_rank_cap,
        std::vector<float>&        U_out,
        std::vector<float>&        S_out,
        std::vector<float>&        Vt_out,
        std::size_t&               rank_out);

    /// Mode-k product: applies U^T (r × n_k) to mode k of tensor `data`.
    /// Returns a tensor with shape[k] replaced by r.
    static std::vector<float> modeKProduct(
        const std::vector<float>&        data,
        const std::vector<std::size_t>&  shape,
        std::size_t                      mode_k,
        const std::vector<float>&        U,   ///< [n_k × r] row-major
        std::size_t                      n_k,
        std::size_t                      r);

    /**
     * @brief Recursive top-down HT node construction from a "core" tensor.
     *
     * @param core       Flat core tensor of shape
     *                   [phys_{L},...,phys_{R-1}, r_out] (row-major).
     * @param core_shape Shape of `core` (length = (R-L) + 1, last elem = r_out).
     * @param L, R       Mode range [L, R) covered by this subtree.
     * @param U_cache    HOSVD leaf bases indexed by physical mode.
     * @param T_shape    Original tensor shape (for leaf n_k).
     */
    std::unique_ptr<tensor::HTNode> buildHTNode(
        const std::vector<float>&              core,
        const std::vector<std::size_t>&        core_shape,
        std::size_t                            L,
        std::size_t                            R,
        const std::vector<std::vector<float>>& U_cache,
        const std::vector<std::size_t>&        T_shape) const;
};

} // namespace storage
} // namespace themis
