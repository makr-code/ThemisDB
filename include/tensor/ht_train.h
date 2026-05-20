/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/ht_train.h                                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-07                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 5 (Q1 2028)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/ht_train.h
 * @brief Hierarchical Tucker (HT) decomposition types and contraction engine.
 *
 * ## Overview
 *
 * A Hierarchical Tucker (HT) tensor is represented as a balanced binary tree of nodes:
 *
 * - **Leaf nodes**: store a basis matrix U_k ∈ ℝ^{n_k × rank} where n_k is the
 *   physical dimension of mode k and rank is the representation rank for this node.
 *
 * - **Internal nodes**: store a transfer tensor B ∈ ℝ^{r_left × r_right × rank}
 *   that contracts the outputs of the two children into a representation of size rank.
 *
 * ### Storage complexity
 *
 * For d modes with uniform physical size n and uniform rank r:
 * - Leaves: d × (n × r) = O(d·n·r) floats
 * - Internal nodes: (d-1) × (r × r × r) = O(d·r³) floats
 * - Total: O(d·n·r + d·r³)
 *
 * ### Inner product (compressed domain)
 *
 * ⟨A, B⟩_HT is computed bottom-up via Gram matrix propagation:
 * 1. Leaf k: Γ_k = U_A_k^T · U_B_k  ∈ ℝ^{r_Ak × r_Bk}  (O(n·r²))
 * 2. Internal node: Γ = ∑ B_A[γ_l,γ_r,α] Γ_l[γ_l,γ_l'] Γ_r[γ_r,γ_r'] B_B[γ_l',γ_r',β]
 * 3. Root: ⟨A, B⟩ = Γ_root[0, 0]   (rank_out = 1 at root)
 *
 * Total cost: O(d·n·r² + d·r⁴).
 *
 * ## References
 * - Grasedyck, L. (2010). Hierarchical Singular Value Decomposition of Tensors. SIAM.
 * - Hackbusch, W. & Kühn, S. (2009). A New Scheme for the Tensor Representation.
 * - ThemisDB Tensor Phase 5 (2028). §HT in Phase 5 ROADMAP.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Forward declaration of TTTrain for the compatibility bridge
namespace themis {
namespace storage {
struct TTTrain;
} // namespace storage
} // namespace themis

namespace themis {
namespace tensor {

// ============================================================================
// HTNode — one node in the HT binary tree
// ============================================================================

/**
 * @brief One node in the Hierarchical Tucker binary tree.
 *
 * **Leaf node** (is_leaf == true):
 *   - Stores basis matrix U ∈ ℝ^{n_k × rank} (row-major: U[i,α] = U[i*rank+α]).
 *   - `rank` is the output rank this leaf contributes to its parent.
 *
 * **Internal node** (is_leaf == false):
 *   - Stores transfer tensor B ∈ ℝ^{r_left × r_right × rank}
 *     (row-major: B[l,r,α] = B[l*r_right*rank + r*rank + α]).
 *   - `r_left`  = rank of the left child's output.
 *   - `r_right` = rank of the right child's output.
 *   - `rank`    = output rank of this node (1 at the root).
 */
struct HTNode {
    bool        is_leaf    = true;  ///< true → leaf; false → internal
    std::size_t mode_index = 0;     ///< Physical mode index (leaf only)
    std::size_t n_k        = 0;     ///< Physical mode size, i.e. shape[mode_index] (leaf only)
    std::size_t rank       = 0;     ///< Output rank of this node

    // ── Leaf data ─────────────────────────────────────────────────────────────
    /// Basis matrix U ∈ ℝ^{n_k × rank}.  Stored row-major.
    std::vector<float> U;

    // ── Internal-node data ────────────────────────────────────────────────────
    std::size_t r_left  = 0;  ///< Rank of left child's output
    std::size_t r_right = 0;  ///< Rank of right child's output

    /// Transfer tensor B ∈ ℝ^{r_left × r_right × rank}.  Stored row-major.
    std::vector<float> B;

    /// Left child (nullptr for leaves)
    std::unique_ptr<HTNode> left;
    /// Right child (nullptr for leaves)
    std::unique_ptr<HTNode> right;

    // ── Accessors ─────────────────────────────────────────────────────────────

    /// U[i, alpha] — only valid for leaf nodes.
    float  atU(std::size_t i, std::size_t alpha) const { return U[i * rank + alpha]; }
    float& atU(std::size_t i, std::size_t alpha)       { return U[i * rank + alpha]; }

    /// B[l, r, alpha] — only valid for internal nodes.
    float  atB(std::size_t l, std::size_t r, std::size_t alpha) const
    { return B[l * r_right * rank + r * rank + alpha]; }
    float& atB(std::size_t l, std::size_t r, std::size_t alpha)
    { return B[l * r_right * rank + r * rank + alpha]; }

    /// Total float parameters stored in this node (not recursing into children).
    std::size_t numParams() const noexcept { return is_leaf ? U.size() : B.size(); }

    /// Recursive total: all float parameters in this subtree.
    std::size_t totalParams() const noexcept;

    /// Deep copy of this subtree.
    std::unique_ptr<HTNode> clone() const;
};

// ============================================================================
// HTTrain — a complete HT decomposition of one tensor
// ============================================================================

/**
 * @brief A Hierarchical Tucker (HT) representation of a dense multi-dimensional tensor.
 *
 * The tree is a balanced binary tree over d = shape.size() modes.
 * `shape[k]` gives n_k (the original size along mode k).
 */
struct HTTrain {
    /// Root of the HT binary tree.
    std::unique_ptr<HTNode> root;

    /// Original tensor shape (length = d = order of the tensor).
    std::vector<std::size_t> shape;

    /// Upper-bound rank used during decomposition.
    std::size_t max_rank = 0;

    /// Achieved relative reconstruction error ‖T − T̃‖_F / ‖T‖_F.
    double achieved_eps = 0.0;

    /// Frobenius norm of the original tensor.
    double original_norm = 0.0;

    // ── Introspection ──────────────────────────────────────────────────────────

    /// Number of modes d.
    std::size_t order() const noexcept { return shape.size(); }

    /// Total float parameters stored in the HT tree.
    std::size_t totalParams() const noexcept { return root ? root->totalParams() : 0; }

    /// Compression ratio: (∏ n_k) / totalParams.  > 1 means compressed.
    double compressionRatio() const noexcept;

    // ── Compatibility bridge ───────────────────────────────────────────────────

    /**
     * @brief Flatten the HT representation to a TT-train.
     *
     * Reconstructs the full dense tensor and re-decomposes it as a TT-train.
     * Intended for compatibility with `ITensorIndex`; not efficient for large tensors.
     *
     * @note
     * // STUB/SIMULATION NOTE (STUB #286):
     * // Purpose: TTTrain compatibility until ITensorIndex is extended for HTTrain.
     * // Activation: Always.
     * // Production Delta: O(∏ n_k) full reconstruction + TT redecomposition.
     * // Removal Plan: Q2 2028 — extend ITensorIndex / add IHierarchicalTuckerIndex path.
     */
    storage::TTTrain toTTTrain() const;

    /// Reconstruct the dense tensor (slow; for correctness testing only).
    std::vector<float> reconstruct() const;

    // ── Serialization ──────────────────────────────────────────────────────────

    /// Serialise to a byte blob for persistent storage.
    std::vector<uint8_t> serialize() const;

    /// Deserialise from bytes; returns std::nullopt on format error.
    static std::optional<HTTrain> deserialize(const std::vector<uint8_t>& bytes);

    // ── Move / copy ────────────────────────────────────────────────────────────

    HTTrain() = default;
    HTTrain(HTTrain&&) noexcept = default;
    HTTrain& operator=(HTTrain&&) noexcept = default;

    // No implicit copy; use clone()
    HTTrain(const HTTrain&)            = delete;
    HTTrain& operator=(const HTTrain&) = delete;

    /// Deep-copy the entire HT tree.
    HTTrain clone() const;

    // ─── HTToTT bridge (stub #286) ────────────────────────────────────────────

    /// @brief Type alias for HT-to-TT conversion injection.
    using HTToTTFn = std::function<storage::TTTrain(const HTTrain&)>;

    /**
     * @brief Install a HT-to-TT conversion function used by toTTTrain().
     *
     * When set, toTTTrain() delegates to this function instead of the
     * O(∏n_k) full-reconstruction placeholder.
     * @param fn Callable receiving a const HTTrain reference → TTTrain.
     */
    static void setHTToTTFn(HTToTTFn fn) {
        std::lock_guard<std::mutex> lock(s_ht_to_tt_fn_mutex_);
        s_ht_to_tt_fn_ = std::move(fn);
    }

    /**
     * @brief Remove the HT-to-TT conversion bridge (reverts to placeholder).
     */
    static void clearHTToTTFn() {
        std::lock_guard<std::mutex> lock(s_ht_to_tt_fn_mutex_);
        s_ht_to_tt_fn_ = nullptr;
    }

    /// @cond INTERNAL
    static inline std::mutex s_ht_to_tt_fn_mutex_;
    static inline HTToTTFn   s_ht_to_tt_fn_;
    /// @endcond
};

// ============================================================================
// HTContractionEngine — compressed-domain arithmetic on HTTrain objects
// ============================================================================

/**
 * @brief Arithmetic engine for Hierarchical Tucker tensors.
 *
 * Provides O(d·r⁴) inner product without full tensor reconstruction.
 *
 * ### Inner product algorithm (Grasedyck 2010 §4)
 *
 * Given two HT tensors A and B with the same binary-tree topology:
 * 1. Leaf k:  Γ_k[α, β] = ∑_i U_A_k[i, α] · U_B_k[i, β]    (O(n·r²))
 * 2. Internal node (bottom-up):
 *    Γ[α, β] = ∑_{γ_l,γ_l',γ_r,γ_r'} B_A[γ_l,γ_r,α] · B_B[γ_l',γ_r',β]
 *                                      · Γ_left[γ_l,γ_l'] · Γ_right[γ_r,γ_r']
 *    (O(r⁴) per node)
 * 3. Root: ⟨A, B⟩ = Γ_root[0, 0]
 *
 * Total: O(d·n·r² + d·r⁴).
 */
class HTContractionEngine {
public:
    /**
     * @brief Compute ⟨A, B⟩ without decompression.
     *
     * Both A and B must have the same tree topology (same shape and rank layout).
     * Returns 0.0 if the trees are structurally incompatible.
     */
    static double innerProduct(const HTTrain& A, const HTTrain& B);

    /// Compute ‖A‖_F = sqrt(⟨A, A⟩).
    static double frobeniusNorm(const HTTrain& A);

    /// Cosine similarity ∈ [−1, 1]; returns 0.0 if either tensor has zero norm.
    static double cosineSimilarity(const HTTrain& A, const HTTrain& B);

private:
    /// Recursive helper: returns the Gram matrix Γ_t[α, β] as a flat row-major
    /// vector of shape [r_A × r_B].
    static std::vector<double> computeGram(const HTNode& A, const HTNode& B);
};

} // namespace tensor
} // namespace themis
