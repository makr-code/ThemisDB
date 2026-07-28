/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgresql_adapter.hpp                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:06:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     288                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 261a690e71  2026-02-26  feat(chimera): implement PostgreSQL + pgvector adapter fo... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 5554ae8cdc  2026-02-22  Code audit and bugfix: fix document_matches id field, mas... ║
    • d34adc2bf9  2026-02-22  Implement MongoDB vendor adapter for Chimera module ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file postgresql_adapter.hpp
 * @brief PostgreSQL + pgvector adapter implementation for CHIMERA Suite
 *
 * @details
 * This file provides an implementation of the CHIMERA IDatabaseAdapter
 * interface for PostgreSQL 14+. The adapter supports relational SQL
 * operations as the primary workload and optional vector similarity
 * search via the pgvector extension.
 *
 * Supported capabilities:
 *   - Relational/SQL queries (primary)
 *   - Vector similarity search (pgvector: L2, inner-product, cosine)
 *   - Document storage via JSONB columns
 *   - Full-text search
 *   - Multi-statement ACID transactions
 *   - Batch insert/update operations
 *   - Secondary indexes
 *
 * Unsupported (returns NOT_IMPLEMENTED):
 *   - Graph traversal
 *
 * Connection string format:
 *   postgresql://[user:pass@]host[:port][/dbname][?options]
 *   postgres://[user:pass@]host[:port][/dbname][?options]
 *
 * When a live PostgreSQL server is not available the adapter operates
 * in an in-process simulation mode backed by std::unordered_map, which
 * is sufficient for unit testing without a running server.
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_POSTGRESQL_ADAPTER_HPP
#define CHIMERA_POSTGRESQL_ADAPTER_HPP

#include "chimera/database_adapter.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace chimera {

/**
 * @class PostgreSQLAdapter
 * @brief PostgreSQL + pgvector implementation of the CHIMERA IDatabaseAdapter
 *
 * @details Implements relational, vector (pgvector), document (JSONB), and
 *          transaction operations. Graph operations return NOT_IMPLEMENTED
 *          because PostgreSQL does not natively support graph traversal.
 *
 * @note The production implementation communicates with a live PostgreSQL
 *       instance via libpqxx. When the driver is not linked the adapter
 *       operates in an in-process simulation mode suitable for unit testing
 *       without a running server.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Keep adapter contract testable without libpqxx/PostgreSQL runtime.
 * Activation: Active when libpqxx integration is not linked/available.
 * Production Delta: Uses in-memory simulation instead of PostgreSQL-backed execution.
 * Removal Plan: Keep as test fallback; production deployments should use libpqxx integration.
 */
class PostgreSQLAdapter : public IDatabaseAdapter {
public:
    PostgreSQLAdapter();
    ~PostgreSQLAdapter() override;

    // Non-copyable
    PostgreSQLAdapter(const PostgreSQLAdapter&) = delete;
    PostgreSQLAdapter& operator=(const PostgreSQLAdapter&) = delete;

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
    // IRelationalAdapter (primary capability)
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
    // IVectorAdapter (pgvector extension)
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
    // IDocumentAdapter (JSONB simulation)
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
    // ITransactionAdapter
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
    std::string database_name_;

    // In-process relational table store used when libpqxx is not linked.
    // Maps table_name -> vector of RelationalRows.
    mutable std::mutex table_mutex_;
    std::unordered_map<std::string, std::vector<RelationalRow>> table_store_;

    // In-process vector store for pgvector simulation.
    // Maps collection_name -> vector of (Vector, id) pairs.
    mutable std::mutex vector_mutex_;
    std::unordered_map<std::string, std::vector<Document>> vector_store_;

    // In-process document store for JSONB simulation.
    // Maps collection_name -> vector of Documents.
    mutable std::mutex doc_mutex_;
    std::unordered_map<std::string, std::vector<Document>> document_store_;

    // Active transaction IDs
    mutable std::mutex txn_mutex_;
    std::unordered_map<std::string, bool> active_transactions_;

    // Monotonic counters for ID generation
    std::atomic<uint64_t> next_row_id_{1};
    std::atomic<uint64_t> next_doc_id_{1};
    std::atomic<uint64_t> next_txn_id_{1};

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /// Mask user credentials in a PostgreSQL connection string so that the
    /// stored string cannot leak passwords via core dumps or logs.
    /// postgresql://user:pass@host is transformed to postgresql://***:***@host.
    static std::string mask_credentials(const std::string& connection_string);

    /// Parse the database name from a PostgreSQL connection string.
    static std::string parse_database_name(const std::string& connection_string);

    /// Generate a unique row ID.
    std::string generate_row_id();

    /// Generate a unique document ID.
    std::string generate_document_id();

    /// Generate a unique transaction ID.
    std::string generate_transaction_id();

    /// Compute cosine similarity between two float vectors.
    static double cosine_similarity(const std::vector<float>& a,
                                    const std::vector<float>& b);

    /// Return true if a document matches every key-value pair in filter.
    static bool document_matches(const Document& doc,
                                 const std::map<std::string, Scalar>& filter);
};

} // namespace chimera

#endif // CHIMERA_POSTGRESQL_ADAPTER_HPP
