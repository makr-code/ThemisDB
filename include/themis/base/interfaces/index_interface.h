/**
 * @file index_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/// @file index_interface.h
/// @brief Abstract interfaces for index operations
/// 
/// This file defines contracts for various index types in ThemisDB:
/// - Secondary indexes (B-tree, hash)
/// - Vector indexes (HNSW, IVF)
/// - Graph indexes (adjacency lists)
/// - Full-text indexes
/// 
/// Design Goals:
/// - Break circular dependencies between Index ↔ Query ↔ Storage
/// - Enable isolated unit testing with mock implementations
/// - Support multiple index implementation strategies
/// - Allow query engine to use indexes without knowing implementation details
/// 
/// @note This is a Phase 1 interface definition. Implementations will be
///       refactored in subsequent phases to use this interface.

#pragma once

#include "themis/base/export.h"
#include "themis/base/interfaces/query_interface.h"
#include "utils/expected.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/// @brief Index type enumeration
enum class IndexType {
    SECONDARY,      ///< B-tree or hash secondary index
    VECTOR,         ///< Vector similarity index (HNSW, IVF)
    GRAPH,          ///< Graph adjacency index
    FULLTEXT,       ///< Full-text search index
    SPATIAL,        ///< Spatial/geospatial index
    COMPOSITE       ///< Composite multi-field index
};

/// @brief Index scan order
enum class ScanOrder {
    ASCENDING,
    DESCENDING,
    UNORDERED  ///< No particular order (fastest for hash indexes)
};

/// @brief Abstract interface for secondary indexes
/// 
/// Secondary indexes provide fast lookup by non-primary-key fields.
/// They map indexed field values to primary keys.
class THEMIS_BASE_API ISecondaryIndex {
public:
    virtual ~ISecondaryIndex() = default;

    /// @brief Insert or update an index entry
    /// @param indexed_value The value being indexed
    /// @param primary_key The primary key of the document/row
    /// @return true on success, false on failure
    [[nodiscard]] virtual bool insert(std::string_view indexed_value, 
                       std::string_view primary_key) = 0;

    /// @brief Remove an index entry
    /// @param indexed_value The indexed value
    /// @param primary_key The primary key
    /// @return true if entry existed and was removed, false otherwise
    [[nodiscard]] virtual bool remove(std::string_view indexed_value,
                       std::string_view primary_key) = 0;

    /// @brief Lookup by exact value
    /// @param value The value to look up
    /// @return Vector of matching primary keys
    [[nodiscard]] virtual std::vector<std::string> lookup(std::string_view value) const = 0;

    /// @brief Range scan [start_value, end_value)
    /// @param start_value Start of range (inclusive)
    /// @param end_value End of range (exclusive)
    /// @param order Scan order
    /// @return Vector of matching primary keys in requested order
    [[nodiscard]] virtual std::vector<std::string> rangeScan(
        std::string_view start_value,
        std::string_view end_value,
        ScanOrder order = ScanOrder::ASCENDING) const = 0;

    /// @brief Get index name
    /// @return Index name/identifier
    [[nodiscard]] virtual std::string getName() const = 0;

    /// @brief Get indexed field name
    /// @return Field name that this index covers
    [[nodiscard]] virtual std::string getFieldName() const = 0;

    /// @brief Get index statistics
    /// @return JSON string with statistics (size, cardinality, etc.)
    [[nodiscard]] virtual std::string getStatistics() const = 0;
};

/// @brief Vector similarity search result
struct VectorSearchResult {
    std::string primary_key;  ///< Primary key of the matching document
    float distance;           ///< Distance/similarity score
    
    VectorSearchResult(std::string pk, float dist) 
        : primary_key(std::move(pk)), distance(dist) {}
};

/// @brief Abstract interface for vector indexes
/// 
/// Vector indexes enable similarity search over high-dimensional embeddings.
/// Common implementations include HNSW, IVF, and PQ-based indexes.
class THEMIS_BASE_API IVectorIndex {
public:
    virtual ~IVectorIndex() = default;

    /// @brief Insert or update a vector entry
    /// @param primary_key The primary key of the document
    /// @param vector The embedding vector
    /// @return true on success, false on failure
    [[nodiscard]] virtual bool insert(std::string_view primary_key,
                       const std::vector<float>& vector) = 0;

    /// @brief Remove a vector entry
    /// @param primary_key The primary key
    /// @return true if entry existed and was removed, false otherwise
    [[nodiscard]] virtual bool remove(std::string_view primary_key) = 0;

    /// @brief Search for k nearest neighbors
    /// @param query_vector The query vector
    /// @param k Number of results to return
    /// @param filter Optional filter expression (injected evaluator)
    /// @return Top k results sorted by distance (closest first)
    [[nodiscard]] virtual std::vector<VectorSearchResult> search(
        const std::vector<float>& query_vector,
        uint32_t k,
        const IExpressionEvaluator* filter = nullptr) const = 0;

    /// @brief Range search (all vectors within distance threshold)
    /// @param query_vector The query vector
    /// @param max_distance Maximum distance threshold
    /// @param filter Optional filter expression
    /// @return All results within threshold
    [[nodiscard]] virtual std::vector<VectorSearchResult> rangeSearch(
        const std::vector<float>& query_vector,
        float max_distance,
        const IExpressionEvaluator* filter = nullptr) const = 0;

    /// @brief Get index name
    /// @return Index name/identifier
    [[nodiscard]] virtual std::string getName() const = 0;

    /// @brief Get vector dimension
    /// @return Dimension of indexed vectors
    [[nodiscard]] virtual uint32_t getDimension() const = 0;

    /// @brief Get index statistics
    /// @return JSON string with statistics (count, memory usage, etc.)
    [[nodiscard]] virtual std::string getStatistics() const = 0;
};

/// @brief Graph edge representation
struct GraphEdge {
    std::string from_node;  ///< Source node ID
    std::string to_node;    ///< Target node ID
    std::string edge_type;  ///< Edge type/label
    double weight;          ///< Edge weight (optional, 1.0 default)

    GraphEdge(std::string from, std::string to, 
             std::string type = "", double w = 1.0)
        : from_node(std::move(from))
        , to_node(std::move(to))
        , edge_type(std::move(type))
        , weight(w) {}
};

/// @brief Abstract interface for graph indexes
/// 
/// Graph indexes store and query graph structure (nodes and edges).
/// They enable efficient graph traversal queries.
class THEMIS_BASE_API IGraphIndex {
public:
    virtual ~IGraphIndex() = default;

    /// @brief Insert or update an edge
    /// @param edge The edge to insert
    /// @return true on success, false on failure
    [[nodiscard]] virtual bool insertEdge(const GraphEdge& edge) = 0;

    /// @brief Remove an edge
    /// @param from_node Source node ID
    /// @param to_node Target node ID
    /// @param edge_type Edge type (empty = all types)
    /// @return true if edge existed and was removed, false otherwise
    [[nodiscard]] virtual bool removeEdge(std::string_view from_node,
                           std::string_view to_node,
                           std::string_view edge_type = "") = 0;

    /// @brief Get outgoing edges from a node
    /// @param node_id Node ID
    /// @param edge_type Optional edge type filter
    /// @return Vector of outgoing edges
    [[nodiscard]] virtual std::vector<GraphEdge> getOutgoingEdges(
        std::string_view node_id,
        std::string_view edge_type = "") const = 0;

    /// @brief Get incoming edges to a node
    /// @param node_id Node ID
    /// @param edge_type Optional edge type filter
    /// @return Vector of incoming edges
    [[nodiscard]] virtual std::vector<GraphEdge> getIncomingEdges(
        std::string_view node_id,
        std::string_view edge_type = "") const = 0;

    /// @brief Find shortest path between two nodes
    /// @param from_node Source node
    /// @param to_node Target node
    /// @param edge_type Optional edge type filter
    /// @param max_depth Maximum search depth (0 = unlimited)
    /// @return Path as vector of node IDs, empty if no path exists
    [[nodiscard]] virtual std::vector<std::string> findShortestPath(
        std::string_view from_node,
        std::string_view to_node,
        std::string_view edge_type = "",
        uint32_t max_depth = 0) const = 0;

    /// @brief Get index name
    /// @return Index name/identifier
    [[nodiscard]] virtual std::string getName() const = 0;

    /// @brief Get index statistics
    /// @return JSON string with statistics (nodes, edges, etc.)
    [[nodiscard]] virtual std::string getStatistics() const = 0;
};

/// @brief Abstract interface for managing all indexes
/// 
/// The index manager coordinates multiple indexes and provides
/// a unified interface for query engines to access them.
/// 
/// Key responsibilities:
/// - Create and drop indexes
/// - Route queries to appropriate indexes
/// - Coordinate multi-index updates
/// - Provide query optimization hints
class IIndexManager {
public:
    virtual ~IIndexManager() = default;

    /// @brief Create a new secondary index
    /// @param name Index name
    /// @param field_name Field to index
    /// @param config Implementation-specific configuration
    /// @return Result containing pointer to created index, or Error on failure
    ///         Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST
    [[nodiscard]] virtual Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config = "") = 0;

    /// @brief Create a new vector index
    /// @param name Index name
    /// @param dimension Vector dimension
    /// @param config Implementation-specific configuration
    /// @return Result containing pointer to created index, or Error on failure
    ///         Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED, ERR_API_INVALID_REQUEST
    [[nodiscard]] virtual Result<IVectorIndex*> createVectorIndex(
        std::string_view name,
        uint32_t dimension,
        const std::string& config = "") = 0;

    /// @brief Create a new graph index
    /// @param name Index name
    /// @param config Implementation-specific configuration
    /// @return Result containing pointer to created index, or Error on failure
    ///         Possible errors: ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED
    [[nodiscard]] virtual Result<IGraphIndex*> createGraphIndex(
        std::string_view name,
        const std::string& config = "") = 0;

    /// @brief Get an existing secondary index by name
    /// @param name Index name
    /// @return Result containing pointer to index, or Error if not found
    ///         Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
    [[nodiscard]] virtual Result<ISecondaryIndex*> getSecondaryIndex(std::string_view name) const = 0;

    /// @brief Get an existing vector index by name
    /// @param name Index name
    /// @return Result containing pointer to index, or Error if not found
    ///         Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
    [[nodiscard]] virtual Result<IVectorIndex*> getVectorIndex(std::string_view name) const = 0;

    /// @brief Get an existing graph index by name
    /// @param name Index name
    /// @return Result containing pointer to index, or Error if not found
    ///         Possible errors: ERR_INDEX_NOT_FOUND, ERR_INDEX_INVALID_TYPE
    [[nodiscard]] virtual Result<IGraphIndex*> getGraphIndex(std::string_view name) const = 0;

    /// @brief Drop an index by name
    /// @param name Index name
    /// @return Result<void> indicating success or error
    ///         Possible errors: ERR_INDEX_NOT_FOUND
    [[nodiscard]] virtual Result<void> dropIndex(std::string_view name) = 0;

    /// @brief List all indexes
    /// @return Vector of index names
    [[nodiscard]] virtual std::vector<std::string> listIndexes() const = 0;

    /// @brief Get type of an index
    /// @param name Index name
    /// @return Result<IndexType> with index type, or ERR_INDEX_NOT_FOUND if index doesn't exist
    [[nodiscard]] virtual Result<IndexType> getIndexType(std::string_view name) const = 0;
};

} // namespace themis
