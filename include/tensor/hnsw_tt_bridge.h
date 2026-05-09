/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/hnsw_tt_bridge.h                            ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 2 (Q4 2026)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/hnsw_tt_bridge.h
 * @brief HnswTTBridge — hybrid HNSW navigation over Tensor-Train storage.
 *
 * ## The HNSW-on-TT-Cores Pattern
 *
 * This class implements the "two-layer" architecture described in
 * `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md §HYBRID regime`:
 *
 * ```
 *  Query (flat vector)
 *        │
 *        ▼
 *  ┌──────────────────────────────────────────────────────┐
 *  │  HNSW Navigation Layer (src/index)                  │
 *  │  • Graph built over first-core sketches only         │
 *  │  • Sketch dim: min(n_1, 64)  — small footprint       │
 *  │  • Candidate set size: ef_search (configurable)      │
 *  └──────────────────────────────────────────────────────┘
 *        │  top-k candidates (IDs only)
 *        ▼
 *  ┌──────────────────────────────────────────────────────┐
 *  │  TT Re-Ranking Layer (src/tensor)                   │
 *  │  • TT-cosine similarity: O(d · r²) per candidate    │
 *  │  • Full TT-train loaded from TensorNetworkStorage    │
 *  │  • Result sorted by TT-domain cosine distance        │
 *  └──────────────────────────────────────────────────────┘
 *        │
 *        ▼
 *  Final k results (with exact TT-domain distances)
 * ```
 *
 * ## When to use
 *
 * According to the boundary analysis the HYBRID regime is optimal when:
 *
 *   κ ∈ [2×, 6×)  — moderate compressibility
 *   dim ∈ [1024, 16384]
 *   n ≥ 500 K
 *
 * For κ ≥ 6× the pure TT-index is preferred (use `TensorIndexManager`).
 * For κ < 2× the standard HNSW backend (use `src/index`) is preferred.
 *
 * ## Scientific basis
 * - Malkov & Yashunin 2020 (HNSW, DOI:10.1109/TPAMI.2018.2889473)
 * - Oseledets 2011 (TT-SVD)
 * - Boundary analysis: `research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`
 */

#pragma once

#include "tensor/tensor_index.h"
#include "storage/tensor_train_decomposer.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// HnswTTConfig
// ============================================================================

struct HnswTTConfig {
    // HNSW navigation layer
    size_t  M              = 16;     ///< HNSW M-parameter (neighbours per node)
    size_t  ef_construction = 200;   ///< ef during index construction
    size_t  ef_search       = 50;    ///< ef during search (candidate pool)
    size_t  sketch_dim      = 64;    ///< First-core sketch dimension for HNSW
                                     ///<   (0 = use full first-core dimension)

    // TT re-ranking layer
    size_t  rerank_candidates = 200; ///< Candidates passed to TT re-ranker
    size_t  max_tt_rank       = 32;  ///< Maximum TT-rank for compression
    double  epsilon           = 0.01; ///< Relative reconstruction error bound

    // Routing threshold (used by TensorRouter to decide HYBRID)
    double  kappa_min = 2.0;  ///< Minimum compressibility for HYBRID route
    double  kappa_max = 6.0;  ///< Maximum compressibility (above → pure TT)
};

// ============================================================================
// HnswTTBridge
// ============================================================================

/**
 * @brief Hybrid index combining HNSW navigation with TT-domain re-ranking.
 *
 * Implements `ITensorIndex` so it can be managed by `TensorIndexManager` as
 * a first-class entry alongside pure-TT and pure-HNSW indexes.
 *
 * ### Thread safety
 * Concurrent search() calls are safe after build().  add() / remove() acquire
 * an exclusive lock internally.
 */
class HnswTTBridge final : public ITensorIndex {
public:
    /**
     * @brief Construct with configuration.
     *
     * @param config   Tuning parameters for both layers.
     * @param dim      Vector dimension (must match all added vectors).
     */
    explicit HnswTTBridge(HnswTTConfig config = {}, size_t dim = 0);
    ~HnswTTBridge() override;

    // ITensorIndex implementation ----------------------------------------

    [[nodiscard]] bool add(int64_t id,
                           const storage::TTTrain& train) override;

    [[nodiscard]] bool addFlat(int64_t id,
                               const float* vector,
                               size_t dim) override;

    bool remove(int64_t id) override;

    std::vector<TensorSearchResult> search(
        const storage::TTTrain& query, int k) const override;

    std::vector<TensorSearchResult> searchFlat(
        const float* query, size_t dim, int k) const override;

    std::optional<float> innerProduct(int64_t id_a,
                                       int64_t id_b) const override;

    std::optional<float> norm(int64_t id) const override;

    const storage::TTTrain* get(int64_t id) const override;

    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;

    [[nodiscard]] size_t           size()  const override;
    [[nodiscard]] TensorIndexStats stats() const override;

    // HnswTTBridge-specific extras ---------------------------------------

    /**
     * @brief Return the first-core sketch for a stored vector.
     *
     * Used by the HNSW layer internally; exposed for testing.
     *
     * @param id  Vector ID.
     * @return    Float sketch of dimension min(n_1, sketch_dim), or empty.
     */
    std::vector<float> getSketch(int64_t id) const;

    /**
     * @brief Expose current configuration.
     */
    const HnswTTConfig& config() const { return cfg_; }

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /// Extract the first-core sketch from a TTTrain for HNSW insertion.
    std::vector<float> extractSketch(const storage::TTTrain& train) const;

    /// TT-domain cosine similarity: <A,B>_TT / (||A||_TT · ||B||_TT).
    float ttCosineSimilarity(const storage::TTTrain& a,
                             const storage::TTTrain& b) const;

    // -----------------------------------------------------------------------
    // Static TT arithmetic helpers (shared with tensor_index.cpp logic)
    // -----------------------------------------------------------------------

    static float ttInnerProductFromTrains(const storage::TTTrain& A,
                                           const storage::TTTrain& B);
    static float ttNormFromTrain(const storage::TTTrain& T);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    HnswTTConfig cfg_;
    size_t       dim_ = 0;

    // HNSW layer — lazily initialised on first add()
    struct HnswLayer;
    std::unique_ptr<HnswLayer> hnsw_;

    // TT storage layer
    struct TTStore;
    std::unique_ptr<TTStore> tt_store_;

    mutable std::shared_mutex rw_mutex_;
    TensorIndexStats          stats_;
};

} // namespace tensor
} // namespace themis
