/**
 * @file tensor_fingerprint_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
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

    /// Cached squared L2 norm of `fingerprint` for query-time fast path.
    float fingerprint_sq_norm = 0.0f;

    /// Frobenius norm of the first TT-core (used for normalisation).
    float first_core_norm = 0.0f;

    /// Full TT train used for exact compressed-domain similarity in findSimilar().
    storage::TTTrain exact_train;
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
    *
    * @note Complexity: O(N * C_ip) for N candidates and TT inner-product cost C_ip.
    *       With `setExactSimilarityFn()`, complexity becomes O(N * C_backend).
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
    *
    * @note Complexity: O(N * D_overlap) where D_overlap is the shared prefix
    *       length of compared fingerprints. Missing dimensions are treated as zeros.
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

    // ─── ExactSimilarity bridge (stub #276) ───────────────────────────────

    /// @brief Type alias for exact-similarity injection.
    using ExactSimilarityFn = std::function<float(const std::string& key_a,
                                                   const std::string& key_b)>;

    /**
     * @brief Install an exact-similarity function for key-based similarity queries.
     *
     * When set, findSimilar() delegates per-pair scoring to this function
     * instead of computing TT cosine similarity via inner products.
     * findSimilarByFingerprint() remains a fingerprint-only approximate path.
     *
     * @param fn Callable receiving two adapter keys and returning a score in [0, 1].
     */
    static void setExactSimilarityFn(ExactSimilarityFn fn);

    /**
     * @brief Remove the exact-similarity override (reverts to TT cosine path).
     */
    static void clearExactSimilarityFn();

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

    /// Cosine similarity with explicit zero-padding semantics and cached norms.
    /// Returns 0.0 if either vector has near-zero norm.
    static float cosineSimilarityZeroPadded(const std::vector<float>& a,
                                            float                    a_sq_norm,
                                            const std::vector<float>& b,
                                            float                    b_sq_norm) noexcept;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, FingerprintEntry> entries_;
    std::unordered_map<std::string, storage::TTTrain> trains_;
    std::unordered_map<std::string, double>           train_self_ip_;

    mutable std::shared_mutex stats_mutex_;
    mutable GraphStats        stats_;
};

} // namespace tensor
} // namespace themis
