/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_adapter.hpp                               ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-12 11:39:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     220+                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 1aba82430  2026-02-28  fix(chimera): mask credentials in ThemisDBAdapter::connec... ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file themisdb_adapter.hpp
 * @brief ThemisDB adapter implementation for CHIMERA Suite
 *
 * @details
 * This file provides the production implementation of the CHIMERA adapter
 * interface for ThemisDB. The adapter supports two operating modes:
 *
 *  1. **In-process simulation mode** (default constructor): Uses lightweight
 *     in-memory collections so the adapter can be exercised in unit tests
 *     without a live ThemisDB instance.
 *
 *  2. **Wired engine mode** (engine-injection constructor): Accepts optional
 *     pointers to ThemisDB's native engine components (QueryEngine,
 *     VectorIndexManager, GraphIndexManager) so all operations are
 *     delegated directly to the production back-end.
 *
 * Other database systems should follow this pattern to integrate with
 * the CHIMERA Benchmark Suite.
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_THEMISDB_ADAPTER_HPP
#define CHIMERA_THEMISDB_ADAPTER_HPP

#include "chimera/database_adapter.hpp"

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// Forward declarations for optional ThemisDB engine injection.
// Consumers that only include this header do NOT need the full ThemisDB
// engine headers; the engine types are resolved in the translation unit.
namespace themis {
class QueryEngine;
class VectorIndexManager;
class GraphIndexManager;
} // namespace themis

namespace chimera {

/**
 * @class ThemisDBAdapter
 * @brief ThemisDB implementation of the CHIMERA adapter interface
 *
 * @details This adapter provides integration between ThemisDB and the
 *          CHIMERA Benchmark Suite. It implements all required interfaces
 *          and marks unsupported features with NOT_IMPLEMENTED.
 *
 *          When constructed with the default constructor the adapter operates
 *          in in-process simulation mode: all data is kept in lightweight
 *          in-memory collections so that the full CHIMERA API surface can be
 *          exercised without a live ThemisDB server.
 *
 *          When constructed with engine pointers the adapter delegates every
 *          operation to the supplied ThemisDB back-end components, enabling
 *          true production-grade integration.
 */
class ThemisDBAdapter : public IDatabaseAdapter {
public:
    /**
     * @brief Default constructor — in-process simulation mode.
     *
     * All CHIMERA operations are served from lightweight in-memory
     * collections.  No live ThemisDB server is required.
     */
    ThemisDBAdapter() = default;
    ~ThemisDBAdapter() override = default;

    /**
     * @brief Engine-injection constructor — wired production mode.
     *
     * Accepts optional pointers to ThemisDB's native engine components.
     * When an engine pointer is non-null the corresponding operations are
     * delegated to the real back-end; otherwise the in-memory fallback is
     * used for that subsystem.
     *
     * @param query_engine   Optional ThemisDB QueryEngine for AQL execution.
     * @param vector_index   Optional VectorIndexManager for kNN search.
     * @param graph_index    Optional GraphIndexManager for graph traversal.
     */
    explicit ThemisDBAdapter(
        themis::QueryEngine*       query_engine,
        themis::VectorIndexManager* vector_index  = nullptr,
        themis::GraphIndexManager*  graph_index   = nullptr
    );
    
    // Connection Management
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) override;
    
    Result<bool> disconnect() override;
    bool is_connected() const override;
    
    // IRelationalAdapter
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) override;
    
    Result<size_t> insert_row(
        const std::string& table_name,
        const RelationalRow& row
    ) override;
    
    Result<size_t> batch_insert(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) override;
    
    Result<QueryStatistics> get_query_statistics() const override;
    
    // IVectorAdapter
    Result<std::string> insert_vector(
        const std::string& collection,
        const Vector& vector
    ) override;
    
    Result<size_t> batch_insert_vectors(
        const std::string& collection,
        const std::vector<Vector>& vectors
    ) override;
    
    Result<std::vector<std::pair<Vector, double>>> search_vectors(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) override;
    
    Result<bool> create_index(
        const std::string& collection,
        size_t dimensions,
        const std::map<std::string, Scalar>& index_params = {}
    ) override;
    
    // IGraphAdapter
    Result<std::string> insert_node(const GraphNode& node) override;
    Result<std::string> insert_edge(const GraphEdge& edge) override;
    
    Result<GraphPath> shortest_path(
        const std::string& source_id,
        const std::string& target_id,
        size_t max_depth = 10
    ) override;
    
    Result<std::vector<GraphNode>> traverse(
        const std::string& start_id,
        size_t max_depth,
        const std::vector<std::string>& edge_labels = {}
    ) override;
    
    Result<std::vector<GraphPath>> execute_graph_query(
        const std::string& query,
        const std::map<std::string, Scalar>& params = {}
    ) override;
    
    // IDocumentAdapter
    Result<std::string> insert_document(
        const std::string& collection,
        const Document& doc
    ) override;
    
    Result<size_t> batch_insert_documents(
        const std::string& collection,
        const std::vector<Document>& docs
    ) override;
    
    Result<std::vector<Document>> find_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        size_t limit = 100
    ) override;
    
    Result<size_t> update_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        const std::map<std::string, Scalar>& updates
    ) override;
    
    // ITransactionAdapter
    Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) override;
    
    Result<bool> commit_transaction(const std::string& transaction_id) override;
    Result<bool> rollback_transaction(const std::string& transaction_id) override;
    
    // ISystemInfoAdapter
    Result<SystemInfo> get_system_info() const override;
    Result<SystemMetrics> get_metrics() const override;
    bool has_capability(Capability cap) const override;
    std::vector<Capability> get_capabilities() const override;

private:
    // ── Connection state ────────────────────────────────────────────────────
    bool connected_ = false;
    std::string connection_string_;

    // ── Injected ThemisDB engine components (optional) ──────────────────────
    // When non-null, the corresponding operations are delegated to the real
    // ThemisDB back-end instead of the in-memory simulation layer.
    themis::QueryEngine*        query_engine_  = nullptr;
    themis::VectorIndexManager* vector_index_  = nullptr;
    themis::GraphIndexManager*  graph_index_   = nullptr;

    // ── In-memory simulation collections ────────────────────────────────────
    // Used when no engine is injected (unit-test / simulation mode).
    // All accesses must be guarded by store_mutex_ for thread safety.
    mutable std::mutex store_mutex_;

    // Relational: table_name -> rows
    std::map<std::string, std::vector<RelationalRow>> table_store_;

    // Vector: collection_name -> [(id, Vector)]
    std::map<std::string, std::vector<std::pair<std::string, Vector>>> vector_store_;

    // Graph nodes and edges
    std::map<std::string, GraphNode>  graph_nodes_;
    std::map<std::string, GraphEdge>  graph_edges_;
    // Adjacency list: source_id -> [(edge_id, target_id)]
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> adj_out_;

    // Document: collection_name -> (doc_id -> Document)
    std::map<std::string, std::map<std::string, Document>> doc_store_;

    // Active transactions: txn_id -> options
    std::map<std::string, TransactionOptions> active_transactions_;

    // ── Private helpers ──────────────────────────────────────────────────────

    /// Generate a new unique ID (UUID v4).
    static std::string generate_id();

    // Credential security helpers
    static bool is_valid_connection_string(const std::string& connection_string);
    static std::string mask_credentials(const std::string& connection_string);
};

} // namespace chimera

#endif // CHIMERA_THEMISDB_ADAPTER_HPP
