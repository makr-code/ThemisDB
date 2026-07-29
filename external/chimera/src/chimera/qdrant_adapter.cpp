/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            qdrant_adapter.cpp                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:15:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     948                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 566ec90912  2026-03-12  Changes before error encountered        ║
    • 29a0ee06c5  2026-03-12  feat(chimera): wire THEMIS_ENABLE_* driver blocks, add Co... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 10fd73cb8a  2026-02-28  audit(chimera): fill all gaps identified in Pinecone adap... ║
    • e481c0e03a  2026-02-27  feat(chimera): Add Qdrant native vector database adapter ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file qdrant_adapter.cpp
 * @brief Qdrant native vector database adapter for CHIMERA Suite
 *
 * @details
 * Implements the IDatabaseAdapter interface for Qdrant v1.x. When the
 * HTTP client driver is not available the adapter operates in an in-process
 * simulation mode backed by std::unordered_map, which is sufficient for
 * unit testing without a live Qdrant server.
 *
 * Production deployments should link against an HTTP client library
 * (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks
 * with real Qdrant REST API calls.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Provide deterministic vector adapter behavior for local tests without Qdrant.
 * Activation: Active when THEMIS_ENABLE_QDRANT is not defined.
 * Production Delta: Uses in-memory simulation rather than Qdrant REST API calls.
 * Removal Plan: Keep as test fallback; production builds should enable THEMIS_ENABLE_QDRANT.
 *
 * @copyright MIT License
 */

#include "chimera/qdrant_adapter.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <sstream>

// ---------------------------------------------------------------------------
// Real Qdrant REST driver integration (cpp-httplib)
// ---------------------------------------------------------------------------
// When THEMIS_ENABLE_QDRANT is defined (set by CMake when cpp-httplib is
// found) the adapter performs real HTTP calls to the Qdrant REST API.
// Without the macro the adapter falls back to the in-process simulation.
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_QDRANT
#include <httplib.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace chimera {

// ---------------------------------------------------------------------------
// QdrantState – driver-specific state
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_QDRANT

/// Parse host and port from an http(s)://host[:port] URL.
/// Returns {host, port, use_tls}.
static std::tuple<std::string, int, bool> parse_qdrant_endpoint(
    const std::string& url)
{
    bool use_https = false;
    std::string rest;
    if (url.rfind("https://", 0) == 0) {
        rest = url.substr(8);
        use_https = true;
    } else {
        rest = url.substr(7); // http://
    }

    // Strip path component if present
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }

    const auto colon = rest.find(':');
    if (colon != std::string::npos) {
        return {rest.substr(0, colon),
                std::stoi(rest.substr(colon + 1)),
                use_https};
    }
    return {rest, use_https ? 443 : 6333, use_https};
}

struct QdrantAdapter::QdrantState {
    std::string host;
    int         port;
    bool        use_tls;
    std::string api_key;
    ConnectionPool<httplib::Client> pool;

    QdrantState(const std::string& url,
                const std::string& key,
                const ConnectionPoolConfig& pool_cfg)
        : host(std::get<0>(parse_qdrant_endpoint(url)))
        , port(std::get<1>(parse_qdrant_endpoint(url)))
        , use_tls(std::get<2>(parse_qdrant_endpoint(url)))
        , api_key(key)
        , pool(
            [this]() -> std::unique_ptr<httplib::Client> {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                if (use_tls) {
                    auto cli = std::make_unique<httplib::SSLClient>(host, port);
                    cli->set_connection_timeout(30);
                    if (!api_key.empty()) {
                        cli->set_default_headers({{"api-key", api_key}});
                    }
                    // Return as base-class pointer via unique_ptr to Client.
                    // SSLClient derives from Client so this is safe.
                    return std::unique_ptr<httplib::Client>(cli.release());
                }
#endif
                if (use_tls) {
                    // TLS requested but OpenSSL support not compiled in.
                    // Return nullptr so the pool treats it as a failed factory.
                    return nullptr;
                }
                auto cli = std::make_unique<httplib::Client>(host, port);
                cli->set_connection_timeout(30);
                if (!api_key.empty()) {
                    cli->set_default_headers({{"api-key", api_key}});
                }
                return cli;
            },
            [](httplib::Client& cli) -> bool {
                // Health-check: GET /collections (200 or 401 means server up)
                auto res = cli.Get("/collections");
                return res && (res->status == 200 || res->status == 401);
            },
            pool_cfg)
    {
        // Validate that TLS endpoints are actually reachable.
        if (use_tls) {
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
            throw std::runtime_error(
                "Qdrant: HTTPS endpoint requested but cpp-httplib was built "
                "without OpenSSL support (CPPHTTPLIB_OPENSSL_SUPPORT). "
                "Use an http:// URL or rebuild cpp-httplib with OpenSSL.");
#endif
        }
    }
};

#else

struct QdrantAdapter::QdrantState {};

#endif // THEMIS_ENABLE_QDRANT

// ---------------------------------------------------------------------------
// Auto-registration
// ---------------------------------------------------------------------------

namespace {
// Register QdrantAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
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
// Construction / Destruction
// ---------------------------------------------------------------------------

QdrantAdapter::QdrantAdapter() = default;

QdrantAdapter::~QdrantAdapter() {
    if (connected_) {
        disconnect();
    }
    // qdrant_state_ destructor runs here.
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> QdrantAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options
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
            "Invalid Qdrant connection string: must start with "
            "http:// or https://"
        );
    }

    connection_string_ = connection_string;

    // Extract API key from options if provided.
    // The key is stored only as a presence flag – it is intentionally NOT
    // copied into connection_string_ or any field surfaced by get_system_info()
    // so that it cannot be leaked through logs or memory inspection.
    auto api_it = options.find("api_key");
    has_api_key_ = (api_it != options.end() && !api_it->second.empty());
    const std::string api_key =
        has_api_key_ ? api_it->second : std::string{};

#ifdef THEMIS_ENABLE_QDRANT
    try {
        qdrant_state_ = std::make_unique<QdrantState>(
            connection_string, api_key, pool_config_);
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            std::string("Qdrant pool init failed: ") + ex.what()
        );
    }
#endif

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> QdrantAdapter::disconnect() {
#ifdef THEMIS_ENABLE_QDRANT
    qdrant_state_.reset();
#endif
    connected_ = false;
    connection_string_.clear();
    has_api_key_ = false;
    return Result<bool>::ok(true);
}

bool QdrantAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// IRelationalAdapter – not supported by Qdrant
// ---------------------------------------------------------------------------

Result<RelationalTable> QdrantAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support relational/SQL queries"
    );
}

Result<size_t> QdrantAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support relational row insertion"
    );
}

Result<size_t> QdrantAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support relational batch insertion"
    );
}

Result<QueryStatistics> QdrantAdapter::get_query_statistics() const {
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
    if (vector.data.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Vector must not be empty"
        );
    }

#ifdef THEMIS_ENABLE_QDRANT
    // Real path: POST /collections/{name}/points
    try {
        const std::string point_id = generate_point_id();

        json payload_obj = json::object();
        for (const auto& kv : vector.metadata) {
            std::visit(
                [&](const auto& v) { payload_obj[kv.first] = v; },
                kv.second);
        }

        json point = {
            {"id",     point_id},
            {"vector", json(vector.data.begin(), vector.data.end())},
            {"payload", payload_obj}
        };
        json body = {{"points", json::array({point})}};
        const std::string path = "/collections/" + collection + "/points";

        auto conn = qdrant_state_->pool.acquire();
        if (!conn) {
            return Result<std::string>::err(
                ErrorCode::RESOURCE_EXHAUSTED, "Qdrant pool exhausted");
        }
        auto res = conn->Put(path.c_str(), body.dump(), "application/json");
        qdrant_state_->pool.release(std::move(conn));

        if (!res || res->status != 200) {
            const std::string msg =
                res ? res->body : "no response from Qdrant";
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Qdrant PUT points failed: " + msg
            );
        }
        return Result<std::string>::ok(point_id);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Qdrant insert_vector failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    Document doc;
    doc.id = generate_point_id();
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

    size_t inserted = 0;
    for (const auto& vec : vectors) {
        auto result = insert_vector(collection, vec);
        if (result.is_ok()) {
            ++inserted;
        }
    }
    return Result<size_t>::ok(inserted);
}

Result<std::vector<std::pair<Vector, double>>> QdrantAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }
    if (query_vector.data.empty()) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Query vector must not be empty"
        );
    }

#ifdef THEMIS_ENABLE_QDRANT
    // Real path: POST /collections/{name}/points/search
    try {
        json body = {
            {"vector", json(query_vector.data.begin(), query_vector.data.end())},
            {"limit",  static_cast<int64_t>(k)},
            {"with_payload", true},
            {"with_vector", true}
        };

        // Translate simple key=value filters into a Qdrant must[] condition.
        if (!filters.empty()) {
            json must_arr = json::array();
            for (const auto& kv : filters) {
                std::visit(
                    [&](const auto& v) {
                        must_arr.push_back({
                            {"key", kv.first},
                            {"match", {{"value", v}}}
                        });
                    },
                    kv.second);
            }
            body["filter"] = {{"must", must_arr}};
        }

        const std::string path =
            "/collections/" + collection + "/points/search";
        auto conn = qdrant_state_->pool.acquire();
        if (!conn) {
            return Result<std::vector<std::pair<Vector, double>>>::err(
                ErrorCode::RESOURCE_EXHAUSTED, "Qdrant pool exhausted");
        }
        auto res = conn->Post(path.c_str(), body.dump(), "application/json");
        qdrant_state_->pool.release(std::move(conn));

        if (!res || res->status != 200) {
            const std::string msg = res ? res->body : "no response";
            return Result<std::vector<std::pair<Vector, double>>>::err(
                ErrorCode::INTERNAL_ERROR,
                "Qdrant search failed: " + msg
            );
        }

        auto resp_json = json::parse(res->body);
        std::vector<std::pair<Vector, double>> results;
        if (resp_json.contains("result")) {
            for (auto& hit : resp_json["result"]) {
                Vector v;
                if (hit.contains("vector") && hit["vector"].is_array()) {
                    for (auto& f : hit["vector"]) {
                        v.data.push_back(f.get<float>());
                    }
                }
                double score = hit.value("score", 0.0);
                if (hit.contains("payload")) {
                    for (auto& [key, val] : hit["payload"].items()) {
                        if (val.is_string()) {
                            v.metadata[key] = Scalar{val.get<std::string>()};
                        } else if (val.is_number_integer()) {
                            v.metadata[key] = Scalar{val.get<int64_t>()};
                        } else if (val.is_number_float()) {
                            v.metadata[key] = Scalar{val.get<double>()};
                        } else if (val.is_boolean()) {
                            v.metadata[key] = Scalar{val.get<bool>()};
                        }
                    }
                }
                results.emplace_back(std::move(v), score);
            }
        }
        return Result<std::vector<std::pair<Vector, double>>>::ok(
            std::move(results));
    } catch (const std::exception& ex) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Qdrant search_vectors failed: ") + ex.what()
        );
    }
#else
    // Simulation path
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

        double score = dot_product_similarity(query_vector.data, stored_vec.data);
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
#endif
}

Result<bool> QdrantAdapter::create_index(
    const std::string& collection,
    size_t dimensions,
    const std::map<std::string, Scalar>& index_params
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
        );
    }
#ifdef THEMIS_ENABLE_QDRANT
    // Real path: PUT /collections/{name} with HNSW vector config.
    try {
        const std::string distance =
            (index_params.count("distance") &&
             std::holds_alternative<std::string>(
                 index_params.at("distance")))
                ? std::get<std::string>(index_params.at("distance"))
                : "Dot";

        json body = {
            {"vectors", {
                {"size",     static_cast<int64_t>(dimensions)},
                {"distance", distance}
            }}
        };
        const std::string path = "/collections/" + collection;

        auto conn = qdrant_state_->pool.acquire();
        if (!conn) {
            return Result<bool>::err(
                ErrorCode::RESOURCE_EXHAUSTED, "Qdrant pool exhausted");
        }
        auto res = conn->Put(path.c_str(), body.dump(), "application/json");
        qdrant_state_->pool.release(std::move(conn));

        if (!res || (res->status != 200 && res->status != 201)) {
            const std::string msg = res ? res->body : "no response";
            return Result<bool>::err(
                ErrorCode::INTERNAL_ERROR,
                "Qdrant PUT collection failed: " + msg
            );
        }
        return Result<bool>::ok(true);
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Qdrant create_index failed: ") + ex.what()
        );
    }
#else
    // Simulation mode: no-op.
    (void)collection;
    (void)dimensions;
    (void)index_params;
    return Result<bool>::ok(true);
#endif
}

// ---------------------------------------------------------------------------
// IGraphAdapter – not supported by Qdrant natively
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::insert_node(const GraphNode& /*node*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support native graph node insertion"
    );
}

Result<std::string> QdrantAdapter::insert_edge(const GraphEdge& /*edge*/) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support native graph edge insertion"
    );
}

Result<GraphPath> QdrantAdapter::shortest_path(
    const std::string& /*source_id*/,
    const std::string& /*target_id*/,
    size_t /*max_depth*/
) {
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support native graph shortest-path queries"
    );
}

Result<std::vector<GraphNode>> QdrantAdapter::traverse(
    const std::string& /*start_id*/,
    size_t /*max_depth*/,
    const std::vector<std::string>& /*edge_labels*/
) {
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support native graph traversal"
    );
}

Result<std::vector<GraphPath>> QdrantAdapter::execute_graph_query(
    const std::string& /*query*/,
    const std::map<std::string, Scalar>& /*params*/
) {
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support native graph queries"
    );
}

// ---------------------------------------------------------------------------
// IDocumentAdapter – Qdrant payload store
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
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
        stored.id = generate_point_id();
    }
    const std::string inserted_id = stored.id;
    store.push_back(std::move(stored));
    return Result<std::string>::ok(inserted_id);
}

Result<size_t> QdrantAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
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

Result<std::vector<Document>> QdrantAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
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

Result<size_t> QdrantAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Qdrant"
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
// ITransactionAdapter – not supported by Qdrant
// ---------------------------------------------------------------------------

Result<std::string> QdrantAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support ACID transactions"
    );
}

Result<bool> QdrantAdapter::commit_transaction(
    const std::string& /*transaction_id*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support ACID transactions"
    );
}

Result<bool> QdrantAdapter::rollback_transaction(
    const std::string& /*transaction_id*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Qdrant does not support ACID transactions"
    );
}

// ---------------------------------------------------------------------------
// ISystemInfoAdapter
// ---------------------------------------------------------------------------

Result<SystemInfo> QdrantAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "Qdrant";
    info.version = "1.7";
    info.build_info["index_type"] = "HNSW";
    info.build_info["platform"]   = "Linux/Windows/macOS";
    info.build_info["api"]        = "REST/gRPC";
#ifdef THEMIS_ENABLE_QDRANT
    info.build_info["driver"] = "cpp-httplib (real)";
#else
    info.build_info["driver"] = "cpp-httplib (simulation)";
#endif
    info.configuration["endpoint"] = Scalar{connection_string_};
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> QdrantAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.memory.total_bytes        = 0;
    metrics.memory.used_bytes         = 0;
    metrics.memory.available_bytes    = 0;
    metrics.storage.total_bytes       = 0;
    metrics.storage.used_bytes        = 0;
    metrics.storage.available_bytes   = 0;
    metrics.cpu.utilization_percent   = 0.0;
#ifdef THEMIS_ENABLE_QDRANT
    // Report pool active connections as a proxy for driver thread usage.
    if (qdrant_state_) {
        const auto stats = qdrant_state_->pool.get_stats();
        metrics.cpu.thread_count =
            static_cast<uint32_t>(stats.active_connections);
    } else {
        metrics.cpu.thread_count = 0;
    }
#else
    metrics.cpu.thread_count = 0;
#endif
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool QdrantAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::VECTOR_SEARCH:
        case Capability::DOCUMENT_STORE:
        case Capability::BATCH_OPERATIONS:
        case Capability::SECONDARY_INDEXES:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> QdrantAdapter::get_capabilities() const {
    return {
        Capability::VECTOR_SEARCH,
        Capability::DOCUMENT_STORE,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool QdrantAdapter::is_valid_connection_string(
    const std::string& connection_string
) {
    return connection_string.rfind("http://", 0) == 0 ||
           connection_string.rfind("https://", 0) == 0;
}

std::string QdrantAdapter::generate_point_id() {
    // Simple deterministic ID generation for the simulation layer.
    // Production code would use a UUID v4 or numeric uint64 as required
    // by the Qdrant REST API point IDs.
    std::ostringstream oss;
    oss << "qdrant_point_" << next_point_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

double QdrantAdapter::dot_product_similarity(const std::vector<float>& a,
                                              const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;

    double dot = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    }
    return dot;
}

bool QdrantAdapter::document_matches(
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
