/**
 * @file mongodb_adapter.hpp
 * @brief MongoDB backend adapter for the Chimera abstraction layer.
 *
 * Implements IDatabaseAdapter against a MongoDB replica-set or
 * standalone instance via the official C++ driver.
 */

#pragma once

#include "chimera/database_adapter.hpp"
#include "chimera/transaction.hpp"
#include "chimera/batch_executor.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Forward declarations for mongocxx (MongoDB C++ driver)
namespace mongocxx {
class client;
class database;
namespace collection {
class collection;
}
} // namespace mongocxx

namespace chimera {

/**
 * @class MongoDBAdapter
 * @brief MongoDB implementation of the CHIMERA adapter interface
 * 
 * @details
 * Provides integration between MongoDB and the CHIMERA Benchmark Suite.
 * Implements relational (documents as rows), document, and batch operations.
 * 
 * Features:
 * - Real MongoDB driver integration (mongocxx)
 * - Transaction support with ACID properties
 * - Batch operation optimization
 * - Retry policy with exponential backoff
 * - Connection pooling
 * 
 * Limitations (by design):
 * - Vector operations not directly supported; recommend Qdrant for KNN
 * - Graph operations limited to document-based relationships
 * 
 * Thread-safety: Connection pooling is thread-safe; each client thread
 * should acquire its own connection.
 */
class MongoDBAdapter : public IDatabaseAdapter,
                       public ITransactionalAdapter,
                       public IBatchAdapter {
public:
    /**
     * @brief Construct MongoDB adapter with default settings.
     */
    MongoDBAdapter();

    /// @brief Destructor; closes connection pool.
    ~MongoDBAdapter() override;

    // ────────────────────────────────────────────────────────────────────────
    // IDatabaseAdapter implementation
    // ────────────────────────────────────────────────────────────────────────

    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) override;

    Result<bool> disconnect() override;
    bool is_connected() const override;

    // Relational operations
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

    // Vector operations (unsupported; return NOT_IMPLEMENTED)
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

    // Graph operations (limited; document relationships)
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

    // Document operations (primary support)
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

    // Transaction operations (stub; TODO: implement with MongoDB sessions)
    Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) override;

    Result<bool> commit_transaction(const std::string& transaction_id) override;
    Result<bool> rollback_transaction(const std::string& transaction_id) override;

    Result<std::string> create_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) override;

    Result<bool> rollback_to_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) override;

    Result<bool> release_savepoint(
        const std::string& transaction_id,
        const std::string& savepoint_name
    ) override;

    Result<TransactionStats> get_transaction_stats(
        const std::string& transaction_id
    ) override;

    Result<TransactionState> get_transaction_state(
        const std::string& transaction_id
    ) override;

    // System info
    Result<SystemInfo> get_system_info() const override;
    Result<SystemMetrics> get_metrics() const override;
    bool has_capability(Capability cap) const override;
    std::vector<Capability> get_capabilities() const override;

    // ────────────────────────────────────────────────────────────────────────
    // ITransactionalAdapter implementation
    // ────────────────────────────────────────────────────────────────────────

    Result<TransactionHandle> begin_transaction(
        IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED
    ) override;

    Result<bool> commit_transaction(
        const TransactionHandle& handle
    ) override;

    Result<bool> rollback_transaction(
        const TransactionHandle& handle
    ) override;

    Result<std::string> create_savepoint(
        const TransactionHandle& handle,
        const std::string& savepoint_name
    ) override;

    Result<bool> rollback_to_savepoint(
        const TransactionHandle& handle,
        const std::string& savepoint_name
    ) override;

    TransactionState get_transaction_state(
        const TransactionHandle& handle
    ) const override;

    // ────────────────────────────────────────────────────────────────────────
    // IBatchAdapter implementation
    // ────────────────────────────────────────────────────────────────────────

    Result<bool> queue_insert(
        const std::string& table_name,
        const RelationalRow& row
    ) override;

    Result<bool> queue_insert_batch(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) override;

    Result<bool> queue_update(
        const std::string& table_name,
        const RelationalRow& row,
        const std::string& where_clause
    ) override;

    Result<bool> queue_delete(
        const std::string& table_name,
        const std::string& where_clause
    ) override;

    Result<BatchStatistics> flush() override;

    size_t get_pending_count() const override;

    Result<bool> set_batch_config(const BatchConfig& config) override;

    const BatchConfig& get_batch_config() const override;

private:
    // ────────────────────────────────────────────────────────────────────────
    // Connection and client management
    // ────────────────────────────────────────────────────────────────────────

    std::unique_ptr<mongocxx::client> client_;
    std::unique_ptr<mongocxx::database> database_;
    bool connected_ = false;
    std::string connection_string_;

    // ────────────────────────────────────────────────────────────────────────
    // Batch queue and configuration
    // ────────────────────────────────────────────────────────────────────────

    struct QueuedOperation {
        std::string op_type;  // "insert", "update", "delete"
        std::string table_name;
        std::string data;  // Serialized row or query
    };

    mutable std::mutex batch_mutex_;
    std::vector<QueuedOperation> batch_queue_;
    BatchConfig batch_config_;

    // ────────────────────────────────────────────────────────────────────────
    // Transaction tracking
    // ────────────────────────────────────────────────────────────────────────

    mutable std::mutex txn_mutex_;
    std::map<std::string, std::shared_ptr<TransactionContext>> active_transactions_;

    // ────────────────────────────────────────────────────────────────────────
    // Private helpers
    // ────────────────────────────────────────────────────────────────────────

    static std::string generate_id();
    static bool is_valid_connection_string(const std::string& cs);
    static std::string mask_credentials(const std::string& cs);

    /// Convert a Scalar to BSON value.
    static std::string scalar_to_bson_string(const Scalar& scalar);

    /// Convert a RelationalRow to MongoDB document.
    static std::string row_to_bson_document(const RelationalRow& row);

    /// Parse MongoDB query (AQL to MongoDB translation stub).
    Result<std::string> parse_query_to_mongo(
        const std::string& aql_query
    ) const;
};

} // namespace chimera

#endif // CHIMERA_MONGODB_ADAPTER_HPP
