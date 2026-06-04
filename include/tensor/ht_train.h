/**
 * @file ht_train.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include "storage/tensor_train_decomposer.h"
#include <string>
#include <vector>

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

    // ── TT-train memoization cache (stub #286) ─────────────────────────────────
    //
    // `tt_cache_mtx_` is a shared_ptr so the struct remains moveable: after a
    // move, the moved-from object's mtx_ becomes null and cache operations
    // degrade gracefully to uncached behaviour.
    mutable std::shared_ptr<std::mutex>       tt_cache_mtx_{std::make_shared<std::mutex>()};
    /// Lazily populated by `toTTTrain()`; null until first call.
    mutable std::shared_ptr<storage::TTTrain> tt_cache_;

    // ── Introspection ──────────────────────────────────────────────────────────

    /// Number of modes d.
    std::size_t order() const noexcept { return shape.size(); }

    /// Total float parameters stored in the HT tree.
    std::size_t totalParams() const noexcept { return root ? root->totalParams() : 0; }
    /// Compression ratio: (∏ n_k) / totalParams.  > 1 means compressed.
    double compressionRatio() const noexcept;

    // ── Compatibility bridge ───────────────────────────────────────────────────

    /**
     * @brief Flatten the HT representation to a TT-train (memoized).
     *
     * On the first call, reconstructs the full dense tensor and re-decomposes it
     * as a TT-train using `TensorTrainDecomposer`.  The result is cached behind a
     * mutex so subsequent calls return the cached value without recomputing.
     *
     * Intended for compatibility with `ITensorIndex`; not efficient for large tensors
     * on the initial call.  Cache is invalidated when the `HTTrain` is move-assigned.
     *
     * @note Stub #286 resolved: memoization behind `tt_cache_mtx_` / `tt_cache_`
     * eliminates repeated O(∏ n_k) reconstruction cost.  The long-term removal plan
     * (Q2 2028) is to extend `ITensorIndex` to support `IHierarchicalTuckerIndex`
     * directly, removing the round-trip entirely.
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

    // Explicit move ctor: move all data members; mutex is default-constructed
    // (mutexes are not moveable in C++).
    HTTrain(HTTrain&& other) noexcept
        : root(std::move(other.root))
        , shape(std::move(other.shape))
        , max_rank(other.max_rank)
        , achieved_eps(other.achieved_eps)
        , original_norm(other.original_norm)
        , tt_cache_mtx_(std::move(other.tt_cache_mtx_))
        , tt_cache_(std::move(other.tt_cache_))
    {}

    // Explicit move assignment.
    HTTrain& operator=(HTTrain&& other) noexcept {
        if (this != &other) {
            root         = std::move(other.root);
            shape        = std::move(other.shape);
            max_rank     = other.max_rank;
            achieved_eps = other.achieved_eps;
            original_norm = other.original_norm;
            tt_cache_mtx_ = std::move(other.tt_cache_mtx_);
            tt_cache_    = std::move(other.tt_cache_);
        }
        return *this;
    }

    // No implicit copy; use clone()
    HTTrain(const HTTrain&)            = delete;
    HTTrain& operator=(const HTTrain&) = delete;

    /// Deep-copy the entire HT tree.
    HTTrain clone() const;

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
