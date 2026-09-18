/**
 * @file adjacency_list.h
 * @brief Graph adjacency-list with iterator-safe edge mutation.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
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

using VertexId   = std::uint64_t;

using EdgeWeight = double;

// ---------------------------------------------------------------------------
// Edge
// ---------------------------------------------------------------------------

struct Edge {
    VertexId    target;           ///< Destination vertex.
    EdgeWeight  weight{1.0};      ///< Edge weight.
    std::string label;            ///< Optional semantic label (may be empty).
};

// ---------------------------------------------------------------------------
// VertexDescriptor
// ---------------------------------------------------------------------------

struct VertexDescriptor {
    VertexId    id;
    std::string label; ///< Human-readable label.
};

// ---------------------------------------------------------------------------
// EdgePredicate
// ---------------------------------------------------------------------------

using EdgePredicate = std::function<bool(const Edge&)>;

// ---------------------------------------------------------------------------
// AdjacencyList
// ---------------------------------------------------------------------------

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

    bool add_vertex(VertexId id, std::string label = {});

    /**
     * @brief Remove vertex.
     * @param[in] id Input parameter.
     * @return True when the operation succeeds.
     */
    bool remove_vertex(VertexId id);

    [[nodiscard]] bool has_vertex(VertexId id) const noexcept;

    [[nodiscard]] std::size_t vertex_count() const noexcept;

    [[nodiscard]] std::optional<VertexDescriptor> vertex(VertexId id) const;

    // -----------------------------------------------------------------------
    // Edge operations
    // -----------------------------------------------------------------------

    /**
     * @brief Add edge.
     * @param[in] src Input parameter.
     * @param[in] edge Input parameter.
     */
    void add_edge(VertexId src, Edge edge);

    /**
     * @brief Remove edges if.
     * @param[in] src Input parameter.
     * @param[in] pred Input parameter.
     * @return Return value.
     */
    std::size_t remove_edges_if(VertexId src, const EdgePredicate& pred);

    [[nodiscard]] std::size_t out_degree(VertexId src) const noexcept;

    [[nodiscard]] const std::vector<Edge>& neighbours(VertexId src) const;

    [[nodiscard]] const Edge& neighbour_at(VertexId src, std::size_t index) const;

    [[nodiscard]] std::size_t edge_count() const noexcept;

    /**
     * @brief Clear.
     * @note Exception safety: noexcept.
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
