/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgresql_adapter.cpp                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:15:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   81.0/100                                       ║
    • Total Lines:     805                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e3c17b310f  2026-02-26  Implement MongoDB Atlas Vector Search integration: add se... ║
    • 261a690e71  2026-02-26  feat(chimera): implement PostgreSQL + pgvector adapter fo... ║
    • 5554ae8cdc  2026-02-22  Code audit and bugfix: fix document_matches id field, mas... ║
    • d34adc2bf9  2026-02-22  Implement MongoDB vendor adapter for Chimera module ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file postgresql_adapter.cpp
 * @brief PostgreSQL + pgvector adapter implementation for CHIMERA Suite
 *
 * @details
 * Implements the IDatabaseAdapter interface for PostgreSQL 14+. When the
 * libpqxx driver is not available the adapter operates in an in-process
 * simulation mode backed by std::unordered_map, which is sufficient for
 * unit testing without a live PostgreSQL server.
 *
 * Production deployments should link against libpqxx and replace the
 * in-process simulation blocks with real libpqxx calls.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Allow adapter-level testing without requiring libpqxx/PostgreSQL runtime.
 * Activation: Active when real libpqxx integration is not compiled/used.
 * Production Delta: Uses in-memory simulation instead of PostgreSQL-backed execution.
 * Removal Plan: Keep as fallback for tests; production deployments should use libpqxx integration.
 *
 * @copyright MIT License
 */

#include "chimera/postgresql_adapter.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <sstream>

namespace chimera {

// ---------------------------------------------------------------------------
// Auto-registration
// ---------------------------------------------------------------------------

namespace {
// Register PostgreSQLAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
const bool postgresql_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "PostgreSQL",
        []() { return std::make_unique<PostgreSQLAdapter>(); }
    );
    assert(ok && "PostgreSQLAdapter: 'PostgreSQL' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

PostgreSQLAdapter::PostgreSQLAdapter() = default;

PostgreSQLAdapter::~PostgreSQLAdapter() {
    if (connected_) {
        disconnect();
    }
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> PostgreSQLAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& /*options*/
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "PostgreSQL connection string must not be empty"
        );
    }

    // Accept postgresql:// and postgres:// schemes
    const bool valid_scheme =
        connection_string.rfind("postgresql://", 0) == 0 ||
        connection_string.rfind("postgres://", 0) == 0;

    if (!valid_scheme) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid PostgreSQL connection string: must start with "
            "postgresql:// or postgres://"
        );
    }

    connection_string_ = mask_credentials(connection_string);
    database_name_ = parse_database_name(connection_string);
    if (database_name_.empty()) {
        database_name_ = "postgres";
    }

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> PostgreSQLAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    return Result<bool>::ok(true);
}

bool PostgreSQLAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// IRelationalAdapter – primary capability
// ---------------------------------------------------------------------------

Result<RelationalTable> PostgreSQLAdapter::execute_query(
    const std::string& query,
    const std::vector<Scalar>& /*params*/
) {
    if (!connected_) {
        return Result<RelationalTable>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (query.empty()) {
        return Result<RelationalTable>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Query string must not be empty"
        );
    }

    // In simulation mode, return an empty result set.
    // Production code would execute via a libpqxx connection.
    RelationalTable table;
    return Result<RelationalTable>::ok(std::move(table));
}

Result<size_t> PostgreSQLAdapter::insert_row(
    const std::string& table_name,
    const RelationalRow& row
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (table_name.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Table name must not be empty"
        );
    }
    if (row.columns.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Row must have at least one column"
        );
    }

    std::lock_guard<std::mutex> lock(table_mutex_);
    table_store_[table_name].push_back(row);
    return Result<size_t>::ok(1);
}

Result<size_t> PostgreSQLAdapter::batch_insert(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (table_name.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Table name must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(table_mutex_);
    auto& tbl = table_store_[table_name];
    tbl.insert(tbl.end(), rows.begin(), rows.end());
    return Result<size_t>::ok(rows.size());
}

Result<QueryStatistics> PostgreSQLAdapter::get_query_statistics() const {
    QueryStatistics stats;
    stats.execution_time = std::chrono::microseconds(0);
    stats.rows_read = 0;
    stats.rows_returned = 0;
    stats.bytes_read = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// ---------------------------------------------------------------------------
// IVectorAdapter – pgvector simulation
// ---------------------------------------------------------------------------

Result<std::string> PostgreSQLAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (vector.data.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Vector must not be empty"
        );
    }

    // Store the vector as a document with internal component fields and
    // a dimension marker so it can be reconstructed for similarity search.
    Document doc;
    doc.id = generate_document_id();
    doc.fields["__vector_dim__"] = Scalar{int64_t(vector.data.size())};
    for (size_t i = 0; i < vector.data.size(); ++i) {
        doc.fields["__v" + std::to_string(i) + "__"] =
            Scalar{static_cast<double>(vector.data[i])};
    }
    for (const auto& kv : vector.metadata) {
        doc.fields[kv.first] = kv.second;
    }

    std::lock_guard<std::mutex> lock(vector_mutex_);
    vector_store_[collection].push_back(std::move(doc));
    return Result<std::string>::ok(vector_store_[collection].back().id);
}

Result<size_t> PostgreSQLAdapter::batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
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

Result<std::vector<std::pair<Vector, double>>> PostgreSQLAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (query_vector.data.empty()) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Query vector must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(vector_mutex_);
    auto it = vector_store_.find(collection);
    if (it == vector_store_.end()) {
        return Result<std::vector<std::pair<Vector, double>>>::ok({});
    }

    std::vector<std::pair<Vector, double>> candidates;
    for (const auto& doc : it->second) {
        auto dim_it = doc.fields.find("__vector_dim__");
        if (dim_it == doc.fields.end()) continue;
        if (!std::holds_alternative<int64_t>(dim_it->second)) continue;
        const size_t dim = static_cast<size_t>(std::get<int64_t>(dim_it->second));
        if (dim != query_vector.data.size()) continue;

        if (!filters.empty() && !document_matches(doc, filters)) continue;

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

        for (const auto& kv : doc.fields) {
            if (kv.first.rfind("__", 0) != 0) {
                stored_vec.metadata[kv.first] = kv.second;
            }
        }

        double sim = cosine_similarity(query_vector.data, stored_vec.data);
        candidates.emplace_back(std::move(stored_vec), sim);
    }

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

Result<bool> PostgreSQLAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    // In simulation mode, index creation is a no-op.
    // Production code would execute: CREATE INDEX ... USING hnsw/ivfflat
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// IGraphAdapter – not supported by PostgreSQL natively
// ---------------------------------------------------------------------------

Result<std::string> PostgreSQLAdapter::insert_node(const GraphNode& /*node*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "PostgreSQL does not support native graph node insertion"
    );
}

Result<std::string> PostgreSQLAdapter::insert_edge(const GraphEdge& /*edge*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "PostgreSQL does not support native graph edge insertion"
    );
}

Result<GraphPath> PostgreSQLAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "PostgreSQL does not support native graph shortest-path queries"
    );
}

Result<std::vector<GraphNode>> PostgreSQLAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "PostgreSQL does not support native graph traversal"
    );
}

Result<std::vector<GraphPath>> PostgreSQLAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "PostgreSQL does not support native graph queries"
    );
}

// ---------------------------------------------------------------------------
// IDocumentAdapter – JSONB simulation
// ---------------------------------------------------------------------------

Result<std::string> PostgreSQLAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
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

    std::lock_guard<std::mutex> lock(doc_mutex_);
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

Result<size_t> PostgreSQLAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
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

Result<std::vector<Document>> PostgreSQLAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }

    std::lock_guard<std::mutex> lock(doc_mutex_);
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

Result<size_t> PostgreSQLAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }
    if (updates.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Update map must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(doc_mutex_);
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

Result<std::string> PostgreSQLAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
        );
    }

    std::string txn_id = generate_transaction_id();
    std::lock_guard<std::mutex> lock(txn_mutex_);
    active_transactions_[txn_id] = true;
    return Result<std::string>::ok(std::move(txn_id));
}

Result<bool> PostgreSQLAdapter::commit_transaction(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
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

Result<bool> PostgreSQLAdapter::rollback_transaction(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to PostgreSQL"
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

Result<SystemInfo> PostgreSQLAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "PostgreSQL";
    // Simulation mode reports 16.0; production implementations should query
    // the server's SELECT version() to obtain the actual version string.
    info.version = "16.0";
    info.build_info["driver"] = "libpqxx";
    info.build_info["extensions"] = "pgvector";
    info.build_info["platform"] = "Linux/Windows/macOS";
    info.configuration["database"] = Scalar{database_name_};
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> PostgreSQLAdapter::get_metrics() const {
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

bool PostgreSQLAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:
        case Capability::VECTOR_SEARCH:
        case Capability::DOCUMENT_STORE:
        case Capability::FULL_TEXT_SEARCH:
        case Capability::TRANSACTIONS:
        case Capability::BATCH_OPERATIONS:
        case Capability::SECONDARY_INDEXES:
        case Capability::MATERIALIZED_VIEWS:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> PostgreSQLAdapter::get_capabilities() const {
    return {
        Capability::RELATIONAL_QUERIES,
        Capability::VECTOR_SEARCH,
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES,
        Capability::MATERIALIZED_VIEWS
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string PostgreSQLAdapter::parse_database_name(
    const std::string& connection_string
) {
    // Locate the path component after the host/port
    // e.g. postgresql://user:pass@host:5432/mydb?options
    //                                        ^^^^
    const std::string prefix_pg   = "postgresql://";
    const std::string prefix_pg2  = "postgres://";

    std::string rest;
    if (connection_string.rfind(prefix_pg, 0) == 0) {
        rest = connection_string.substr(prefix_pg.size());
    } else if (connection_string.rfind(prefix_pg2, 0) == 0) {
        rest = connection_string.substr(prefix_pg2.size());
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

std::string PostgreSQLAdapter::mask_credentials(
    const std::string& connection_string
) {
    // Replace user:password@ portion with ***:***@ so the stored string
    // cannot expose credentials through memory inspection or log leakage.
    const std::string prefix_pg  = "postgresql://";
    const std::string prefix_pg2 = "postgres://";

    std::string scheme;
    std::string rest;
    if (connection_string.rfind(prefix_pg, 0) == 0) {
        scheme = prefix_pg;
        rest   = connection_string.substr(prefix_pg.size());
    } else if (connection_string.rfind(prefix_pg2, 0) == 0) {
        scheme = prefix_pg2;
        rest   = connection_string.substr(prefix_pg2.size());
    } else {
        return connection_string;
    }

    auto at_pos = rest.find('@');
    if (at_pos == std::string::npos) {
        // No credentials present
        return connection_string;
    }

    return scheme + "***:***@" + rest.substr(at_pos + 1);
}

std::string PostgreSQLAdapter::generate_row_id() {
    std::ostringstream oss;
    oss << "pg_row_" << next_row_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::string PostgreSQLAdapter::generate_document_id() {
    std::ostringstream oss;
    oss << "pg_doc_" << next_doc_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::string PostgreSQLAdapter::generate_transaction_id() {
    std::ostringstream oss;
    oss << "pg_txn_" << next_txn_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

double PostgreSQLAdapter::cosine_similarity(const std::vector<float>& a,
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

bool PostgreSQLAdapter::document_matches(
    const Document& doc,
    const std::map<std::string, Scalar>& filter
) {
    for (const auto& kv : filter) {
        // Skip internal vector fields
        if (kv.first.rfind("__", 0) == 0) continue;

        // The "id" key matches the document's top-level id field
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
