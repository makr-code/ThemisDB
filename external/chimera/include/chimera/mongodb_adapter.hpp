/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mongodb_adapter.hpp                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-06 04:06:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     280                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 29a0ee06c5  2026-03-12  feat(chimera): wire THEMIS_ENABLE_* driver blocks, add Co... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 74d8f7c17f  2026-02-28  fix(chimera): resolve MongoDB adapter quality metrics - r... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 5554ae8cdc  2026-02-22  Code audit and bugfix: fix document_matches id field, mas... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file mongodb_adapter.hpp
 * @brief MongoDB adapter implementation for CHIMERA Suite
 *
 * @details
 * This file provides an implementation of the CHIMERA adapter interface for
 * MongoDB. The adapter targets MongoDB 4.4+ and supports Atlas Vector Search
 * for vector similarity operations.
 *
 * Supported capabilities:
 *   - Document storage and querying (primary)
 *   - Full-text search
 *   - Multi-document ACID transactions (MongoDB 4.0+)
 *   - Batch insert/update operations
 *   - Secondary indexes
 *   - Vector similarity search (Atlas Vector Search / $vectorSearch)
 *
 * Unsupported (returns NOT_IMPLEMENTED):
 *   - Relational/SQL queries
 *   - Graph traversal
 *
 * Connection string formats:
 *   mongodb://[user:pass@]host[:port][/dbname][?options]
 *   mongodb+srv://[user:pass@]host[/dbname][?options]
 *
 * @copyright MIT License
 */

#ifndef CHIMERA_MONGODB_ADAPTER_HPP
#define CHIMERA_MONGODB_ADAPTER_HPP

#include "chimera/connection_pool.hpp"
#include "chimera/database_adapter.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace chimera {

/**
 * @class MongoDBAdapter
 * @brief MongoDB implementation of the CHIMERA IDatabaseAdapter interface
 *
 * @details Implements document-oriented operations, vector search via Atlas
 *          Vector Search, and multi-document transactions. Relational and
 *          graph operations return ErrorCode::NOT_IMPLEMENTED because MongoDB
 *          does not natively support those workloads.
 *
 * @note The production implementation communicates with a live MongoDB
 *       instance via the MongoDB C++ driver (mongocxx). When the driver is
 *       not linked the adapter operates in an in-process simulation mode
 *       suitable for unit testing without a running server.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Provide a deterministic adapter path when mongocxx/runtime MongoDB is unavailable.
 * Activation: Active when MongoDB driver integration is not linked/available.
 * Production Delta: Uses in-memory simulation state instead of live MongoDB persistence.
 * Removal Plan: Keep as test fallback; production deployments should use mongocxx-backed mode.
 */
class MongoDBAdapter : public IDatabaseAdapter {
public:
    MongoDBAdapter();
    ~MongoDBAdapter() override;

    // Non-copyable
    MongoDBAdapter(const MongoDBAdapter&) = delete;
    MongoDBAdapter& operator=(const MongoDBAdapter&) = delete;

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
    // IVectorAdapter (Atlas Vector Search)
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
    // IDocumentAdapter
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

    // Connection pool configuration (configurable before connect()).
    ConnectionPoolConfig pool_config_;

    // Opaque driver-specific state (mongocxx client / pool).
    // Defined in mongodb_adapter.cpp; nullptr in simulation mode.
    struct MongoState;
    std::unique_ptr<MongoState> mongo_state_;

    // In-process document store used when the mongocxx driver is not linked.
    // Maps collection_name -> vector of Documents.
    mutable std::mutex store_mutex_;
    std::unordered_map<std::string, std::vector<Document>> document_store_;

    // Active transaction IDs (lightweight in-process tracking)
    mutable std::mutex txn_mutex_;
    std::unordered_map<std::string, bool> active_transactions_;

    // Monotonic counters – use atomics to avoid holding a broader mutex
    std::atomic<uint64_t> next_doc_id_{1};
    std::atomic<uint64_t> next_txn_id_{1};

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /// Mask user credentials inside a MongoDB connection string so that the
    /// stored string cannot leak passwords (e.g. via core dumps or logs).
    /// The format mongodb://user:pass@host is transformed to
    /// mongodb://***:***@host.
    static std::string mask_credentials(const std::string& connection_string);

    /// Parse the database name from a MongoDB connection string.
    static std::string parse_database_name(const std::string& connection_string);

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

#endif // CHIMERA_MONGODB_ADAPTER_HPP
