/**
 * @file tensor_fingerprint_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief 128-bit MinHash + Frobenius-norm-based fingerprint.
 *
 * Two tensors with similar fingerprints are candidate duplicates; exact
 * cosine similarity is computed to confirm.
 */
struct TensorFingerprint {
    /// 128-element MinHash signature (each element is a 64-bit hash)
    std::array<uint64_t, 128> minhash{};

    /// Frobenius norm of each TT-core (length = TT-order)
    std::vector<float> core_norms;

    /// Total Frobenius norm of the tensor
    float total_norm = 0.0f;

    /// Tensor order (d)
    std::size_t order = 0;

    /// Maximum TT-rank
    std::size_t max_rank = 0;
};

// ============================================================================
// SimilarTensorResult
// ============================================================================

/**
 * @brief One entry in a similar-tensor query result.
 */
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

/**
 * @brief Durable node payload used for graph metadata recovery.
 *
 * Stores only fingerprint + node metadata. TT-trains and edges are rebuilt or
 * resolved externally after restore.
 */
struct PersistedFingerprintNode {
    std::string tensor_id;
    TensorFingerprint fingerprint;
    std::string tenant;
    std::string collection;
    std::string field;
};

/**
 * @brief Durable directed edge payload for adjacency re-hydration.
 */
struct PersistedFingerprintEdge {
    std::string from;
    std::string to = {};
    double similarity = 0.0;
};

/**
 * @brief Durable full graph payload used for one-shot snapshot restore.
 */
struct PersistedFingerprintGraphSnapshot {
    std::vector<PersistedFingerprintNode> nodes;
    std::vector<PersistedFingerprintEdge> edges;
};

// ============================================================================
// FingerprintGraphConfig
// ============================================================================

/**
 * @brief Configuration for TensorFingerprintGraph.
 */
struct FingerprintGraphConfig {
    /// Cosine similarity threshold for adding an edge (default: 0.95)
    double similarity_threshold = 0.95;

    /// Number of MinHash hash functions (default: 128)
    std::size_t num_hash_funcs  = 128;

    /// Number of LSH bands (default: 32; rows_per_band = num_hash_funcs / bands)
    std::size_t num_bands       = 32;

    /// Maximum candidates to verify per query before early-stopping
    std::size_t max_candidates  = 1000;

    /// Maximum number of similar tensors to return per query
    std::size_t top_k           = 50;

    /// Keep full TT-trains in-memory for similarity verification.
    /// When false, a train loader callback should be provided.
    bool cache_trains_in_memory = true;
};

// ============================================================================
// TensorFingerprintGraph
// ============================================================================

/**
 * @brief Directed similarity graph over TT-compressed tensors.
 *
 * Nodes are identified by a string `tensor_id` (typically
 * `"<tenant>/<collection>/<field>@<version>"`).  Edges are added
 * automatically when fingerprint similarity exceeds `similarity_threshold`.
 *
 * ### Thread safety
 * All public methods are protected by an internal `std::mutex`.  The graph
 * supports concurrent inserts and queries, but updates during a query may
 * cause the query to see a partially-updated adjacency list (acceptable for
 * approximate similarity search).
 */
class TensorFingerprintGraph {
public:
    using TrainLoadFn = std::function<std::optional<storage::TTTrain>(
        const std::string& tensor_id,
        const std::string& tenant,
        const std::string& collection,
        const std::string& field)>;
    /**
     * @brief Construct with configuration.
     * @throws std::invalid_argument if num_hash_funcs % num_bands != 0.
     */
    explicit TensorFingerprintGraph(
        const FingerprintGraphConfig& cfg = {});

    ~TensorFingerprintGraph() = default;

    // ─── Insert / Update ──────────────────────────────────────────────────

    /**
     * @brief Add or update a tensor in the graph.
     *
     * 1. Computes the TensorFingerprint.
     * 2. Performs LSH lookup to find candidate neighbours.
     * 3. Computes exact cosine similarity for each candidate.
     * 4. Adds edges for pairs with similarity ≥ threshold.
     *
     * @param tensor_id  Unique identifier for this tensor.
     * @param train      TT-train to fingerprint.
     * @param tenant     Owning tenant (metadata).
     * @param collection Collection name (metadata).
     * @param field      Field name (metadata).
     */
    void insert(const std::string&           tensor_id,
                const storage::TTTrain&       train,
                const std::string&           tenant     = "",
                const std::string&           collection = "",
                const std::string&           field      = "");

    /// Configure optional train resolver used when in-memory train cache is disabled.
    void setTrainLoadFn(TrainLoadFn fn);

    /**
     * @brief Remove a tensor and all its edges from the graph.
     *
     * @return True if the tensor_id existed, false otherwise.
     */
    bool remove(const std::string& tensor_id);

    // ─── Query ────────────────────────────────────────────────────────────

    /**
     * @brief Find tensors most similar to the given train.
     *
     * Uses LSH for candidate selection + exact cosine similarity ranking.
     *
     * @param train   Query tensor (does not need to be in the graph).
     * @param top_k   Number of results to return (overrides config if > 0).
     * @return        Sorted result list (descending similarity).
     */
    std::vector<SimilarTensorResult>
    findSimilar(const storage::TTTrain& train,
                std::size_t top_k = 0) const;

    /**
     * @brief Get graph neighbours of an already-stored tensor.
     *
     * Returns tensors directly connected to `tensor_id` by a similarity edge.
     *
     * @return Sorted result list (descending similarity), or empty if not found.
     */
    std::vector<SimilarTensorResult>
    neighbours(const std::string& tensor_id) const;

    /// Export node fingerprint metadata for durable graph bootstrap.
    std::vector<PersistedFingerprintNode> exportPersistedNodes() const;

    /// Replace in-memory graph with persisted node metadata and rebuilt buckets.
    /// Edges are not restored and start empty after import.
    void importPersistedNodes(const std::vector<PersistedFingerprintNode>& nodes);

    /// Export directed adjacency edges for durable graph re-hydration.
    std::vector<PersistedFingerprintEdge> exportPersistedEdges() const;

    /// Replace in-memory adjacency with persisted directed edges.
    /// Missing nodes and duplicate directed edges are ignored.
    void importPersistedEdges(const std::vector<PersistedFingerprintEdge>& edges);

    /// Export node + edge payload in one snapshot.
    PersistedFingerprintGraphSnapshot exportPersistedGraph() const;

    /// Export one persisted node payload if present.
    std::optional<PersistedFingerprintNode>
    exportPersistedNode(const std::string& tensor_id) const;

    /// Export directed edges originating from one node.
    std::vector<PersistedFingerprintEdge>
    exportPersistedEdgesFor(const std::string& tensor_id) const;

    /// Atomically replace graph state from a full persisted snapshot.
    void importPersistedGraph(const PersistedFingerprintGraphSnapshot& snapshot);

    /// Upsert one persisted node and replace its symmetric adjacency.
    void upsertPersistedNode(const PersistedFingerprintNode& node,
                             const std::vector<PersistedFingerprintEdge>& edges);

    // ─── GraphIndex-backed durable storage hooks ──────────────────────────

    /// Called after insert() succeeds; delivers the newly persisted node and
    /// all outgoing directed edges from that node.
    /// Invoked outside the graph mutex. Exceptions are swallowed.
    using NodePersistHookFn =
        std::function<void(const PersistedFingerprintNode&,
                           const std::vector<PersistedFingerprintEdge>&)>;

    /// Called after remove() succeeds; delivers the removed tensor_id.
    /// Invoked outside the graph mutex. Exceptions are swallowed.
    using NodeRemoveHookFn = std::function<void(std::string_view tensor_id)>;

    /// Callback signature for enumerating externally stored nodes during restore.
    /// The caller's callback receives each (node, edges) pair.
    using NodeEnumerateFn = std::function<void(
        std::function<void(const PersistedFingerprintNode&,
                           const std::vector<PersistedFingerprintEdge>&)>)>;

    /// Register (or clear with nullptr/empty fn) the node-persistence hook.
    /// The hook is invoked after every successful insert() call.
    void setNodePersistHook(NodePersistHookFn fn);

    /// Register (or clear with nullptr/empty fn) the node-removal hook.
    /// The hook is invoked after every successful remove() call.
    void setNodeRemoveHook(NodeRemoveHookFn fn);

    /// Restore graph state by enumerating nodes from an external per-node store.
    ///
    /// Clears the current in-memory graph, then calls @p enumerate_fn which
    /// must invoke its argument once per stored (node, edges) pair.
    /// After enumeration the graph supports findSimilar() and neighbours()
    /// without requiring a full-blob snapshot.
    ///
    /// @param enumerate_fn  Callback that enumerates all stored nodes.
    void restoreFromExternalStore(NodeEnumerateFn enumerate_fn);

    // ─── Statistics ───────────────────────────────────────────────────────

    /// Number of nodes in the graph.
    std::size_t nodeCount()  const noexcept;

    /// Number of directed edges in the graph.
    std::size_t edgeCount()  const noexcept;

    /// Configuration.
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

    // ─── Fingerprinting ───────────────────────────────────────────────────

    TensorFingerprint computeFingerprint(const storage::TTTrain& train) const;

    void insertIntoBuckets(const std::string& id, const TensorFingerprint& fp);
    void removeFromBuckets(const std::string& id, const TensorFingerprint& fp);

    std::unordered_set<std::string>
    lshCandidates(const TensorFingerprint& fp) const;

    static uint64_t bandHash(const TensorFingerprint& fp,
                             std::size_t band_start,
                             std::size_t rows_per_band,
                             std::size_t band_idx) noexcept;

    static uint64_t fnv1a64(const void* data, std::size_t len) noexcept;

    double exactSimilarity(const storage::TTTrain& a,
                           const storage::TTTrain& b) const;

    std::optional<storage::TTTrain>
    resolveTrainForNode(const std::string& tensor_id,
                        const NodeEntry& node) const;

    /// Build persisted node payload — caller MUST hold mutex_ (at least shared).
    PersistedFingerprintNode
    buildPersistedNodeLocked(const std::string& tensor_id) const;

    /// Build persisted outgoing edges for a node — caller MUST hold mutex_ (at least shared).
    std::vector<PersistedFingerprintEdge>
    buildPersistedEdgesForLocked(const std::string& tensor_id) const;
};

} // namespace graph
} // namespace themis

