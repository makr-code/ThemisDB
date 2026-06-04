/**
 * @file hierarchical_tucker_decomposer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    /// Maximum HOOI alternating-optimization iterations after HOSVD initialization.
    /// Set to 0 to use HOSVD-only (lower quality but faster).  Default 20 iterations
    /// are sufficient for convergence in most practical cases.
    std::size_t max_hooi_iter = 20;
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
     * @note Stub #287 resolved: HOOI alternating optimization loop added after
     * HOSVD initialization in `decompose()` (see `hierarchical_tucker_decomposer.cpp`,
     * Step 2b).  Iterates until ‖G‖_F converges (rel. change < 1e-6) or 20 sweeps
     * complete.  Long-term plan (Q2 2028): extend `ITensorIndex` to support HT directly.
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
     * Uses the shared `TensorTrainDecomposer::truncatedSVD()` backend.
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
