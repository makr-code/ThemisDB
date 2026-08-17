/**
 * @file adjacency_list.h
 * @brief Graph adjacency-list with iterator-safe edge mutation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: CWE-416 iterator safety applied (Sprint 7 Batch C, Phase 2B)
 * @note Status: Production Ready
 *
 * Provides an in-memory directed adjacency-list graph representation.
 * All mutation operations (add/remove edge, clear vertex) that historically
 * modified vectors while iterators were live have been remediated via the
 * collect-then-modify pattern using `themis::security::SafeIterator`
 * (gap IDs A003, A006 from Sprint 7).
 *
 * **CWE Remediations:**
 * - CWE-416 (Type A – Invalidation): `remove_edges_if()` collects indices
 *   into a scratch vector and erases in reverse order; no iterator survives
 *   across container mutation.
 * - CWE-129 (Type B – Bounds): `neighbour_at()` validates index with
 *   `BoundsChecker::check_dereference()`.
 * - `RangeValidator` wraps every traversal sub-range.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "security/safe_iterator.h"

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// VertexId / EdgeWeight
// ---------------------------------------------------------------------------

/// Opaque vertex identifier.
using VertexId   = std::uint64_t;

/// Edge weight (default 1.0 for unweighted graphs).
using EdgeWeight = double;

// ---------------------------------------------------------------------------
// Edge
// ---------------------------------------------------------------------------

/**
 * @brief One directed edge: source → target with optional weight and label.
 */
struct Edge {
    VertexId    target;           ///< Destination vertex.
    EdgeWeight  weight{1.0};      ///< Edge weight.
    std::string label;            ///< Optional semantic label (may be empty).
};

// ---------------------------------------------------------------------------
// VertexDescriptor
// ---------------------------------------------------------------------------

/**
 * @brief Metadata attached to a vertex.
 */
struct VertexDescriptor {
    VertexId    id;
    std::string label; ///< Human-readable label.
};

// ---------------------------------------------------------------------------
// EdgePredicate
// ---------------------------------------------------------------------------

/**
 * @brief Predicate type used by `remove_edges_if()`.
 *
 * Return `true` to remove the edge, `false` to keep it.
 */
using EdgePredicate = std::function<bool(const Edge&)>;

// ---------------------------------------------------------------------------
// AdjacencyList
// ---------------------------------------------------------------------------

/**
 * @brief Directed in-memory graph represented as an adjacency list.
 *
 * Vertices are identified by `VertexId` (uint64_t).  Each vertex stores a
 * `std::vector<Edge>` for its outgoing edges.  Vertex and edge operations
 * maintain consistency via the collect-then-modify pattern throughout.
 *
 * **Iterator safety guarantees:**
 * - `remove_edges_if()`: collects candidate edge indices in a scratch buffer,
 *   then erases in reverse order.  No iterator into the edge vector is held
 *   across any `erase()` call.
 * - `neighbour_at()`: index validated with `BoundsChecker::check_dereference()`.
 * - Traversal helpers wrap sub-ranges in `RangeValidator` before iteration.
 *
 * **Thread safety:** not thread-safe; external synchronisation required.
 *
 * **Usage:**
 * ```cpp
 * AdjacencyList g;
 * g.add_vertex(1, "Alice");
 * g.add_vertex(2, "Bob");
 * g.add_edge(1, {2, 1.0, "knows"});
 *
 * // Safe removal — invalidation-free:
 * g.remove_edges_if(1, [](const Edge& e){ return e.weight < 0.5; });
 * ```
 */
class AdjacencyList {
public:
    AdjacencyList()  = default;
    ~AdjacencyList() = default;

    AdjacencyList(const AdjacencyList&)            = default;
    AdjacencyList& operator=(const AdjacencyList&) = default;
    AdjacencyList(AdjacencyList&&)                 noexcept = default;
    AdjacencyList& operator=(AdjacencyList&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Vertex operations
    // -----------------------------------------------------------------------

    /**
     * @brief Add a vertex.  No-op if the vertex already exists.
     * @param id    Vertex identifier.
     * @param label Optional human-readable label.
     * @return `true` if inserted, `false` if already present.
     */
    bool add_vertex(VertexId id, std::string label = {});

    /**
     * @brief Remove a vertex and all edges incident on it.
     * @param id Vertex to remove.
     * @return `true` if removed, `false` if not found.
     *
     * **Iterator safety:** uses `remove_edges_if()` internally so that all
     * vector mutations follow the collect-then-erase pattern.
     */
    bool remove_vertex(VertexId id);

    /**
     * @brief Test whether a vertex exists.
     * @return `true` if present.
     */
    [[nodiscard]] bool has_vertex(VertexId id) const noexcept;

    /**
     * @brief Number of vertices.
     * @return Vertex count.
     */
    [[nodiscard]] std::size_t vertex_count() const noexcept;

    /**
     * @brief Vertex descriptor for a given id.
     * @return `std::nullopt` if not found.
     */
    [[nodiscard]] std::optional<VertexDescriptor> vertex(VertexId id) const;

    // -----------------------------------------------------------------------
    // Edge operations
    // -----------------------------------------------------------------------

    /**
     * @brief Add an outgoing edge from `src`.
     * @param src  Source vertex (must exist).
     * @param edge Edge to add.
     * @throws std::out_of_range if `src` does not exist.
     */
    void add_edge(VertexId src, Edge edge);

    /**
     * @brief Remove edges from `src` matching the predicate.
     *
     * **Iterator safety (Type A):** All edges are scanned once; indices of
     * matching edges are collected in a scratch `std::vector<std::size_t>`.
     * Erasure is performed in reverse-index order via
     * `AdvanceSafe::advance()` so no iterator is invalidated during the
     * removal loop.
     *
     * @param src  Source vertex.
     * @param pred Predicate: returns `true` for edges to remove.
     * @return Number of edges removed.
     * @throws std::out_of_range if `src` does not exist.
     */
    std::size_t remove_edges_if(VertexId src, const EdgePredicate& pred);

    /**
     * @brief Total number of outgoing edges from `src`.
     * @return Edge count, or 0 if `src` not found.
     */
    [[nodiscard]] std::size_t out_degree(VertexId src) const noexcept;

    /**
     * @brief Return all outgoing edges from `src`.
     * @return Const reference to the edge vector; empty if `src` not found.
     *
     * **Validity:** the returned reference is invalidated by any subsequent
     * call to `add_edge()`, `remove_edges_if()`, or `remove_vertex()`.  Do
     * not hold the reference across mutation calls.
     */
    [[nodiscard]] const std::vector<Edge>& neighbours(VertexId src) const;

    /**
     * @brief Access a specific neighbour by index with bounds checking.
     * @param src   Source vertex.
     * @param index Zero-based index into the outgoing edge list.
     * @return Const reference to the edge.
     * @throws std::out_of_range if `src` is unknown or `index >= out_degree(src)`.
     *
     * **Iterator safety:** uses `BoundsChecker::check_dereference()`.
     */
    [[nodiscard]] const Edge& neighbour_at(VertexId src, std::size_t index) const;

    /**
     * @brief Total number of edges across all vertices.
     * @return Edge count.
     */
    [[nodiscard]] std::size_t edge_count() const noexcept;

    /**
     * @brief Clear all vertices and edges.
     */
    void clear() noexcept;

private:
    struct VertexData {
        VertexDescriptor  descriptor;
        std::vector<Edge> edges;
    };

    std::unordered_map<VertexId, VertexData> graph_;

    static const std::vector<Edge> kEmptyEdges;
};

}  // namespace graph
}  // namespace themis
