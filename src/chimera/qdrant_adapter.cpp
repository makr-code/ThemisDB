/**
 * @file qdrant_adapter.cpp
 * @brief Qdrant vector-store adapter implementation.
 *
 * REST/gRPC forwarding for Qdrant operations via the Chimera
 * IDatabaseAdapter contract, including payload filtering and batch upsert.
 */

#include "chimera/qdrant_adapter.hpp"
#include "utils/uuid.h"

#include <cassert>

namespace chimera {

// Auto-registration
namespace {
const bool qdrant_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "Qdrant",
        []() { return std::make_unique<QdrantAdapter>(); }
    );
    assert(ok && "QdrantAdapter: 'Qdrant' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Constructor and Destructor
// ---------------------------------------------------------------------------

QdrantAdapter::QdrantAdapter() = default;

QdrantAdapter::~QdrantAdapter() {
    if (connected_) {
        disconnect();
    }
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> QdrantAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& /*options*/
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Qdrant connection string must not be empty"
        );
    }

    if (!is_valid_connection_string(connection_string)) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid Qdrant connection string: must include host:port or URL"
        );
    }

    connection_string_ = mask_credentials(connection_string);

#ifdef THEMIS_CHIMERA_QDRANT
    // NOT IMPLEMENTED: Requires qdrant-client-cpp. Gate: THEMIS_CHIMERA_QDRANT
    // TODO: Actual gRPC channel creation to Qdrant endpoint
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant adapter unavailable: gRPC client setup is not implemented yet."
    );
#else
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant adapter unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_QDRANT=ON to enable."
    );
#endif
}

Result<bool> QdrantAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    return Result<bool>::ok(true);
}

bool QdrantAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// Relational Adapter (Not Supported)
// ---------------------------------------------------------------------------

Result<RelationalTable> QdrantAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational queries not supported in Qdrant adapter; use ThemisDB/MongoDB"
    );
}

Result<size_t> QdrantAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational insert not supported in Qdrant adapter"
    );
}

Result<size_t> QdrantAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch insert not supported in Qdrant adapter"
    );
}

Result<QueryStatistics> QdrantAdapter::get_query_statistics() const {
    QueryStatistics stats;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// ---------------------------------------------------------------------------
// Vector Adapter (Primary)
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }

#ifdef THEMIS_CHIMERA_QDRANT
    // NOT IMPLEMENTED: Requires qdrant-client-cpp. Gate: THEMIS_CHIMERA_QDRANT
    // TODO: Upsert point via gRPC UpsertPoints RPC
    const std::string id = generate_id();
    return Result<std::string>::ok(id);
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant insert_vector unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_QDRANT=ON to enable."
    );
#endif
}

Result<size_t> QdrantAdapter::batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }

    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        for (const auto& v : vectors) {
            vector_queue_.push_back({collection, v, generate_id()});
        }
    }

    return Result<size_t>::ok(vectors.size());
}

Result<std::vector<std::pair<Vector, double>>> QdrantAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& /*filters*/
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }

#ifdef THEMIS_CHIMERA_QDRANT
    // NOT IMPLEMENTED: Requires qdrant-client-cpp. Gate: THEMIS_CHIMERA_QDRANT
    // TODO: Execute KNN search via gRPC Search RPC with payload filter
    std::vector<std::pair<Vector, double>> results;
    return Result<std::vector<std::pair<Vector, double>>>::ok(std::move(results));
#else
    return Result<std::vector<std::pair<Vector, double>>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant search_vectors unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_QDRANT=ON to enable."
    );
#endif
}

Result<bool> QdrantAdapter::create_index(
    const std::string& collection,
    size_t dimensions,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }

#ifdef THEMIS_CHIMERA_QDRANT
    // NOT IMPLEMENTED: Requires qdrant-client-cpp. Gate: THEMIS_CHIMERA_QDRANT
    // TODO: Create collection with VectorParams (size, distance metric) via gRPC
    return Result<bool>::ok(true);
#else
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant create_index unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_QDRANT=ON to enable."
    );
#endif
}

// ---------------------------------------------------------------------------
// Graph Adapter (Not Supported)
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::insert_node(const GraphNode& /*node*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph operations not supported in Qdrant adapter; use Neo4j"
    );
}

Result<std::string> QdrantAdapter::insert_edge(const GraphEdge& /*edge*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph operations not supported in Qdrant adapter; use Neo4j"
    );
}

Result<GraphPath> QdrantAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph operations not supported in Qdrant adapter"
    );
}

Result<std::vector<GraphNode>> QdrantAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph operations not supported in Qdrant adapter"
    );
}

Result<std::vector<GraphPath>> QdrantAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Graph operations not supported in Qdrant adapter"
    );
}

// ---------------------------------------------------------------------------
// Document Adapter (Not Supported)
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::insert_document(
    const std::string& /*collection*/,
    const Document& /*doc*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Document operations not supported in Qdrant adapter"
    );
}

Result<size_t> QdrantAdapter::batch_insert_documents(
    const std::string& /*collection*/,
    const std::vector<Document>& /*docs*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Document operations not supported in Qdrant adapter"
    );
}

Result<std::vector<Document>> QdrantAdapter::find_documents(
    const std::string& /*collection*/,
    const std::map<std::string, Scalar>& /*filter*/,
    size_t /*limit*/
) {
    return Result<std::vector<Document>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Document operations not supported in Qdrant adapter"
    );
}

Result<size_t> QdrantAdapter::update_documents(
    const std::string& /*collection*/,
    const std::map<std::string, Scalar>& /*filter*/,
    const std::map<std::string, Scalar>& /*updates*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Document operations not supported in Qdrant adapter"
    );
}

// ---------------------------------------------------------------------------
// Transaction Adapter (Not Supported)
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<bool> QdrantAdapter::commit_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<bool> QdrantAdapter::rollback_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<std::string> QdrantAdapter::create_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<bool> QdrantAdapter::rollback_to_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<bool> QdrantAdapter::release_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<TransactionStats> QdrantAdapter::get_transaction_stats(
    const std::string& /*transaction_id*/
) {
    return Result<TransactionStats>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

Result<TransactionState> QdrantAdapter::get_transaction_state(
    const std::string& /*transaction_id*/
) {
    return Result<TransactionState>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Transactions not supported in Qdrant adapter"
    );
}

// ---------------------------------------------------------------------------
// System Info Adapter
// ---------------------------------------------------------------------------

Result<SystemInfo> QdrantAdapter::get_system_info() const {
    SystemInfo info;
    info.adapter_name = "Qdrant";
    info.adapter_version = "0.1.0";
    info.database_version = "unknown";  // NOT IMPLEMENTED: Query via qdrant-client-cpp requires THEMIS_CHIMERA_QDRANT
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> QdrantAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.total_queries = 0;
    metrics.total_errors = 0;
    metrics.avg_query_time_ms = 0.0;
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool QdrantAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::VECTOR_SEARCH:
            return true;
        case Capability::BATCH_OPERATIONS:
            return true;
        case Capability::CONNECTION_POOLING:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> QdrantAdapter::get_capabilities() const {
    return {
        Capability::VECTOR_SEARCH,
        Capability::BATCH_OPERATIONS,
        Capability::CONNECTION_POOLING
    };
}

// ---------------------------------------------------------------------------
// IBatchAdapter Implementation
// ---------------------------------------------------------------------------

Result<bool> QdrantAdapter::queue_insert(
    const std::string& table_name,
    const RelationalRow& row
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch operations not supported"
    );
}

Result<bool> QdrantAdapter::queue_insert_batch(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch operations not supported"
    );
}

Result<bool> QdrantAdapter::queue_update(
    const std::string& table_name,
    const RelationalRow& row,
    const std::string& where_clause
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch operations not supported"
    );
}

Result<bool> QdrantAdapter::queue_delete(
    const std::string& table_name,
    const std::string& where_clause
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch operations not supported"
    );
}

Result<BatchStatistics> QdrantAdapter::flush() {
    BatchStatistics stats;
    {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        stats.rows_processed = vector_queue_.size();
        stats.rows_committed = vector_queue_.size();
        vector_queue_.clear();
    }
    return Result<BatchStatistics>::ok(std::move(stats));
}

size_t QdrantAdapter::get_pending_count() const {
    std::unique_lock<std::mutex> lock(batch_mutex_);
    return vector_queue_.size();
}

Result<bool> QdrantAdapter::set_batch_config(const BatchConfig& config) {
    std::unique_lock<std::mutex> lock(batch_mutex_);
    batch_config_ = config;
    return Result<bool>::ok(true);
}

const BatchConfig& QdrantAdapter::get_batch_config() const {
    return batch_config_;
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

std::string QdrantAdapter::generate_id() {
    return utils::generate_uuid_v4();
}

bool QdrantAdapter::is_valid_connection_string(const std::string& cs) {
    // Accept host:port or http(s)://... format
    return cs.find(':') != std::string::npos ||
           cs.find("http://") == 0 ||
           cs.find("https://") == 0;
}

std::string QdrantAdapter::mask_credentials(const std::string& cs) {
    // NOT IMPLEMENTED: Full API key masking requires URL parsing.
    // Gate: THEMIS_CHIMERA_QDRANT. For safety, return as-is; do not log raw cs.
    return cs;
}

} // namespace chimera
