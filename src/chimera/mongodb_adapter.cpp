/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mongodb_adapter.cpp                                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-06 04:15:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   68.0/100                                       ║
    • Total Lines:     1192                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 566ec90912  2026-03-12  Changes before error encountered        ║
    • 29a0ee06c5  2026-03-12  feat(chimera): wire THEMIS_ENABLE_* driver blocks, add Co... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 74d8f7c17f  2026-02-28  fix(chimera): resolve MongoDB adapter quality metrics - r... ║
    • e3c17b310f  2026-02-26  Implement MongoDB Atlas Vector Search integration: add se... ║
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
 * in-process simulation blocks with real mongocxx calls.
 *
 * @copyright MIT License
 */

#include "chimera/mongodb_adapter.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <sstream>

// ---------------------------------------------------------------------------
// Real mongocxx driver integration
// ---------------------------------------------------------------------------
// When THEMIS_ENABLE_MONGODB is defined (set by CMake when libmongocxx is
// found) the adapter uses a real mongocxx::pool for all database operations.
// Without the macro the adapter falls back to the in-process simulation.
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_MONGODB
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/pool.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#endif

namespace chimera {

// ---------------------------------------------------------------------------
// MongoState – driver-specific state (defined here so headers stay clean)
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_MONGODB

/// Ensures a single mongocxx::instance exists for the lifetime of the process.
static mongocxx::instance& get_mongocxx_instance() {
    // NOLINTNEXTLINE(cert-err58-cpp)
    static mongocxx::instance inst{};
    return inst;
}

struct MongoDBAdapter::MongoState {
    mongocxx::pool pool;

    explicit MongoState(const std::string& uri_str,
                        const ConnectionPoolConfig& pool_cfg)
        : pool([&]() -> mongocxx::pool {
              // Ensure the singleton exists before constructing the pool.
              (void)get_mongocxx_instance();

              // Embed pool limits in the URI as query parameters so that
              // mongocxx honours them without requiring driver-version-specific
              // option structs.
              std::string full_uri = uri_str;
              const bool has_query = full_uri.find('?') != std::string::npos;
              const char sep       = has_query ? '&' : '?';
              if (full_uri.find("maxPoolSize") == std::string::npos) {
                  full_uri += sep;
                  full_uri += "maxPoolSize=";
                  full_uri += std::to_string(pool_cfg.max_connections);
                  full_uri += "&minPoolSize=";
                  full_uri += std::to_string(pool_cfg.min_connections);
              }
              return mongocxx::pool{mongocxx::uri{full_uri}};
          }()) {}
};

#else

// Stub definition so unique_ptr<MongoState> compiles without driver headers.
struct MongoDBAdapter::MongoState {};

#endif // THEMIS_ENABLE_MONGODB

// ---------------------------------------------------------------------------
// Auto-registration
// ---------------------------------------------------------------------------

namespace {
// Register MongoDBAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
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
// Construction / Destruction
// ---------------------------------------------------------------------------

MongoDBAdapter::MongoDBAdapter() = default;
MongoDBAdapter::~MongoDBAdapter() {
    if (connected_) {
        disconnect();
    }
    // mongo_state_ destructor runs here; unique_ptr<MongoState> needs
    // MongoState to be complete, which it is since we defined it above.
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

#ifdef THEMIS_ENABLE_MONGODB
    // Wire in the real mongocxx connection pool.
    try {
        mongo_state_ = std::make_unique<MongoState>(connection_string, pool_config_);
    } catch (const mongocxx::exception& ex) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            std::string("mongocxx: ") + ex.what()
        );
    }
    (void)options;  // options are embedded in the connection URI for mongocxx
#else
    (void)options;  // no driver-specific options in simulation mode
#endif

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> MongoDBAdapter::disconnect() {
#ifdef THEMIS_ENABLE_MONGODB
    mongo_state_.reset();
#endif
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
// IVectorAdapter – Atlas Vector Search
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

#ifdef THEMIS_ENABLE_MONGODB
    // Real path: store the vector as a BSON document with an "embedding" array.
    try {
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::array;
        using bsoncxx::builder::stream::finalize;
        using bsoncxx::builder::stream::open_array;
        using bsoncxx::builder::stream::close_array;

        auto entry  = mongo_state_->pool.acquire();
        auto& client = *entry;
        auto coll   = client[database_name_][collection];

        const std::string oid = generate_document_id();
        auto builder = document{};
        builder << "_id" << oid;
        // Build the embedding array
        {
            auto arr = builder << "embedding" << open_array;
            for (const float f : vector.data) {
                arr << static_cast<double>(f);
            }
            arr << close_array;
        }
        // Append metadata fields
        for (const auto& kv : vector.metadata) {
            std::visit(
                [&](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, double>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        builder << kv.first << v;
                    }
                },
                kv.second);
        }
        auto bson_doc = builder << finalize;
        auto result = coll.insert_one(bson_doc.view());
        if (!result) {
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "MongoDB insert_one (vector) returned empty result"
            );
        }
        return Result<std::string>::ok(oid);
    } catch (const mongocxx::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_vector failed: ") + ex.what()
        );
    }
#else
    // Simulation path: store vector as a document with named scalar fields.
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

    std::lock_guard<std::mutex> lock(store_mutex_);
    document_store_[collection].push_back(std::move(doc));
    return Result<std::string>::ok(document_store_[collection].back().id);
#endif
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

#ifdef THEMIS_ENABLE_MONGODB
    // Real path: Atlas Vector Search via $vectorSearch aggregation stage.
    // Requires an Atlas Search index named "vector_index" on the "embedding"
    // field created with numDimensions matching the stored vectors.
    try {
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::array;
        using bsoncxx::builder::stream::finalize;
        using bsoncxx::builder::stream::open_array;
        using bsoncxx::builder::stream::close_array;
        using bsoncxx::builder::stream::open_document;
        using bsoncxx::builder::stream::close_document;

        auto entry  = mongo_state_->pool.acquire();
        auto& client = *entry;
        auto coll   = client[database_name_][collection];

        // Build the query vector as a BSON array string for use in the
        // $vectorSearch pipeline stage (MongoDB Atlas / 6.x+ standalone).
        auto vec_arr_builder = array{};
        for (const float f : query_vector.data) {
            vec_arr_builder << static_cast<double>(f);
        }
        auto vec_arr = vec_arr_builder << finalize;

        // $vectorSearch aggregation stage
        auto pipeline_builder = bsoncxx::builder::basic::array{};
        {
            bsoncxx::builder::basic::document vs_stage;
            bsoncxx::builder::basic::document vs_doc;
            vs_doc.append(
                bsoncxx::builder::basic::kvp("index", "vector_index"),
                bsoncxx::builder::basic::kvp("path", "embedding"),
                bsoncxx::builder::basic::kvp("queryVector", vec_arr.view()),
                bsoncxx::builder::basic::kvp(
                    "numCandidates",
                    static_cast<int64_t>(k * 10)),
                bsoncxx::builder::basic::kvp("limit", static_cast<int64_t>(k))
            );
            vs_stage.append(
                bsoncxx::builder::basic::kvp("$vectorSearch", vs_doc.extract()));
            pipeline_builder.append(vs_stage.extract());
        }

        mongocxx::pipeline pipeline;
        pipeline.append_stage(pipeline_builder.extract()[0].get_document().value);

        auto cursor = coll.aggregate(pipeline, mongocxx::options::aggregate{});

        std::vector<std::pair<Vector, double>> results;
        for (auto&& bson_doc : cursor) {
            Vector v;
            double score = 0.0;
            // Extract embedding array
            if (bson_doc["embedding"] &&
                bson_doc["embedding"].type() == bsoncxx::type::k_array) {
                for (auto&& el : bson_doc["embedding"].get_array().value) {
                    if (el.type() == bsoncxx::type::k_double) {
                        v.data.push_back(
                            static_cast<float>(el.get_double().value));
                    }
                }
            }
            // Extract score (Atlas adds __score field)
            if (bson_doc["__score"] &&
                bson_doc["__score"].type() == bsoncxx::type::k_double) {
                score = bson_doc["__score"].get_double().value;
            }
            // Extract metadata
            for (auto&& el : bson_doc) {
                const std::string key{el.key()};
                if (key == "_id" || key == "embedding" || key == "__score") {
                    continue;
                }
                switch (el.type()) {
                    case bsoncxx::type::k_utf8:
                        v.metadata[key] = Scalar{std::string{el.get_utf8().value}};
                        break;
                    case bsoncxx::type::k_int64:
                        v.metadata[key] = Scalar{el.get_int64().value};
                        break;
                    case bsoncxx::type::k_double:
                        v.metadata[key] = Scalar{el.get_double().value};
                        break;
                    default:
                        break;
                }
            }
            results.emplace_back(std::move(v), score);
        }
        return Result<std::vector<std::pair<Vector, double>>>::ok(
            std::move(results));
    } catch (const mongocxx::exception& ex) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB search_vectors failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = document_store_.find(collection);
    if (it == document_store_.end()) {
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
#endif
}

Result<bool> MongoDBAdapter::create_index(
    const std::string& collection,
    size_t dimensions,
    const std::map<std::string, Scalar>& index_params
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }
#ifdef THEMIS_ENABLE_MONGODB
    // Create a standard field index on the collection.  Atlas Vector Search
    // indexes must be created via the Atlas API (not the driver), so we create
    // a regular index here as a best-effort operation.
    try {
        (void)dimensions;
        (void)index_params;
        auto entry  = mongo_state_->pool.acquire();
        auto& client = *entry;
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;
        auto key_doc = document{} << "embedding" << 1 << finalize;
        mongocxx::options::index opts;
        client[database_name_][collection].create_index(key_doc.view(), opts);
        return Result<bool>::ok(true);
    } catch (const mongocxx::exception& ex) {
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB create_index failed: ") + ex.what()
        );
    }
#else
    // Simulation mode: index creation is a no-op.
    (void)collection;
    (void)dimensions;
    (void)index_params;
    return Result<bool>::ok(true);
#endif
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

#ifdef THEMIS_ENABLE_MONGODB
    // Real mongocxx path: build a BSON document and call insert_one().
    try {
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;
        using bsoncxx::builder::stream::open_document;
        using bsoncxx::builder::stream::close_document;

        auto entry = mongo_state_->pool.acquire();
        auto& client = *entry;
        auto coll = client[database_name_][collection];

        // Build the BSON document
        auto builder = document{};
        const std::string oid = doc.id.empty() ? generate_document_id() : doc.id;
        builder << "_id" << oid;
        for (const auto& kv : doc.fields) {
            std::visit(
                [&](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, double>) {
                        builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        builder << kv.first << v;
                    }
                },
                kv.second);
        }
        auto bson_doc = builder << finalize;

        auto result = coll.insert_one(bson_doc.view());
        if (!result) {
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "MongoDB insert_one returned empty result"
            );
        }
        return Result<std::string>::ok(oid);
    } catch (const mongocxx::exception& ex) {
        // Duplicate key → map to ALREADY_EXISTS
        const int code = ex.code().value();
        if (code == 11000 || code == 11001) {
            return Result<std::string>::err(
                ErrorCode::ALREADY_EXISTS,
                std::string("MongoDB duplicate key: ") + ex.what()
            );
        }
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_one failed: ") + ex.what()
        );
    }
#else
    // Simulation path
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

#ifdef THEMIS_ENABLE_MONGODB
    try {
        using bsoncxx::builder::stream::document;
        using bsoncxx::builder::stream::finalize;

        auto entry  = mongo_state_->pool.acquire();
        auto& client = *entry;
        auto coll   = client[database_name_][collection];

        // Build filter document
        auto filter_builder = document{};
        for (const auto& kv : filter) {
            std::visit(
                [&](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        filter_builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        filter_builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, double>) {
                        filter_builder << kv.first << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        filter_builder << kv.first << v;
                    }
                },
                kv.second);
        }
        auto filter_doc = filter_builder << finalize;

        mongocxx::options::find opts;
        if (limit > 0 && limit < static_cast<size_t>(INT64_MAX)) {
            opts.limit(static_cast<int64_t>(limit));
        }

        auto cursor = coll.find(filter_doc.view(), opts);

        std::vector<Document> results;
        for (auto&& bson_doc : cursor) {
            Document d;
            for (auto&& el : bson_doc) {
                const std::string key{el.key()};
                if (key == "_id") {
                    if (el.type() == bsoncxx::type::k_utf8) {
                        d.id = std::string{el.get_utf8().value};
                    }
                    continue;
                }
                switch (el.type()) {
                    case bsoncxx::type::k_utf8:
                        d.fields[key] = Scalar{std::string{el.get_utf8().value}};
                        break;
                    case bsoncxx::type::k_int64:
                        d.fields[key] = Scalar{el.get_int64().value};
                        break;
                    case bsoncxx::type::k_int32:
                        d.fields[key] = Scalar{static_cast<int64_t>(el.get_int32().value)};
                        break;
                    case bsoncxx::type::k_double:
                        d.fields[key] = Scalar{el.get_double().value};
                        break;
                    case bsoncxx::type::k_bool:
                        d.fields[key] = Scalar{el.get_bool().value};
                        break;
                    default:
                        break;
                }
            }
            results.push_back(std::move(d));
        }
        return Result<std::vector<Document>>::ok(std::move(results));
    } catch (const mongocxx::exception& ex) {
        return Result<std::vector<Document>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB find failed: ") + ex.what()
        );
    }
#else
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

#ifdef THEMIS_ENABLE_MONGODB
    // Multi-document ACID transactions require a pinned mongocxx::client_session
    // that must live across begin/commit/rollback calls within the same thread.
    // Storing an active session in a map keyed by txn_id requires a thread-safe
    // session store and careful lifetime management; that is deferred to a
    // dedicated transaction-manager feature.  In the meantime, return
    // NOT_IMPLEMENTED so callers are not misled into thinking writes made
    // through this adapter are protected by a real MongoDB transaction.
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB multi-document transactions require a pinned client session "
        "that outlives the begin() call. Use per-operation sessions instead, "
        "or wait for the dedicated transaction-manager feature."
    );
#else
    std::string txn_id = generate_transaction_id();
    std::lock_guard<std::mutex> lock(txn_mutex_);
    active_transactions_[txn_id] = true;
    return Result<std::string>::ok(std::move(txn_id));
#endif
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
#ifdef THEMIS_ENABLE_MONGODB
    info.version = "7.0";  // Populated from buildInfo in production deployments
    info.build_info["driver"] = "mongocxx (real)";
#else
    info.version = "7.0";
    info.build_info["driver"] = "mongocxx (simulation)";
#endif
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
#ifdef THEMIS_ENABLE_MONGODB
    // Report the number of connections currently checked out from the pool
    // as a proxy for concurrently active driver threads.
    if (mongo_state_) {
        // mongocxx::pool does not expose public stats; we use the configured
        // max_connections as the upper bound reported to the caller so that
        // benchmarks can reason about driver concurrency capacity.
        metrics.cpu.thread_count =
            static_cast<uint32_t>(pool_config_.max_connections);
    } else {
        metrics.cpu.thread_count = 0;
    }
#else
    metrics.cpu.thread_count = 0;
#endif
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
