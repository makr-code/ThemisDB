/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_fingerprint_graph.h                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 4 prep (Q3 2027)                    ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_fingerprint_graph.h
 * @brief Adapter similarity graph with TT-exact and fingerprint-approximate lookup.
 *
 * ## Overview (paper §Adapter Sovereignty)
 *
 * When many LoRA/PEFT adapters are stored as TT graphs inside ThemisDB,
 * the `TensorFingerprintGraph` provides two lookup modes:
 * - `findSimilar()` computes cosine similarity in the TT domain via
 *   TT inner products (exact ranking in compressed space).
 * - `findSimilarByFingerprint()` computes cosine similarity on a compact
 *   first-core fingerprint vector (fast approximate ranking).
 *
 * Each adapter is fingerprinted by the column means of its first
 * TT-core (`G_0`), yielding a compact float32 vector that approximates
 * the dominant variance direction of the adapter.  Similarity is then
 * measured by cosine distance on these fingerprints.
 *
 * ### Why column-mean of G_0?
 * The first TT-core `G_0 ∈ ℝ^{n₁ × r₁}` captures the leading inter-
 * mode correlations.  Its column means (per output rank) approximate
 * the first singular vector scaled by `r₁`, which is equivalent to
 * a rank-1 sketch of the adapter.  This is O(n₁ · r₁) to compute
 * and O(r₁) to store — negligible compared to the full adapter.
 *
 * ## Thread Safety
 * All public methods are thread-safe via shared_mutex.
 *
 * ## References
 * - Holtz, S. et al. (2012). SIAM J. Sci. Comput. — TT inner-product.
 * - Hu, E. et al. (2022). LoRA: Low-Rank Adaptation.  ICLR.
 * - ThemisDB Research Group (2026). §Adapter Sovereignty. Pre-print.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// FingerprintEntry — per-adapter fingerprint record
// ============================================================================

/**
 * @brief Fingerprint descriptor stored for one adapter.
 */
struct FingerprintEntry {
    std::string adapter_key;    ///< Storage key (as used by AdapterRepository)
    std::string domain;         ///< Domain tag (e.g. "legal", "medical")
    std::string base_model_id;  ///< Base model identifier
    std::string tenant_id;      ///< Owning tenant

    /// Column means of G_0 (first TT-core); length = r₁ of the first core.
    std::vector<float> fingerprint;

    /// Frobenius norm of the first TT-core (used for normalisation).
    float first_core_norm = 0.0f;
};

// ============================================================================
// SimilarityResult — one entry returned by findSimilar()
// ============================================================================

struct SimilarityResult {
    std::string adapter_key;   ///< Adapter key of the similar adapter
    std::string domain;
    std::string base_model_id;
    float       score = 0.0f;  ///< Cosine similarity ∈ [−1, 1]
};

// ============================================================================
// TensorFingerprintGraph
// ============================================================================

/**
 * @brief Fingerprint-based similarity graph for TT-encoded adapters.
 *
 * ### Typical usage
 * ```cpp
 * TensorFingerprintGraph graph;
 * graph.addAdapter("__adapters__:t1:legal:llama3", adapter_tt,
 *                  "legal", "llama3", "t1");
 *
 * auto results = graph.findSimilar("__adapters__:t1:legal:llama3", 5);
 * for (auto& r : results) {
 *     std::cout << r.adapter_key << "  sim=" << r.score << "\n";
 * }
 * ```
 */
class TensorFingerprintGraph {
public:
    TensorFingerprintGraph() = default;

    // ─── Write API ────────────────────────────────────────────────────────

    /**
     * @brief Add or update an adapter's fingerprint in the graph.
     *
     * Extracts the column-mean fingerprint from `train.cores[0]` and
     * registers it under `adapter_key`.  If an entry already exists for
     * the same key it is replaced.
     *
     * @param adapter_key   Unique storage key of the adapter.
     * @param train         Full TT-format adapter weights.
     * @param domain        Domain tag.
     * @param base_model_id Base model identifier.
     * @param tenant_id     Owning tenant namespace.
     * @return false if `train.cores` is empty (nothing inserted).
     */
    bool addAdapter(const std::string&        adapter_key,
                    const storage::TTTrain&   train,
                    const std::string&        domain,
                    const std::string&        base_model_id,
                    const std::string&        tenant_id = "default");

    /**
     * @brief Remove an adapter's fingerprint from the graph.
     *
     * @return true if the key was found and removed, false otherwise.
     */
    bool removeAdapter(const std::string& adapter_key);

    // ─── Query API ────────────────────────────────────────────────────────

    /**
     * @brief Find the top-k adapters most similar to the query adapter.
     *
     * Similarity is computed as cosine similarity in the TT domain using
     * `TensorTrainDecomposer::innerProduct()`.  The query adapter itself is
     * excluded from the result list.
     *
     * @param query_key  Storage key of the query adapter (must be registered).
     * @param k          Maximum number of results to return.
     * @return           Sorted list (descending score) of similar adapters.
     *                   Empty if query_key is not registered or k == 0.
     */
    [[nodiscard]] std::vector<SimilarityResult>
        findSimilar(const std::string& query_key, std::size_t k) const;

    /**
     * @brief Find the top-k adapters most similar to the given raw fingerprint.
     *
     * Useful when the caller holds a fingerprint not yet stored in the graph.
     * This path is intentionally approximate and does not use TT inner-product.
     *
     * @param fingerprint  Query fingerprint vector.
     * @param k            Maximum number of results to return.
     * @param tenant_id    If non-empty, restrict results to this tenant.
     */
    [[nodiscard]] std::vector<SimilarityResult>
        findSimilarByFingerprint(const std::vector<float>& fingerprint,
                                 std::size_t               k,
                                 const std::string&        tenant_id = "") const;

    // ─── Inspection ───────────────────────────────────────────────────────

    /**
     * @brief Retrieve the stored fingerprint entry for a given key.
     */
    [[nodiscard]] std::optional<FingerprintEntry>
        entry(const std::string& adapter_key) const;

    /// Number of adapters currently registered.
    [[nodiscard]] std::size_t size() const noexcept;

    /// List all registered adapter keys (sorted).
    [[nodiscard]] std::vector<std::string> adapterKeys() const;

    // ─── Statistics ───────────────────────────────────────────────────────

    struct GraphStats {
        std::size_t total_adapters      = 0;
        std::size_t total_query_calls   = 0;
        std::size_t total_comparisons   = 0;  ///< Total fingerprint dot-products
    };

    [[nodiscard]] GraphStats stats() const noexcept;

private:
    /// Compute column means of a TT-core data block.
    /// data layout: [n_rows × n_cols] row-major.
    static std::vector<float> columnMeans(const std::vector<float>& data,
                                          std::size_t n_rows,
                                          std::size_t n_cols);

    /// Cosine similarity between two equal-length vectors.
    /// Returns 0.0 if either vector is zero.
    static float cosineSimilarity(const std::vector<float>& a,
                                   const std::vector<float>& b) noexcept;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, FingerprintEntry> entries_;
    std::unordered_map<std::string, storage::TTTrain> trains_;

    mutable std::shared_mutex stats_mutex_;
    mutable GraphStats        stats_;
};

} // namespace tensor
} // namespace themis
