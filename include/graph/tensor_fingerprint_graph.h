/**
 * @file tensor_fingerprint_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "storage/tensor_train_decomposer.h"

namespace themis {
namespace graph {

// ============================================================================
// TensorFingerprint — MinHash + Simhash signature of a TTTrain
// ============================================================================

struct TensorFingerprint {
    std::array<uint64_t, 128> minhash{};

    std::vector<float> core_norms;

    float total_norm = 0.0f;

    std::size_t order = 0;

    std::size_t max_rank = 0;
};

// ============================================================================
// SimilarTensorResult
// ============================================================================

struct SimilarTensorResult {
    std::string tensor_id;      ///< Identifier of the similar tensor
    double      similarity = 0; ///< Cosine similarity ∈ [−1, 1]
    std::string tenant;
    std::string collection;
    std::string field;
};

// ============================================================================
// PersistedFingerprintNode
// ============================================================================

struct PersistedFingerprintNode {
    std::string tensor_id;
    TensorFingerprint fingerprint;
    std::string tenant;
    std::string collection;
    std::string field;
};

struct PersistedFingerprintEdge {
    std::string from;
    std::string to = {};
    double similarity = 0.0;
};

struct PersistedFingerprintGraphSnapshot {
    std::vector<PersistedFingerprintNode> nodes;
    std::vector<PersistedFingerprintEdge> edges;
};

// ============================================================================
// FingerprintGraphConfig
// ============================================================================

struct FingerprintGraphConfig {
    double similarity_threshold = 0.95;

    std::size_t num_hash_funcs  = 128;

    std::size_t num_bands       = 32;

    std::size_t max_candidates  = 1000;

    std::size_t top_k           = 50;

    bool cache_trains_in_memory = true;
};

// ============================================================================
// TensorFingerprintGraph
// ============================================================================

class TensorFingerprintGraph {
public:
    using TrainLoadFn = std::function<std::optional<storage::TTTrain>(
        const std::string& tensor_id,
        const std::string& tenant,
        const std::string& collection,
        const std::string& field)>;
    explicit TensorFingerprintGraph(
        const FingerprintGraphConfig& cfg = {});

    ~TensorFingerprintGraph() = default;

    // ─── Insert / Update ──────────────────────────────────────────────────

    void insert(const std::string&           tensor_id,
                const storage::TTTrain&       train,
                const std::string&           tenant     = "",
                const std::string&           collection = "",
                const std::string&           field      = "");

    /**
     * @brief Set Train Load Fn.
     * @param[in] fn Input parameter.
     */
    void setTrainLoadFn(TrainLoadFn fn);

    /**
     * @brief Remove.
     * @param[in] tensor_id Identifier of the tensor.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& tensor_id);

    // ─── Query ────────────────────────────────────────────────────────────

    std::vector<SimilarTensorResult>
    findSimilar(const storage::TTTrain& train,
                std::size_t top_k = 0) const;

    std::vector<SimilarTensorResult>
    neighbours(const std::string& tensor_id) const;

    /**
     * @brief Export Persisted Nodes.
     * @return Return value.
     */
    std::vector<PersistedFingerprintNode> exportPersistedNodes() const;

    /**
     * @brief Import Persisted Nodes.
     * @param[in] nodes Input parameter.
     */
    void importPersistedNodes(const std::vector<PersistedFingerprintNode>& nodes);

    /**
     * @brief Export Persisted Edges.
     * @return Return value.
     */
    std::vector<PersistedFingerprintEdge> exportPersistedEdges() const;

    /**
     * @brief Import Persisted Edges.
     * @param[in] edges Input parameter.
     */
    void importPersistedEdges(const std::vector<PersistedFingerprintEdge>& edges);

    /**
     * @brief Export Persisted Graph.
     * @return Return value.
     */
    PersistedFingerprintGraphSnapshot exportPersistedGraph() const;

    std::optional<PersistedFingerprintNode>
    exportPersistedNode(const std::string& tensor_id) const;

    std::vector<PersistedFingerprintEdge>
    exportPersistedEdgesFor(const std::string& tensor_id) const;

    /**
     * @brief Import Persisted Graph.
     * @param[in] snapshot Input parameter.
     */
    void importPersistedGraph(const PersistedFingerprintGraphSnapshot& snapshot);

    /**
     * @brief Upsert Persisted Node.
     * @param[in] node Input parameter.
     * @param[in] edges Input parameter.
     */
    void upsertPersistedNode(const PersistedFingerprintNode& node,
                             const std::vector<PersistedFingerprintEdge>& edges);

    // ─── GraphIndex-backed durable storage hooks ──────────────────────────

    using NodePersistHookFn =
        std::function<void(const PersistedFingerprintNode&,
                           const std::vector<PersistedFingerprintEdge>&)>;

    using NodeRemoveHookFn = std::function<void(std::string_view tensor_id)>;

    using NodeEnumerateFn = std::function<void(
        std::function<void(const PersistedFingerprintNode&,
                           const std::vector<PersistedFingerprintEdge>&)>)>;

    /**
     * @brief Set Node Persist Hook.
     * @param[in] fn Input parameter.
     */
    void setNodePersistHook(NodePersistHookFn fn);

    /**
     * @brief Set Node Remove Hook.
     * @param[in] fn Input parameter.
     */
    void setNodeRemoveHook(NodeRemoveHookFn fn);

    /**
     * @brief Restore From External Store.
     * @param[in] enumerate_fn Input parameter.
     */
    void restoreFromExternalStore(NodeEnumerateFn enumerate_fn);

    /**
     * @brief ─── Statistics ───────────────────────────────────────────────────────
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    std::size_t nodeCount()  const noexcept;

    /**
     * @brief Edge Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::size_t edgeCount()  const noexcept;

    const FingerprintGraphConfig& config() const noexcept { return cfg_; }

private:
    FingerprintGraphConfig cfg_;

    // Node metadata + fingerprint
    struct NodeEntry {
        TensorFingerprint  fingerprint;
        storage::TTTrain   train;
        std::string        tenant;
        std::string        collection;
        std::string        field;
    };
    std::unordered_map<std::string, NodeEntry> nodes_;

    // Adjacency list: tensor_id → {neighbour_id, similarity}
    struct Edge { std::string to; double similarity; };
    std::unordered_map<std::string, std::vector<Edge>> adj_;

    // LSH buckets: band_idx:bucket_hash → set of tensor_ids
    std::unordered_map<uint64_t, std::unordered_set<std::string>> lsh_buckets_;

    // Non-empty bucket presence set: O(1) empty-band skip in lshCandidates().
    // Mirrors the key set of lsh_buckets_ (populated on insert, cleared on erase).
    std::unordered_set<uint64_t> lsh_nonempty_;

    mutable std::mutex mutex_;
    std::atomic<std::size_t> edge_count_{0};
    TrainLoadFn train_load_fn_;

    std::size_t rows_per_band_ = 4;

    // ─── Persistence hooks ────────────────────────────────────────────────
    NodePersistHookFn  node_persist_hook_;
    NodeRemoveHookFn   node_remove_hook_;
    mutable std::mutex hook_mutex_;   ///< guards node_persist_hook_ / node_remove_hook_
    std::atomic<bool>  has_node_persist_hook_{false};
    std::atomic<bool>  has_node_remove_hook_{false};

    /**
     * @brief ─── Fingerprinting ───────────────────────────────────────────────────
     * @param[in] train Input parameter.
     * @return Return value.
     */

    TensorFingerprint computeFingerprint(const storage::TTTrain& train) const;

    /**
     * @brief Insert Into Buckets.
     * @param[in] id Input parameter.
     * @param[in] fp Input parameter.
     */
    void insertIntoBuckets(const std::string& id, const TensorFingerprint& fp);
    /**
     * @brief Remove From Buckets.
     * @param[in] id Input parameter.
     * @param[in] fp Input parameter.
     */
    void removeFromBuckets(const std::string& id, const TensorFingerprint& fp);

    std::unordered_set<std::string>
    lshCandidates(const TensorFingerprint& fp) const;

    /**
     * @brief Band Hash.
     * @param[in] fp Input parameter.
     * @param[in] band_start Input parameter.
     * @param[in] rows_per_band Input parameter.
     * @param[in] band_idx Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static uint64_t bandHash(const TensorFingerprint& fp,
                             std::size_t band_start,
                             std::size_t rows_per_band,
                             std::size_t band_idx) noexcept;

    /**
     * @brief Fnv1a64.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static uint64_t fnv1a64(const void* data, std::size_t len) noexcept;

    /**
     * @brief Exact Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    double exactSimilarity(const storage::TTTrain& a,
                           const storage::TTTrain& b) const;

    std::optional<storage::TTTrain>
    resolveTrainForNode(const std::string& tensor_id,
                        const NodeEntry& node) const;

    PersistedFingerprintNode
    buildPersistedNodeLocked(const std::string& tensor_id) const;

    std::vector<PersistedFingerprintEdge>
    buildPersistedEdgesForLocked(const std::string& tensor_id) const;
};

} // namespace graph
} // namespace themis

