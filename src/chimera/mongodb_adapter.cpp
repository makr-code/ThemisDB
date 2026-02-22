/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mongodb_adapter.cpp                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-22 08:56:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   77.0/100                                       ║
    • Total Lines:     743                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5554ae8cd  2026-02-22  Code audit and bugfix: fix document_matches id field, mas... ║
    • d34adc2bf  2026-02-22  Implement MongoDB vendor adapter for Chimera module ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file mongodb_adapter.cpp
 * @brief MongoDB adapter implementation for CHIMERA Suite
 *
 * @details
 * Implements the IDatabaseAdapter interface for MongoDB 4.4+. When the
 * mongocxx driver is not available the adapter operates in an in-process
 * simulation mode backed by std::unordered_map, which is sufficient for
 * unit testing without a live MongoDB server.
 *
 * Production deployments should link against libmongocxx and replace the
 * in-process simulation blocks (guarded by the TODO markers) with real
 * mongocxx calls.
 *
 * @copyright MIT License
 */

#include "chimera/mongodb_adapter.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>

namespace chimera {

// ---------------------------------------------------------------------------
// Construction / Destruction
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
    const std::map<std::string, std::string>& options
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "MongoDB connection string must not be empty"
        );
    }

    // Accept mongodb:// and mongodb+srv:// schemes
    const bool valid_scheme =
        connection_string.rfind("mongodb://", 0) == 0 ||
        connection_string.rfind("mongodb+srv://", 0) == 0;

    if (!valid_scheme) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid MongoDB connection string: must start with "
            "mongodb:// or mongodb+srv://"
        );
    }

    connection_string_ = mask_credentials(connection_string);
    database_name_ = parse_database_name(connection_string);
    if (database_name_.empty()) {
        database_name_ = "test";
    }

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    return Result<bool>::ok(true);
}

bool MongoDBAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// IRelationalAdapter – not supported by MongoDB
// ---------------------------------------------------------------------------

Result<RelationalTable> MongoDBAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support relational/SQL queries"
    );
}

Result<size_t> MongoDBAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support relational row insertion"
    );
}

Result<size_t> MongoDBAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support relational batch insertion"
    );
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
// IVectorAdapter – Atlas Vector Search simulation
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }
    if (vector.data.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Vector must not be empty"
        );
    }

    // Store vector as a document with a special "__vector__" field containing
    // serialized float data encoded in the metadata.
    Document doc;
    doc.id = generate_document_id();
    // Store dimension count
    doc.fields["__vector_dim__"] = Scalar{int64_t(vector.data.size())};
    // Embed each component as a named scalar field for retrieval
    for (size_t i = 0; i < vector.data.size(); ++i) {
        doc.fields["__v" + std::to_string(i) + "__"] =
            Scalar{static_cast<double>(vector.data[i])};
    }
    // Copy metadata
    for (const auto& kv : vector.metadata) {
        doc.fields[kv.first] = kv.second;
    }

    std::lock_guard<std::mutex> lock(store_mutex_);
    document_store_[collection].push_back(std::move(doc));
    return Result<std::string>::ok(document_store_[collection].back().id);
}

Result<size_t> MongoDBAdapter::batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    size_t inserted = 0;
    for (const auto& vec : vectors) {
        auto result = insert_vector(collection, vec);
        if (result.is_ok()) {
            ++inserted;
        }
    }
    return Result<size_t>::ok(inserted);
}

Result<std::vector<std::pair<Vector, double>>> MongoDBAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }
    if (query_vector.data.empty()) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Query vector must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = document_store_.find(collection);
    if (it == document_store_.end()) {
        // Empty collection – return empty result set
        return Result<std::vector<std::pair<Vector, double>>>::ok({});
    }

    // Reconstruct vectors from stored documents and compute similarity
    std::vector<std::pair<Vector, double>> candidates;
    for (const auto& doc : it->second) {
        // Check dimension marker
        auto dim_it = doc.fields.find("__vector_dim__");
        if (dim_it == doc.fields.end()) continue;
        if (!std::holds_alternative<int64_t>(dim_it->second)) continue;
        const size_t dim = static_cast<size_t>(std::get<int64_t>(dim_it->second));
        if (dim != query_vector.data.size()) continue;

        // Apply metadata filters (skip __vector__ internal fields)
        if (!filters.empty() && !document_matches(doc, filters)) continue;

        // Reconstruct float vector
        Vector stored_vec;
        stored_vec.data.resize(dim);
        bool ok = true;
        for (size_t i = 0; i < dim; ++i) {
            auto fi = doc.fields.find("__v" + std::to_string(i) + "__");
            if (fi == doc.fields.end() ||
                !std::holds_alternative<double>(fi->second)) {
                ok = false;
                break;
            }
            stored_vec.data[i] =
                static_cast<float>(std::get<double>(fi->second));
        }
        if (!ok) continue;

        // Copy non-internal metadata
        for (const auto& kv : doc.fields) {
            if (kv.first.rfind("__", 0) != 0) {
                stored_vec.metadata[kv.first] = kv.second;
            }
        }

        double sim = cosine_similarity(query_vector.data, stored_vec.data);
        candidates.emplace_back(std::move(stored_vec), sim);
    }

    // Sort descending by similarity
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    if (candidates.size() > k) {
        candidates.resize(k);
    }

    return Result<std::vector<std::pair<Vector, double>>>::ok(
        std::move(candidates));
}

Result<bool> MongoDBAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }
    // In simulation mode index creation is a no-op
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// IGraphAdapter – not supported by MongoDB
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::insert_node(const GraphNode& /*node*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support native graph node insertion"
    );
}

Result<std::string> MongoDBAdapter::insert_edge(const GraphEdge& /*edge*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support native graph edge insertion"
    );
}

Result<GraphPath> MongoDBAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support native graph shortest-path queries"
    );
}

Result<std::vector<GraphNode>> MongoDBAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support native graph traversal"
    );
}

Result<std::vector<GraphPath>> MongoDBAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB does not support native graph queries"
    );
}

// ---------------------------------------------------------------------------
// IDocumentAdapter
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
    if (collection.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Collection name must not be empty"
        );
    }

    Document stored = doc;
    if (stored.id.empty()) {
        stored.id = generate_document_id();
    }
    stored.timestamp = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(store_mutex_);
    // Reject duplicate IDs within the same collection
    auto& coll = document_store_[collection];
    for (const auto& existing : coll) {
        if (existing.id == stored.id) {
            return Result<std::string>::err(
                ErrorCode::ALREADY_EXISTS,
                "Document with id '" + stored.id + "' already exists"
            );
        }
    }
    coll.push_back(std::move(stored));
    return Result<std::string>::ok(coll.back().id);
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

    size_t inserted = 0;
    for (const auto& doc : docs) {
        auto result = insert_document(collection, doc);
        if (result.is_ok()) {
            ++inserted;
        }
    }
    return Result<size_t>::ok(inserted);
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

    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = document_store_.find(collection);
    if (it == document_store_.end()) {
        return Result<std::vector<Document>>::ok({});
    }

    std::vector<Document> results;
    for (const auto& doc : it->second) {
        if (filter.empty() || document_matches(doc, filter)) {
            results.push_back(doc);
            if (results.size() >= limit) break;
        }
    }
    return Result<std::vector<Document>>::ok(std::move(results));
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
    if (updates.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Update map must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = document_store_.find(collection);
    if (it == document_store_.end()) {
        return Result<size_t>::ok(0);
    }

    size_t updated = 0;
    for (auto& doc : it->second) {
        if (filter.empty() || document_matches(doc, filter)) {
            for (const auto& kv : updates) {
                doc.fields[kv.first] = kv.second;
            }
            ++updated;
        }
    }
    return Result<size_t>::ok(updated);
}

// ---------------------------------------------------------------------------
// ITransactionAdapter
// ---------------------------------------------------------------------------

Result<std::string> MongoDBAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    std::string txn_id = generate_transaction_id();
    std::lock_guard<std::mutex> lock(txn_mutex_);
    active_transactions_[txn_id] = true;
    return Result<std::string>::ok(std::move(txn_id));
}

Result<bool> MongoDBAdapter::commit_transaction(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found"
        );
    }
    if (!it->second) {
        return Result<bool>::err(
            ErrorCode::TRANSACTION_ABORTED,
            "Transaction '" + transaction_id + "' is already closed"
        );
    }
    it->second = false;
    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::rollback_transaction(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found"
        );
    }
    if (!it->second) {
        return Result<bool>::err(
            ErrorCode::TRANSACTION_ABORTED,
            "Transaction '" + transaction_id + "' is already closed"
        );
    }
    it->second = false;
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// ISystemInfoAdapter
// ---------------------------------------------------------------------------

Result<SystemInfo> MongoDBAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "MongoDB";
    // Simulation mode reports 7.0; production implementations should query
    // the server's buildInfo command to obtain the actual version string.
    info.version = "7.0";
    info.build_info["driver"] = "mongocxx";
    info.build_info["platform"] = "Linux/Windows/macOS";
    info.configuration["database"] = Scalar{database_name_};
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> MongoDBAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.memory.total_bytes = 0;
    metrics.memory.used_bytes = 0;
    metrics.memory.available_bytes = 0;
    metrics.storage.total_bytes = 0;
    metrics.storage.used_bytes = 0;
    metrics.storage.available_bytes = 0;
    metrics.cpu.utilization_percent = 0.0;
    metrics.cpu.thread_count = 0;
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool MongoDBAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::DOCUMENT_STORE:
        case Capability::FULL_TEXT_SEARCH:
        case Capability::TRANSACTIONS:
        case Capability::BATCH_OPERATIONS:
        case Capability::SECONDARY_INDEXES:
        case Capability::VECTOR_SEARCH:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> MongoDBAdapter::get_capabilities() const {
    return {
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES,
        Capability::VECTOR_SEARCH
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string MongoDBAdapter::parse_database_name(
    const std::string& connection_string
) {
    // Locate the path component after the host/port
    // e.g. mongodb://user:pass@host:27017/mydb?options
    //                                    ^^^^^
    const std::string prefix_plain = "mongodb://";
    const std::string prefix_srv   = "mongodb+srv://";

    std::string rest;
    if (connection_string.rfind(prefix_srv, 0) == 0) {
        rest = connection_string.substr(prefix_srv.size());
    } else if (connection_string.rfind(prefix_plain, 0) == 0) {
        rest = connection_string.substr(prefix_plain.size());
    } else {
        return {};
    }

    // Skip optional user:pass@ section
    auto at_pos = rest.find('@');
    if (at_pos != std::string::npos) {
        rest = rest.substr(at_pos + 1);
    }

    // Find the start of the path (first '/')
    auto slash_pos = rest.find('/');
    if (slash_pos == std::string::npos) {
        return {};
    }
    rest = rest.substr(slash_pos + 1);

    // Strip query string
    auto q_pos = rest.find('?');
    if (q_pos != std::string::npos) {
        rest = rest.substr(0, q_pos);
    }

    return rest;
}

std::string MongoDBAdapter::mask_credentials(
    const std::string& connection_string
) {
    // Replace user:password@ portion with ***:***@ so the stored string
    // cannot expose credentials through memory inspection or log leakage.
    const std::string prefix_plain = "mongodb://";
    const std::string prefix_srv   = "mongodb+srv://";

    std::string scheme;
    std::string rest;
    if (connection_string.rfind(prefix_srv, 0) == 0) {
        scheme = prefix_srv;
        rest   = connection_string.substr(prefix_srv.size());
    } else if (connection_string.rfind(prefix_plain, 0) == 0) {
        scheme = prefix_plain;
        rest   = connection_string.substr(prefix_plain.size());
    } else {
        // Unknown scheme – return as-is; caller already validated
        return connection_string;
    }

    auto at_pos = rest.find('@');
    if (at_pos == std::string::npos) {
        // No credentials present
        return connection_string;
    }

    // Keep everything after the '@'
    return scheme + "***:***@" + rest.substr(at_pos + 1);
}

std::string MongoDBAdapter::generate_document_id() {
    // Simple deterministic ID generation for the simulation layer.
    // Production code would use ObjectId from the mongocxx driver.
    std::ostringstream oss;
    oss << "mongo_doc_" << next_doc_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::string MongoDBAdapter::generate_transaction_id() {
    std::ostringstream oss;
    oss << "mongo_txn_" << next_txn_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

double MongoDBAdapter::cosine_similarity(const std::vector<float>& a,
                                         const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;

    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot    += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }
    const double denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-12) return 0.0;
    return dot / denom;
}

bool MongoDBAdapter::document_matches(
    const Document& doc,
    const std::map<std::string, Scalar>& filter
) {
    for (const auto& kv : filter) {
        // Skip internal vector fields
        if (kv.first.rfind("__", 0) == 0) continue;

        // The "id" key matches the document's top-level id field, not fields map
        if (kv.first == "id") {
            if (!std::holds_alternative<std::string>(kv.second)) return false;
            if (doc.id != std::get<std::string>(kv.second)) return false;
            continue;
        }

        auto it = doc.fields.find(kv.first);
        if (it == doc.fields.end()) return false;
        if (it->second != kv.second) return false;
    }
    return true;
}

} // namespace chimera
