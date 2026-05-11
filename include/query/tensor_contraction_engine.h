/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_contraction_engine.h                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_contraction_engine.h
 * @brief Algebraic query engine operating directly on TT-compressed tensors.
 *
 * Implements the "Computing in the Compressed Domain" paradigm: standard
 * linear-algebra operations are performed on TT-trains **without ever
 * reconstructing the full dense tensor**.
 *
 * ### Supported operations and complexity
 *
 * | Operation                  | Complexity (dense) | Complexity (TT)       |
 * |----------------------------|--------------------|------------------------|
 * | Inner product ⟨A, B⟩       | O(n^d)             | O(d · n · r³)         |
 * | Frobenius norm ‖A‖_F       | O(n^d)             | O(d · n · r³)         |
 * | Cosine similarity          | O(n^d)             | O(d · n · r³)         |
 * | Subtensor slice A[dim=k]   | O(n^{d-1})         | O(d · n · r²)         |
 * | TT-rounding (recompress)   | —                  | O(d · r³ · n)         |
 * | Hadamard product A ⊙ B     | O(n^d)             | O(d · n · r₁²·r₂²)   |
 *
 * ### References
 * - Holtz, S., Rohwedder, T., & Schneider, R. (2012). The alternating linear
 *   scheme for tensor optimization in the TT-format.
 *   SIAM J. Sci. Comput., 34(2), A683–A713.
 * - Bigoni, D., Engsig-Karup, A. P., & Marzouk, Y. M. (2016).
 *   Spectral tensor-train decomposition.
 *   SIAM J. Sci. Comput., 38(4), A2405–A2439.
 * - Roberts, C. et al. (2019). TensorNetwork: A Library for Physics and ML.
 *   arXiv:1905.01330.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include &lt;optional&gt;
#include <stdexcept>
#include <vector>

namespace themis {
namespace query {

// ============================================================================
// TensorContractionEngine
// ============================================================================

/**
 * @brief Stateless algebraic engine for operations in the TT-compressed domain.
 *
 * All methods are `static` — no instance state is required.  The engine works
 * directly on `themis::storage::TTTrain` objects produced by
 * `TensorTrainDecomposer` or loaded from `TensorNetworkStorageEngine`.
 *
 * ### Automatic TT-Rounding
 * When the intermediate TT-rank of a result would grow beyond
 * `kDefaultMaxRankAfterOp`, the engine automatically applies TT-rounding
 * (Oseledets 2011, Algorithm 2) to bound memory usage.
 *
 * ### Usage (AQL integration)
 * The AQL built-in functions `TENSOR_SIMILARITY`, `TENSOR_NORM`,
 * `TENSOR_SLICE`, and `TENSOR_COMPRESS` delegate directly to this engine.
 *
 * @code
 * using storage::TTTrain;
 * using query::TensorContractionEngine;
 *
 * // Cosine similarity without decompression
 * double sim = TensorContractionEngine::cosineSimilarity(trainA, trainB);
 *
 * // Slice along dimension 1, index 3
 * TTTrain sliced = TensorContractionEngine::slice(train, 1, 3);
 * @endcode
 */
class TensorContractionEngine {
public:
    /// Default max TT-rank applied after binary operations to bound growth.
    static constexpr std::size_t kDefaultMaxRankAfterOp = 64;

    // ─── Inner product / norms ────────────────────────────────────────────

    /**
     * @brief Inner product ⟨A, B⟩ in the TT-compressed domain.
     *
     * Uses the transfer-matrix (zipper) algorithm.
     * Complexity: O(d · n · r³) where r = max(rank_A, rank_B).
     *
     * @throws std::invalid_argument if A and B have incompatible mode_sizes.
     */
    static double innerProduct(const storage::TTTrain& a,
                               const storage::TTTrain& b);

    /**
     * @brief Frobenius norm ‖A‖_F without reconstruction.
     */
    static double frobeniusNorm(const storage::TTTrain& a);

    /**
     * @brief Cosine similarity cos(A, B) = ⟨A,B⟩ / (‖A‖·‖B‖) ∈ [−1, 1].
     *
     * Returns 0.0 when either norm is zero.
     */
    static double cosineSimilarity(const storage::TTTrain& a,
                                   const storage::TTTrain& b);

    // ─── Structural operations ────────────────────────────────────────────

    /**
     * @brief Extract a subtensor by fixing one index.
     *
     * Fixes mode `dim` to index `idx`, producing a (d-1)-dimensional TTTrain.
     * The k-th core is contracted with the unit vector e_{idx}, reducing it
     * from shape (r_{k-1}, n_k, r_k) to (r_{k-1}, r_k).
     *
     * @param train  Source TT-train.
     * @param dim    Mode to slice (0-indexed, must be < train.order()).
     * @param idx    Index along mode `dim` (must be < train.mode_sizes[dim]).
     * @return       TT-train of order d-1.
     * @throws std::out_of_range if dim or idx are out of bounds.
     */
    static storage::TTTrain slice(const storage::TTTrain& train,
                                  std::size_t dim,
                                  std::size_t idx);

    /**
     * @brief Element-wise (Hadamard) product A ⊙ B.
     *
     * Result has ranks r_k = r_A_k × r_B_k (Kronecker product of cores).
     * TT-rounding is applied automatically when max_rank > 0.
     *
     * @throws std::invalid_argument if mode_sizes differ.
     */
    static storage::TTTrain hadamardProduct(
        const storage::TTTrain& a,
        const storage::TTTrain& b,
        std::size_t max_rank = kDefaultMaxRankAfterOp,
        double round_eps     = 1e-4);

    // ─── Recompression ────────────────────────────────────────────────────

    /**
     * @brief Re-compress a TTTrain with tighter epsilon or lower max_rank.
     *
     * Delegates to `TensorTrainDecomposer::round()`.
     *
     * @param train    Input train.
     * @param eps      New reconstruction error tolerance.
     * @param max_rank New maximum TT-rank (0 = unlimited).
     */
    static storage::TTTrain recompress(const storage::TTTrain& train,
                                       double eps,
                                       std::size_t max_rank = 0);

    // ─── Marginalization / Projection ────────────────────────────────────

    /**
     * @brief Project (marginalize) a TTTrain over one mode.
     *
     * Computes result(i₁,…,i_{k-1},i_{k+1},…,i_d) = ∑_{j} T(i₁,…,j,…,i_d)
     * entirely in the compressed domain by summing over the physical index of
     * core k and absorbing the resulting matrix into an adjacent core.
     *
     * Complexity: O(d · n · r²) — no reconstruction required.
     *
     * @param train  Source TT-train of order d ≥ 2.
     * @param mode   Mode to marginalize over (0-indexed, < train.order()).
     * @return       TT-train of order d-1.
     * @throws std::out_of_range  if mode ≥ train.order().
     * @throws std::invalid_argument if the train has order < 2.
     */
    static storage::TTTrain project(const storage::TTTrain& train,
                                    std::size_t mode);

    // ─── Multi-mode tensor contraction ───────────────────────────────────

    /**
     * @brief Multi-mode tensor contraction of two TT-trains.
     *
     * Contracts modes listed in `modes_a` of train `a` with the
     * corresponding modes listed in `modes_b` of train `b`.
     * `modes_a[i]` and `modes_b[i]` must have equal mode_sizes.
     *
     * When all modes of both trains are contracted and both have the same
     * shape, the result is equivalent to `innerProduct(a, b)` returned as a
     * scalar 1-element TTTrain of order 1.
     *
     * For partial contractions the result is a TT-train whose mode sequence
     * is: [non-contracted modes of a] + [non-contracted modes of b].
     * The contracted dimension is eliminated from both operands.  TT-rounding
     * is applied automatically with `eps = max(a.achieved_eps, b.achieved_eps)`
     * to prevent rank explosion.
     *
     * @note Dense reconstruction is used internally for non-scalar results
     *       to guarantee numerical correctness.  This is appropriate for
     *       AQL queries that operate on small tensors embedded in documents.
     *       For large tensors use the compressed-domain innerProduct/hadamardProduct
     *       operations instead.
     *
     * @param a        Left operand.
     * @param b        Right operand.
     * @param modes_a  Modes of `a` to contract (0-indexed, non-repeating).
     * @param modes_b  Modes of `b` to contract (0-indexed, same length as modes_a).
     * @param max_rank Max TT-rank in the result after rounding (0 = unlimited).
     * @param round_eps Reconstruction error tolerance (0 = use operand eps).
     * @return TT-train of order (a.order()-modes_a.size()+b.order()-modes_b.size()).
     *         Order-0 result (full contraction) is returned as an order-1
     *         train with mode_size 1.
     * @throws std::invalid_argument if modes are out of range, lengths differ,
     *         or matched mode sizes are incompatible.
     */
    static storage::TTTrain contractModes(
        const storage::TTTrain&        a,
        const storage::TTTrain&        b,
        const std::vector<std::size_t>& modes_a,
        const std::vector<std::size_t>& modes_b,
        std::size_t                    max_rank  = kDefaultMaxRankAfterOp,
        double                         round_eps = 1e-4);

    // ─── Utility ──────────────────────────────────────────────────────────

    /**
     * @brief Check whether two trains have compatible mode_sizes.
     */
    static bool isCompatible(const storage::TTTrain& a,
                              const storage::TTTrain& b) noexcept;

private:
    // Transfer-matrix algorithm: compute M_k = M_{k-1} ⊗ (G_A_k^T · G_B_k)
    // for k = 1…d, then return Tr(M_d).
    static std::vector<float> transferStep(
        const std::vector<float>& M,
        const storage::TTCore&    coreA,
        const storage::TTCore&    coreB);

    static std::vector<float> matMul(const std::vector<float>& A,
                                     const std::vector<float>& B,
                                     std::size_t m,
                                     std::size_t k,
                                     std::size_t n);
};

} // namespace query
} // namespace themis
