/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_layer_graph.h                                ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     380                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • (initial)  2026-04-09  feat(graph): add multi-layer graph interface ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/expected.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace themis {
namespace graph {

/**
 * @brief Edge directionality type for a graph layer.
 */
enum class LayerEdgeType {
    DIRECTED,      ///< Edges have a single direction (from → to).
    UNDIRECTED,    ///< Edges are symmetric (from ↔ to).
    BIDIRECTIONAL  ///< Edges exist in both directions simultaneously.
};

/**
 * @brief Aggregation strategy when combining per-layer analytics results.
 */
enum class LayerAggregation {
    SUM,   ///< Sum values across layers.
    AVG,   ///< Arithmetic mean across layers.
    MAX,   ///< Maximum value across layers.
    MIN,   ///< Minimum value across layers.
    COUNT  ///< Count non-zero layers.
};

/**
 * @brief Descriptor for a single named layer in a multi-layer graph.
 */
struct GraphLayer {
    /// Unique layer name (e.g. "friendship", "follows", "colleague").
    std::string name;
    /// Edge directionality for this layer.
    LayerEdgeType edge_type{LayerEdgeType::DIRECTED};
    /// Default edge weight when no weight is supplied to addEdge().
    double default_weight{1.0};
    /// Arbitrary user metadata (e.g. description, creation timestamp).
    std::string metadata;
};

/**
 * @brief A path result spanning (potentially multiple) layers.
 */
struct MultiLayerPath {
    /// Ordered list of vertex identifiers in the path.
    std::vector<std::string> vertices;
    /// Ordered list of edge identifiers (may be "<from>-<layer>-<to>").
    std::vector<std::string> edges;
    /// Names of layers traversed (parallel to the edges vector).
    std::vector<std::string> layers_traversed;
    /// Total weighted path cost (sum of edge weights scaled by layer weights).
    double total_cost{0.0};
    /// Number of edges (= hop count).
    int hop_count{0};
};

/**
 * @brief Result of a layer-scoped reachability check.
 */
struct ReachabilityResult {
    bool reachable{false};
    int min_hops{-1};   ///< -1 if not reachable.
    std::string layer;  ///< Layer on which reachability was established.
};

/**
 * @brief Abstract interface for multi-layer graph operations.
 *
 * A multi-layer graph models complex multi-relational data (e.g. a social
 * network with friendship, follows, and colleague edge types) by storing each
 * edge type in a named layer.  Queries can target a single layer, a subset of
 * layers, or all layers simultaneously.
 *
 * Thread-safety: addLayer(), removeLayer(), and addEdge() are write operations
 * and must not be called concurrently.  Read-only operations (shortestPath,
 * pageRank, isReachable, layers) are safe to call concurrently once the graph
 * topology is fixed.
 */
class IMultiLayerGraph {
public:
    virtual ~IMultiLayerGraph() = default;

    // ── Topology mutation ───────────────────────────────────────────────────

    /**
     * @brief Register a new layer.
     *
     * @param layer Layer descriptor.
     * @return true on success; false if a layer with the same name already exists.
     */
    virtual bool addLayer(const GraphLayer& layer) = 0;

    /**
     * @brief Remove a layer and all its edges.
     *
     * @param name Layer name.
     * @return true if the layer existed and was removed; false otherwise.
     */
    virtual bool removeLayer(const std::string& name) = 0;

    /**
     * @brief Add an edge to a named layer.
     *
     * For UNDIRECTED / BIDIRECTIONAL layers the reverse edge is automatically
     * added.  Duplicate edges (same from, to, layer) are silently ignored.
     *
     * @param layer_name  Target layer. Must already exist.
     * @param from        Source vertex identifier.
     * @param to          Target vertex identifier.
     * @param weight      Edge weight (uses layer default_weight when < 0).
     * @return true on success; false if the layer does not exist.
     */
    virtual bool addEdge(
        const std::string& layer_name,
        const std::string& from,
        const std::string& to,
        double weight = -1.0) = 0;

    // ── Query operations ────────────────────────────────────────────────────

    /**
     * @brief Find the shortest path between two vertices across a set of layers.
     *
     * Edges from all @p layers are merged into a single virtual graph for the
     * search.  Each edge cost is the edge weight multiplied by the
     * corresponding entry in @p layer_weights.  When @p layer_weights is
     * shorter than @p layers, missing weights default to 1.0.
     *
     * @param from          Source vertex identifier.
     * @param to            Target vertex identifier.
     * @param layers        Layer names to include in the search.
     * @param layer_weights Per-layer weight multipliers.
     * @param max_hops      Maximum path length (0 = unlimited).
     * @return The shortest path, or an empty MultiLayerPath when unreachable.
     */
    virtual MultiLayerPath shortestPath(
        const std::string& from,
        const std::string& to,
        const std::vector<std::string>& layers,
        const std::vector<double>& layer_weights = {},
        int max_hops = 0) const = 0;

    /**
     * @brief Find all vertices reachable from @p source within @p max_hops.
     *
     * @param source    Starting vertex.
     * @param layers    Layers to traverse.
     * @param max_hops  Maximum hop distance (0 = unlimited).
     * @return Map of vertex_id → minimum hop distance from source.
     */
    virtual std::unordered_map<std::string, int> reachableFrom(
        const std::string& source,
        const std::vector<std::string>& layers,
        int max_hops = 0) const = 0;

    /**
     * @brief Check whether @p target is reachable from @p source.
     *
     * @param from   Source vertex.
     * @param to     Target vertex.
     * @param layers Layers to consider.
     * @return ReachabilityResult with reachable flag and minimum hops.
     */
    virtual ReachabilityResult isReachable(
        const std::string& from,
        const std::string& to,
        const std::vector<std::string>& layers) const = 0;

    /**
     * @brief Compute PageRank scores over the merged layer topology.
     *
     * Each layer's edge weights are scaled by its corresponding entry in
     * @p layer_weights before the merged adjacency matrix is built.
     * Layer scores are combined according to @p aggregation.
     *
     * @param layers        Layer names to include.
     * @param layer_weights Per-layer weight multipliers.
     * @param aggregation   How to combine scores from different layers.
     * @param damping       PageRank damping factor (typically 0.85).
     * @param max_iter      Maximum PageRank iterations.
     * @return Map of vertex_id → aggregated PageRank score.
     */
    virtual std::map<std::string, double> pageRank(
        const std::vector<std::string>& layers,
        const std::vector<double>& layer_weights = {},
        LayerAggregation aggregation = LayerAggregation::AVG,
        double damping = 0.85,
        int max_iter = 100) const = 0;

    // ── Introspection ───────────────────────────────────────────────────────

    /// @return All registered layer descriptors, sorted by name.
    virtual std::vector<GraphLayer> layers() const = 0;

    /// @return Number of vertices across all layers (union).
    virtual size_t vertexCount() const = 0;

    /// @return Number of edges in the given layer (0 if layer unknown).
    virtual size_t edgeCount(const std::string& layer_name) const = 0;

    /// @return true if @p layer_name exists.
    virtual bool hasLayer(const std::string& layer_name) const = 0;
};

/**
 * @brief Configuration for the in-memory MultiLayerGraph.
 */
struct MultiLayerGraphConfig {
    /// Maximum number of hops for path-finding (0 = no default limit).
    int max_path_hops{100};
    /// Maximum number of paths returned by shortestPath().
    int max_paths_returned{1};
};

/**
 * @brief In-memory implementation of IMultiLayerGraph.
 *
 * Stores the adjacency list for each layer in an in-memory unordered_map.
 * Suitable for graphs that fit in RAM (up to ~10M edges per layer).  For
 * larger graphs, attach a persistence backend via a custom IMultiLayerGraph
 * implementation backed by GraphIndexManager.
 *
 * Usage:
 * @code
 *   MultiLayerGraph mlg;
 *   mlg.addLayer({"friendship", LayerEdgeType::UNDIRECTED});
 *   mlg.addLayer({"follows",    LayerEdgeType::DIRECTED});
 *
 *   mlg.addEdge("friendship", "alice", "bob");
 *   mlg.addEdge("follows",    "alice", "charlie");
 *
 *   auto path = mlg.shortestPath("bob", "charlie",
 *                                 {"friendship", "follows"},
 *                                 {1.0, 0.5});
 *
 *   auto rank = mlg.pageRank({"friendship", "follows"},
 *                             {1.0, 1.0},
 *                             LayerAggregation::AVG);
 * @endcode
 */
class MultiLayerGraph : public IMultiLayerGraph {
public:
    explicit MultiLayerGraph(
        MultiLayerGraphConfig config = MultiLayerGraphConfig{});

    ~MultiLayerGraph() override = default;

    // Non-copyable, movable
    MultiLayerGraph(const MultiLayerGraph&) = delete;
    MultiLayerGraph& operator=(const MultiLayerGraph&) = delete;
    MultiLayerGraph(MultiLayerGraph&&) noexcept = default;
    MultiLayerGraph& operator=(MultiLayerGraph&&) noexcept = default;

    bool addLayer(const GraphLayer& layer) override;
    bool removeLayer(const std::string& name) override;
    bool addEdge(
        const std::string& layer_name,
        const std::string& from,
        const std::string& to,
        double weight = -1.0) override;

    MultiLayerPath shortestPath(
        const std::string& from,
        const std::string& to,
        const std::vector<std::string>& layers,
        const std::vector<double>& layer_weights = {},
        int max_hops = 0) const override;

    std::unordered_map<std::string, int> reachableFrom(
        const std::string& source,
        const std::vector<std::string>& layers,
        int max_hops = 0) const override;

    ReachabilityResult isReachable(
        const std::string& from,
        const std::string& to,
        const std::vector<std::string>& layers) const override;

    std::map<std::string, double> pageRank(
        const std::vector<std::string>& layers,
        const std::vector<double>& layer_weights = {},
        LayerAggregation aggregation = LayerAggregation::AVG,
        double damping = 0.85,
        int max_iter = 100) const override;

    std::vector<GraphLayer> layers() const override;
    size_t vertexCount() const override;
    size_t edgeCount(const std::string& layer_name) const override;
    bool hasLayer(const std::string& layer_name) const override;

private:
    MultiLayerGraphConfig config_;

    /// Layer metadata by name.
    std::unordered_map<std::string, GraphLayer> layer_meta_;

    /// Adjacency list per layer: layer_name → (from → [(to, weight)]).
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::pair<std::string, double>>>>
        adj_;

    /// Union of all vertices across layers.
    std::unordered_map<std::string, int> vertex_set_; // vertex → dummy int

    // ── Internal helpers ────────────────────────────────────────────────────

    /// Build merged adjacency list for the given layers with weight scaling.
    std::unordered_map<std::string, std::vector<std::pair<std::string, double>>>
    buildMergedAdj(
        const std::vector<std::string>& layers,
        const std::vector<double>& layer_weights) const;

    /// Dijkstra's algorithm on a pre-built adjacency list.
    MultiLayerPath dijkstra(
        const std::unordered_map<std::string,
            std::vector<std::pair<std::string, double>>>& adj,
        const std::string& from,
        const std::string& to,
        int max_hops,
        const std::vector<std::string>& layers) const;
};

} // namespace graph
} // namespace themis
