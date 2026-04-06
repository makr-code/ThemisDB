/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            weaviate_adapter.hpp                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:06:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     270                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
    • 261a690e71  2026-02-26  feat(chimera): implement PostgreSQL + pgvector adapter fo... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file weaviate_adapter.hpp
 * @brief Weaviate native vector database adapter for CHIMERA Suite
 *
 * @details
 * This file provides an implementation of the CHIMERA IDatabaseAdapter
 * interface for Weaviate. The adapter targets Weaviate v1.x and supports
 * native HNSW vector similarity search as its primary workload, with
 * document storage and full-text (BM25) search as secondary capabilities.
 *
 * Supported capabilities:
 *   - Vector similarity search (primary, HNSW index)
 *   - Document/object storage with metadata
 *   - Full-text search (BM25)
 *   - Batch insert/update operations
 *   - Secondary indexes
 *
 * Unsupported (returns NOT_IMPLEMENTED):
 *   - Relational/SQL queries
 *   - Graph traversal
 *   - ACID transactions
 *
 * Connection string formats:
 *   http://host[:port]
 *   https://host[:port]
 *
 * When a live Weaviate server is not available the adapter operates
 * in an in-process simulation mode backed by std::unordered_map, which
 * is sufficient for unit testing without a running server.
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_WEAVIATE_ADAPTER_HPP
#define CHIMERA_WEAVIATE_ADAPTER_HPP

#include "chimera/database_adapter.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace chimera {

/**
 * @class WeaviateAdapter
 * @brief Weaviate native vector database implementation of the CHIMERA
 *        IDatabaseAdapter interface
 *
 * @details Implements vector search, document storage, and full-text search.
 *          Relational, graph, and transaction operations return NOT_IMPLEMENTED
 *          because Weaviate does not natively support those workloads.
 *
 * @note The production implementation communicates with a live Weaviate
 *       instance via the Weaviate REST/GraphQL API. When the HTTP client
 *       is not linked the adapter operates in an in-process simulation mode
 *       suitable for unit testing without a running server.
 */
class WeaviateAdapter : public IDatabaseAdapter {
public:
    WeaviateAdapter();
    ~WeaviateAdapter() override;

    // Non-copyable
    WeaviateAdapter(const WeaviateAdapter&) = delete;
    WeaviateAdapter& operator=(const WeaviateAdapter&) = delete;

    // -----------------------------------------------------------------------
    // Connection Management
    // -----------------------------------------------------------------------

    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) override;

    Result<bool> disconnect() override;
    bool is_connected() const override;

    // -----------------------------------------------------------------------
    // IRelationalAdapter (not supported – returns NOT_IMPLEMENTED)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // IVectorAdapter (primary capability)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // IGraphAdapter (not supported – returns NOT_IMPLEMENTED)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // IDocumentAdapter (Weaviate object store)
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // ITransactionAdapter (not supported – returns NOT_IMPLEMENTED)
    // -----------------------------------------------------------------------

    Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) override;

    Result<bool> commit_transaction(const std::string& transaction_id) override;
    Result<bool> rollback_transaction(const std::string& transaction_id) override;

    // -----------------------------------------------------------------------
    // ISystemInfoAdapter
    // -----------------------------------------------------------------------

    Result<SystemInfo> get_system_info() const override;
    Result<SystemMetrics> get_metrics() const override;
    bool has_capability(Capability cap) const override;
    std::vector<Capability> get_capabilities() const override;

private:
    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    bool connected_ = false;
    std::string connection_string_;

    // In-process vector store for simulation.
    // Maps collection_name -> vector of (Document encoding vector + metadata).
    mutable std::mutex vector_mutex_;
    std::unordered_map<std::string, std::vector<Document>> vector_store_;

    // In-process document store for object simulation.
    // Maps collection_name -> vector of Documents.
    mutable std::mutex doc_mutex_;
    std::unordered_map<std::string, std::vector<Document>> document_store_;

    // Monotonic counters for ID generation
    std::atomic<uint64_t> next_obj_id_{1};

    // API key extracted from connect() options (stored but never exposed via
    // get_system_info() to prevent credential leakage in logs or core dumps).
    bool has_api_key_ = false;

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /// Validate that the connection string uses http:// or https:// scheme.
    static bool is_valid_connection_string(const std::string& connection_string);

    /// Generate a unique Weaviate object ID.
    std::string generate_object_id();

    /// Compute cosine similarity between two float vectors.
    static double cosine_similarity(const std::vector<float>& a,
                                    const std::vector<float>& b);

    /// Return true if a document matches every key-value pair in filter.
    static bool document_matches(const Document& doc,
                                 const std::map<std::string, Scalar>& filter);
};

} // namespace chimera

#endif // CHIMERA_WEAVIATE_ADAPTER_HPP
