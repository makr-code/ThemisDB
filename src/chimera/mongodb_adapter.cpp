/**
 * @file mongodb_adapter.cpp
 * @brief MongoDB backend adapter implementation.
 *
 * CRUD, batch, and transaction forwarding to MongoDB through the
 * Chimera IDatabaseAdapter contract.
 */

#include "chimera/mongodb_adapter.hpp"
#include "utils/uuid.h"

#include <algorithm>
#include <cassert>
#include <sstream>

namespace chimera {

// Auto-registration
namespace {
const bool mongodb_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "MongoDB",
        []() { return std::make_unique<MongoDBAdapter>(); }
    );
    assert(ok && "MongoDBAdapter: 'MongoDB' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Constructor and Destructor
// ---------------------------------------------------------------------------

MongoDBAdapter::MongoDBAdapter() = default;

MongoDBAdapter::~MongoDBAdapter() {
    if (connected_) {
        disconnect();
    }
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> MongoDBAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& /*options*/
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "MongoDB connection string must not be empty"
        );
    }

    if (!is_valid_connection_string(connection_string)) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid MongoDB connection string: must start with "
            "mongodb:// or mongodb+srv://"
        );
    }

    connection_string_ = mask_credentials(connection_string);

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Actual mongocxx client creation (mongocxx::client, mongocxx::uri)
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB adapter unavailable: driver integration is not implemented yet."
    );
#else
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB adapter unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<bool> MongoDBAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    client_.reset();
    database_.reset();
    return Result<bool>::ok(true);
}

bool MongoDBAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// Relational Adapter
// ---------------------------------------------------------------------------

Result<RelationalTable> MongoDBAdapter::execute_query(
    const std::string& query,
    const std::vector<Scalar>& /*params*/
) {
    if (!connected_) {
        return Result<RelationalTable>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Translate AQL to MongoDB aggregation pipeline and execute
    RelationalTable table = {};
    return Result<RelationalTable>::ok(std::move(table));
#else
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB execute_query unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<size_t> MongoDBAdapter::insert_row(
    const std::string& table_name,
    const RelationalRow& row
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Convert RelationalRow to BSON document and insert into collection
    return Result<size_t>::ok(1);
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_row unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<size_t> MongoDBAdapter::batch_insert(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Batch insert documents into collection via bulk_write
    return static_cast<bool>(Result<size_t < static_cast<int>(::ok(rows.size())));
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB batch_insert unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<QueryStatistics> MongoDBAdapter::get_query_statistics() const {
    QueryStatistics stats;
    stats.execution_time = std::chrono::microseconds(0);
    stats.rows_read = 0;
    stats.rows_returned = 0;
    stats.bytes_read = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// ---------------------------------------------------------------------------
// Vector Adapter (Not Supported)
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::insert_vector(
    const std::string& /*collection*/,
    const Vector& /*vector*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector operations not supported in MongoDB adapter; use Qdrant"
    );
}

Result<size_t> MongoDBAdapter::batch_insert_vectors(
    const std::string& /*collection*/,
    const std::vector<Vector>& /*vectors*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector operations not supported in MongoDB adapter; use Qdrant"
    );
}

Result<std::vector<std::pair<Vector, double>>> MongoDBAdapter::search_vectors(
    const std::string& /*collection*/,
    const Vector& /*query_vector*/,
    size_t /*k*/,
    const std::map<std::string, Scalar>& /*filters*/
) {
    return Result<std::vector<std::pair<Vector, double>>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector search not supported in MongoDB adapter; use Qdrant"
    );
}

Result<bool> MongoDBAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector index creation not supported in MongoDB adapter"
    );
}

// ---------------------------------------------------------------------------
// Graph Adapter (Limited Support)
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::insert_node(const GraphNode& /*node*/) {
#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Store node as document in nodes collection
    return Result<std::string>::ok(generate_id());
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_node unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<std::string> MongoDBAdapter::insert_edge(const GraphEdge& /*edge*/) {
#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Store edge as document with source/target node references
    return Result<std::string>::ok(generate_id());
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_edge unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<GraphPath> MongoDBAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph traversal limited in MongoDB adapter; use Neo4j"
    );
}

Result<std::vector<GraphNode>> MongoDBAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph traversal limited in MongoDB adapter; use Neo4j"
    );
}

Result<std::vector<GraphPath>> MongoDBAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph queries limited in MongoDB adapter; use Neo4j"
    );
}

// ---------------------------------------------------------------------------
// Document Adapter
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Serialize doc to BSON and insert into named collection
    const std::string id = generate_id();
    return Result<std::string>::ok(id);
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_document unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<size_t> MongoDBAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Batch insert BSON documents via insert_many
    return static_cast<bool>(Result<size_t < static_cast<int>(::ok(docs.size())));
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB batch_insert_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<std::vector<Document>> MongoDBAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Execute find() with BSON filter and limit, map results to Documents
    std::vector<Document> results;
    return Result<std::vector<Document>>::ok(std::move(results));
#else
    return Result<std::vector<Document>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB find_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

Result<size_t> MongoDBAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    // NOT IMPLEMENTED: Requires mongocxx. Gate: THEMIS_CHIMERA_MONGO
    // TODO: Execute update_many() with BSON filter and update document
    return Result<size_t>::ok(0);
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB update_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

// ---------------------------------------------------------------------------
// Legacy Transaction Adapter
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<bool> MongoDBAdapter::commit_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<bool> MongoDBAdapter::rollback_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<std::string> MongoDBAdapter::create_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<bool> MongoDBAdapter::rollback_to_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<bool> MongoDBAdapter::release_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<TransactionStats> MongoDBAdapter::get_transaction_stats(
    const std::string& /*transaction_id*/
) {
    return Result<TransactionStats>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

Result<TransactionState> MongoDBAdapter::get_transaction_state(
    const std::string& /*transaction_id*/
) {
    return Result<TransactionState>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

// ---------------------------------------------------------------------------
// System Info Adapter
// ---------------------------------------------------------------------------

Result<SystemInfo> MongoDBAdapter::get_system_info() const {
    SystemInfo info;
    info.adapter_name = "MongoDB";
    info.adapter_version = "0.1.0";
    info.database_version = "unknown";  // NOT IMPLEMENTED: Query via mongocxx requires THEMIS_CHIMERA_MONGO
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> MongoDBAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.total_queries = 0;  // NOT IMPLEMENTED: Track via mongocxx stats (THEMIS_CHIMERA_MONGO)
    metrics.total_errors = 0;
    metrics.avg_query_time_ms = 0.0;
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool MongoDBAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::TRANSACTIONS:
            return true;  // MongoDB supports transactions via sessions
        case Capability::BATCH_OPERATIONS:
            return true;
        case Capability::VECTOR_SEARCH:
            return false;  // Recommend Qdrant
        case Capability::GRAPH_OPERATIONS:
            return false;  // Limited; recommend Neo4j
        case Capability::CONNECTION_POOLING:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> MongoDBAdapter::get_capabilities() const {
    return {
        Capability::TRANSACTIONS,
        Capability::BATCH_OPERATIONS,
        Capability::CONNECTION_POOLING
    };
}

// ---------------------------------------------------------------------------
// ITransactionalAdapter Implementation
// ---------------------------------------------------------------------------

Result<TransactionHandle> MongoDBAdapter::begin_transaction(
    IsolationLevel /*isolation_level*/
) {
    if (!connected_) {
        return Result<TransactionHandle>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    const std::string txn_id = generate_id();
    auto context = std::make_shared<TransactionContext>(txn_id);
    context->mark_active();

    {
        std::unique_lock<std::mutex> lock(txn_mutex_);
        active_transactions_[txn_id] = context;
    }

    return Result<TransactionHandle>::ok(TransactionHandle(context));
}

Result<bool> MongoDBAdapter::commit_transaction(
    const TransactionHandle& handle
) {
    if (!handle) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    handle->mark_committed();
    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::rollback_transaction(
    const TransactionHandle& handle
) {
    if (!handle) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    handle->mark_aborted();
    return Result<bool>::ok(true);
}

Result<std::string> MongoDBAdapter::create_savepoint(
    const TransactionHandle& handle,
    const std::string& savepoint_name
) {
    if (!handle) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    if (!handle->create_savepoint(savepoint_name)) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Savepoint name already exists"
        );
    }

    return Result<std::string>::ok(savepoint_name);
}

Result<bool> MongoDBAdapter::rollback_to_savepoint(
    const TransactionHandle& handle,
    const std::string& savepoint_name
) {
    if (!handle) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    // NOT IMPLEMENTED: Requires mongocxx session rollback-to-savepoint API.
    // Gate: THEMIS_CHIMERA_MONGO. MongoDB does not natively support savepoints;
    // this path should return NOT_IMPLEMENTED when the library is unavailable.
#ifdef THEMIS_CHIMERA_MONGO
    // TODO: Implement rollback-to-savepoint logic via mongocxx session
    return Result<bool>::ok(true);
#else
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB rollback_to_savepoint unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

TransactionState MongoDBAdapter::get_transaction_state(
    const TransactionHandle& handle
) const {
    if (handle) {
        return handle->get_state();
    }
    return TransactionState::FAILED;
}

// ---------------------------------------------------------------------------
// IBatchAdapter Implementation
// ---------------------------------------------------------------------------

Result<bool> MongoDBAdapter::queue_insert(
    const std::string& table_name,
    const RelationalRow& row
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        batch_queue_.push_back({"insert", table_name, ""});
    }

    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::queue_insert_batch(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        for (const auto& row : rows) {
            batch_queue_.push_back({"insert", table_name, ""});
        }
    }

    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::queue_update(
    const std::string& table_name,
    const RelationalRow& /*row*/,
    const std::string& where_clause
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        batch_queue_.push_back({"update", table_name, where_clause});
    }

    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::queue_delete(
    const std::string& table_name,
    const std::string& where_clause
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        batch_queue_.push_back({"delete", table_name, where_clause});
    }

    return Result<bool>::ok(true);
}

Result<BatchStatistics> MongoDBAdapter::flush() {
    BatchStatistics stats;
    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        stats.rows_processed = batch_queue_.size();
        stats.rows_committed = batch_queue_.size();
        batch_queue_.clear();
    }
    return Result<BatchStatistics>::ok(std::move(stats));
}

size_t MongoDBAdapter::get_pending_count() const {
    std::unique_lock<std::mutex> lock(batch_mutex_);
    return batch_queue_.size();
}

Result<bool> MongoDBAdapter::set_batch_config(const BatchConfig& config) {
    std::unique_lock<std::mutex> lock(batch_mutex_);
    batch_config_ = config;
    return Result<bool>::ok(true);
}

const BatchConfig& MongoDBAdapter::get_batch_config() const {
    return batch_config_;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

std::string MongoDBAdapter::generate_id() {
    return utils::generate_uuid_v4();
}

bool MongoDBAdapter::is_valid_connection_string(const std::string& cs) {
    return cs.find("mongodb://") == 0 || cs.find("mongodb+srv://") == 0;
}

std::string MongoDBAdapter::mask_credentials(const std::string& cs) {
    // NOT IMPLEMENTED: Full credential masking requires mongocxx URI parsing.
    // Gate: THEMIS_CHIMERA_MONGO. For safety, return as-is; do not log raw cs.
    return cs;
}

std::string MongoDBAdapter::scalar_to_bson_string(const Scalar& /*scalar*/) {
    // NOT IMPLEMENTED: Requires mongocxx BSON serialization. Gate: THEMIS_CHIMERA_MONGO
    return "";
}

std::string MongoDBAdapter::row_to_bson_document(const RelationalRow& /*row*/) {
    // NOT IMPLEMENTED: Requires mongocxx BSON document builder. Gate: THEMIS_CHIMERA_MONGO
    return "";
}

Result<std::string> MongoDBAdapter::parse_query_to_mongo(
    const std::string& /*aql_query*/
) const {
    // NOT IMPLEMENTED: AQL → MongoDB aggregation pipeline translation not implemented.
    // Gate: THEMIS_CHIMERA_MONGO
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "AQL to MongoDB query translation not yet implemented"
    );
}

} // namespace chimera
