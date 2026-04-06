/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            elasticsearch_adapter.hpp                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:06:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     306                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5b182fc5c6  2026-02-28  Add Elasticsearch adapter: header, implementation, tests,... ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
    • 261a690e71  2026-02-26  feat(chimera): implement PostgreSQL + pgvector adapter fo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file elasticsearch_adapter.hpp
 * @brief Elasticsearch full-text and vector search adapter for CHIMERA Suite
 *
 * @details
 * This file provides an implementation of the CHIMERA IDatabaseAdapter
 * interface for Elasticsearch 8.x. The adapter supports:
 *
 * Primary capabilities:
 *   - Full-text search (BM25 / inverted index)
 *   - Vector similarity search (dense_vector / kNN via HNSW)
 *   - Document/index storage with metadata
 *   - Batch insert/update operations
 *   - Secondary indexes
 *
 * Unsupported (returns NOT_IMPLEMENTED):
 *   - Relational/SQL queries (ES|QL is out of scope for this adapter)
 *   - Graph traversal
 *   - ACID transactions (Elasticsearch provides optimistic concurrency only)
 *
 * Connection string formats:
 *   http://host[:port]
 *   https://host[:port]
 *
 * When a live Elasticsearch server is not available the adapter operates
 * in an in-process simulation mode backed by std::unordered_map, which is
 * sufficient for unit testing without a running server.
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_ELASTICSEARCH_ADAPTER_HPP
#define CHIMERA_ELASTICSEARCH_ADAPTER_HPP

#include "chimera/database_adapter.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace chimera {

/**
 * @class ElasticsearchAdapter
 * @brief Elasticsearch full-text and vector search implementation of the
 *        CHIMERA IDatabaseAdapter interface
 *
 * @details Implements full-text search, vector similarity search (kNN), and
 *          document storage.  Relational, graph, and transaction operations
 *          return NOT_IMPLEMENTED because Elasticsearch does not natively
 *          support those workloads.
 *
 * @note The production implementation communicates with a live Elasticsearch
 *       instance via the Elasticsearch REST API.  When the HTTP client is not
 *       linked the adapter operates in an in-process simulation mode suitable
 *       for unit testing without a running server.
 */
class ElasticsearchAdapter : public IDatabaseAdapter {
public:
    ElasticsearchAdapter();
    ~ElasticsearchAdapter() override;

    // Non-copyable
    ElasticsearchAdapter(const ElasticsearchAdapter&) = delete;
    ElasticsearchAdapter& operator=(const ElasticsearchAdapter&) = delete;

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
    // IVectorAdapter (kNN dense vector search)
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
    // IDocumentAdapter (Elasticsearch index/document store)
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

    // -----------------------------------------------------------------------
    // Elasticsearch-specific: full-text search
    // -----------------------------------------------------------------------

    /**
     * @brief Perform a full-text search using BM25 scoring.
     *
     * @details In simulation mode this checks whether every whitespace-delimited
     *          term from @p query_text appears (case-insensitive) in any string
     *          field of each document.  Production implementations should map
     *          this call to an Elasticsearch "match" query.
     *
     * @param index   Elasticsearch index name.
     * @param field   Field to search (use "_all" to simulate multi-field search).
     * @param query_text  Free-form search text.
     * @param limit   Maximum number of results to return.
     * @return Matching documents ordered by (simulated) relevance score.
     */
    Result<std::vector<Document>> full_text_search(
        const std::string& index,
        const std::string& field,
        const std::string& query_text,
        size_t limit = 10
    );

private:
    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    bool connected_ = false;
    std::string connection_string_;

    // In-process vector store (collection → encoded documents).
    mutable std::mutex vector_mutex_;
    std::unordered_map<std::string, std::vector<Document>> vector_store_;

    // In-process document store (index → documents).
    mutable std::mutex doc_mutex_;
    std::unordered_map<std::string, std::vector<Document>> document_store_;

    // Monotonic counter for ID generation.
    std::atomic<uint64_t> next_doc_id_{1};

    // API key / bearer token (stored only as a presence flag to prevent
    // credential leakage via get_system_info() or log outputs).
    bool has_api_key_ = false;

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /// Validate that the connection string uses http:// or https:// scheme.
    static bool is_valid_connection_string(const std::string& connection_string);

    /// Generate a unique Elasticsearch document ID.
    std::string generate_doc_id();

    /// Compute cosine similarity between two float vectors.
    static double cosine_similarity(const std::vector<float>& a,
                                    const std::vector<float>& b);

    /// Return true if a document matches every key-value pair in filter.
    static bool document_matches(const Document& doc,
                                 const std::map<std::string, Scalar>& filter);

    /**
     * @brief Check whether a document's string fields contain all search terms.
     *
     * @param doc        Document to inspect.
     * @param field      Specific field name, or "_all" to search every field.
     * @param terms      Whitespace-delimited search terms (lower-cased by caller).
     * @return true if all terms are found in the target field(s).
     */
    static bool document_contains_terms(
        const Document& doc,
        const std::string& field,
        const std::vector<std::string>& terms
    );
};

} // namespace chimera

#endif // CHIMERA_ELASTICSEARCH_ADAPTER_HPP
