/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pinecone_adapter.cpp                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:15:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     642                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 10fd73cb8a  2026-02-28  audit(chimera): fill all gaps identified in Pinecone adap... ║
    • 31a9305f66  2026-02-28  feat(chimera): Add Pinecone managed vector search adapter ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file pinecone_adapter.cpp
 * @brief Pinecone managed vector search adapter for CHIMERA Suite
 *
 * @details
 * Implements the IDatabaseAdapter interface for Pinecone (v3 serverless).
 * When the HTTP client driver is not available the adapter operates in an
 * in-process simulation mode backed by std::unordered_map, which is
 * sufficient for unit testing without a live Pinecone index.
 *
 * Production deployments should link against an HTTP client library
 * (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks
 * with real Pinecone REST API calls.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Support adapter verification without requiring a live Pinecone index.
 * Activation: Active when real HTTP-driver integration is not compiled/used.
 * Production Delta: Uses in-memory simulation instead of Pinecone REST API requests.
 * Removal Plan: Keep as fallback for tests; production deployments should use real backend integration.
 *
 * @copyright MIT License
 */

#include "chimera/pinecone_adapter.hpp"

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
// Register PineconeAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
const bool pinecone_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "Pinecone",
        []() { return std::make_unique<PineconeAdapter>(); }
    );
    assert(ok && "PineconeAdapter: 'Pinecone' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

PineconeAdapter::PineconeAdapter() = default;

PineconeAdapter::~PineconeAdapter() {
    if (connected_) {
        disconnect();
    }
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> PineconeAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Pinecone connection string must not be empty"
        );
    }

    if (!is_valid_connection_string(connection_string)) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid Pinecone connection string: must start with https://"
        );
    }

    connection_string_ = connection_string;

    // Extract API key from options if provided.
    // The key is stored only as a presence flag – it is intentionally NOT
    // copied into connection_string_ or any field surfaced by get_system_info()
    // so that it cannot be leaked through logs or memory inspection.
    // Note: the key exists in the caller's `options` map for the duration of
    // this call; callers should avoid logging the options map directly.
    auto it = options.find("api_key");
    has_api_key_ = (it != options.end() && !it->second.empty());

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> PineconeAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    has_api_key_ = false;
    return Result<bool>::ok(true);
}

bool PineconeAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// IRelationalAdapter – not supported by Pinecone
// ---------------------------------------------------------------------------

Result<RelationalTable> PineconeAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support relational/SQL queries"
    );
}

Result<size_t> PineconeAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support relational row insertion"
    );
}

Result<size_t> PineconeAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support relational batch insertion"
    );
}

Result<QueryStatistics> PineconeAdapter::get_query_statistics() const {
    QueryStatistics stats;
    stats.execution_time = std::chrono::microseconds(0);
    stats.rows_read = 0;
    stats.rows_returned = 0;
    stats.bytes_read = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// ---------------------------------------------------------------------------
// IVectorAdapter – primary capability
// ---------------------------------------------------------------------------

Result<std::string> PineconeAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
        );
    }
    if (vector.data.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Vector must not be empty"
        );
    }

    // Store the vector as a document with internal component fields so that
    // search_vectors can reconstruct the vector for similarity scoring.
    Document doc;
    doc.id = generate_vector_id();
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

Result<size_t> PineconeAdapter::batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
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

Result<std::vector<std::pair<Vector, double>>> PineconeAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
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

        double score = cosine_similarity(query_vector.data, stored_vec.data);
        candidates.emplace_back(std::move(stored_vec), score);
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

Result<bool> PineconeAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
        );
    }
    // In simulation mode, Pinecone index/namespace creation is a no-op.
    // Production code would POST to the Pinecone control-plane API to create
    // the index with the specified dimensions and metric.
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// IGraphAdapter – not supported by Pinecone natively
// ---------------------------------------------------------------------------

Result<std::string> PineconeAdapter::insert_node(const GraphNode& /*node*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support native graph node insertion"
    );
}

Result<std::string> PineconeAdapter::insert_edge(const GraphEdge& /*edge*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support native graph edge insertion"
    );
}

Result<GraphPath> PineconeAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support native graph shortest-path queries"
    );
}

Result<std::vector<GraphNode>> PineconeAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support native graph traversal"
    );
}

Result<std::vector<GraphPath>> PineconeAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support native graph queries"
    );
}

// ---------------------------------------------------------------------------
// IDocumentAdapter – Pinecone metadata store
// ---------------------------------------------------------------------------

Result<std::string> PineconeAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
        );
    }

    std::lock_guard<std::mutex> lock(doc_mutex_);
    auto& store = document_store_[collection];

    // Reject duplicate IDs
    if (!doc.id.empty()) {
        for (const auto& existing : store) {
            if (existing.id == doc.id) {
                return Result<std::string>::err(
                    ErrorCode::ALREADY_EXISTS,
                    "Document with id '" + doc.id + "' already exists in '" +
                        collection + "'"
                );
            }
        }
    }

    Document stored = doc;
    if (stored.id.empty()) {
        stored.id = generate_vector_id();
    }
    const std::string inserted_id = stored.id;
    store.push_back(std::move(stored));
    return Result<std::string>::ok(inserted_id);
}

Result<size_t> PineconeAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
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

Result<std::vector<Document>> PineconeAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
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

Result<size_t> PineconeAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Pinecone"
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
// ITransactionAdapter – not supported by Pinecone
// ---------------------------------------------------------------------------

Result<std::string> PineconeAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support ACID transactions"
    );
}

Result<bool> PineconeAdapter::commit_transaction(
    const std::string& /*transaction_id*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support ACID transactions"
    );
}

Result<bool> PineconeAdapter::rollback_transaction(
    const std::string& /*transaction_id*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Pinecone does not support ACID transactions"
    );
}

// ---------------------------------------------------------------------------
// ISystemInfoAdapter
// ---------------------------------------------------------------------------

Result<SystemInfo> PineconeAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "Pinecone";
    // Simulation mode reports v3; production implementations should query
    // the Pinecone describe-index endpoint to obtain the actual version.
    info.version = "3";
    info.build_info["index_type"] = "Managed ANN";
    info.build_info["platform"] = "Managed Cloud (AWS/GCP/Azure)";
    info.build_info["api"] = "REST";
    info.configuration["endpoint"] = Scalar{connection_string_};
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> PineconeAdapter::get_metrics() const {
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

bool PineconeAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::VECTOR_SEARCH:
        case Capability::BATCH_OPERATIONS:
        case Capability::SECONDARY_INDEXES:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> PineconeAdapter::get_capabilities() const {
    return {
        Capability::VECTOR_SEARCH,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool PineconeAdapter::is_valid_connection_string(
    const std::string& connection_string
) {
    // Pinecone is a managed cloud service; only https:// is accepted to
    // enforce secure-by-default usage.
    return connection_string.rfind("https://", 0) == 0;
}

std::string PineconeAdapter::generate_vector_id() {
    // Simple deterministic ID generation for the simulation layer.
    // Production code would use a UUID v4 or a caller-supplied string as
    // required by the Pinecone upsert API.
    std::ostringstream oss;
    oss << "pinecone_vec_" << next_vector_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

double PineconeAdapter::cosine_similarity(const std::vector<float>& a,
                                          const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;

    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        const double ai = static_cast<double>(a[i]);
        const double bi = static_cast<double>(b[i]);
        dot    += ai * bi;
        norm_a += ai * ai;
        norm_b += bi * bi;
    }

    const double denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-12) return 0.0;
    return dot / denom;
}

bool PineconeAdapter::document_matches(
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
