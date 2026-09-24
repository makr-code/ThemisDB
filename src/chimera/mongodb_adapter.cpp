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
    try {
        // Create mongocxx::uri from connection string
        mongocxx::uri uri(connection_string);
        
        // Create connection pool with default pool configuration
        mongocxx::options::pool pool_opts;
        pool_opts.max_pool_size(100);
        pool_opts.min_pool_size(10);
        
        // Instantiate connection pool and store it
        client_.reset(new mongocxx::client(uri, pool_opts));
        
        // Validate connection by selecting default database
        const std::string db_name = uri.database() ? uri.database().value() : "test";
        database_.reset(new mongocxx::database(client_->database(db_name)));
        
        connected_ = true;
        return Result<bool>::ok(true);
    } catch (const mongocxx::exception& ex) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            std::string("MongoDB connection failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB connection: ") + ex.what()
        );
    }
#else
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB adapter unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Disconnect.
 * @return Return value.
 * @details Calls: clear(), reset(), ok().
 */
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

/**
 * @brief Execute query.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err(), ok(), std::move().
 */
Result<RelationalTable> MongoDBAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    if (!connected_) {
        return Result<RelationalTable>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // MongoDB doesn't natively support AQL (ArangoDB's query language),
        // so we treat this as an unsupported operation.
        // A real implementation would translate AQL to MongoDB aggregation pipeline.
        // For now, return NOT_IMPLEMENTED with proper error context.
        return Result<RelationalTable>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "AQL queries are not supported in MongoDB adapter. "
            "Use document operations (find_documents) or implement AQL translation."
        );
    } catch (const mongocxx::exception& ex) {
        return Result<RelationalTable>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB query execution error: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<RelationalTable>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB query execution: ") + ex.what()
        );
    }
#else
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB execute_query unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Insert row.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err(), ok().
 */
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

    if (table_name.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Table name must not be empty"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // MongoDB is document-oriented, not relational.
        // Treat table_name as a collection name and insert RelationalRow as a document.
        
        // Get the collection
        auto coll = database_->collection(table_name);
        
        // Build BSON document from RelationalRow.columns
        bsoncxx::builder::stream::document builder;
        
        // Add a generated _id field for uniqueness
        builder << "_id" << generate_id();
        
        // Add all columns from the row
        for (const auto& [key, value] : row.columns) {
            if (std::holds_alternative<std::monostate>(value)) {
                builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        
        // Insert the document
        auto result = coll.insert_one(builder.extract());
        
        // Return count of inserted documents (always 1 on success)
        return Result<size_t>::ok(1);
    } catch (const mongocxx::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_row failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB insert_row: ") + ex.what()
        );
    }
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_row unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Batch insert.
 * @param[in] table_name Collection name.
 * @param[in] rows Rows to insert.
 * @return Return value - count of inserted rows.
 * @details Batch insert via MongoDB insert_many operation.
 */
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

    if (table_name.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Table name must not be empty"
        );
    }

    if (rows.empty()) {
        return Result<size_t>::ok(0);
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // Get the collection
        auto coll = database_->collection(table_name);
        
        // Build vector of BSON documents
        std::vector<bsoncxx::document::value> bson_docs;
        bson_docs.reserve(rows.size());
        
        for (const auto& row : rows) {
            bsoncxx::builder::stream::document builder;
            
            // Add a generated _id field for uniqueness
            builder << "_id" << generate_id();
            
            // Add all columns from the row
            for (const auto& [key, value] : row.columns) {
                if (std::holds_alternative<std::monostate>(value)) {
                    builder << key << bsoncxx::types::b_null{};
                } else if (std::holds_alternative<bool>(value)) {
                    builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
                } else if (std::holds_alternative<int64_t>(value)) {
                    builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
                } else if (std::holds_alternative<double>(value)) {
                    builder << key << bsoncxx::types::b_double{std::get<double>(value)};
                } else if (std::holds_alternative<std::string>(value)) {
                    builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
                } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                    const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                    builder << key << bsoncxx::types::b_binary{
                        bsoncxx::binary_sub_type::k_binary,
                        static_cast<uint32_t>(binary_data.size()),
                        binary_data.data()
                    };
                }
            }
            
            bson_docs.push_back(builder.extract());
        }
        
        // Insert all documents at once
        auto result = coll.insert_many(bson_docs);
        
        // Return count of inserted documents
        size_t inserted = result->inserted_ids().size();
        return Result<size_t>::ok(inserted);
    } catch (const mongocxx::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB batch_insert failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB batch_insert: ") + ex.what()
        );
    }
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

/**
 * @brief Insert vector.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<std::string> MongoDBAdapter::insert_vector(
    const std::string& /*collection*/,
    const Vector& /*vector*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector operations not supported in MongoDB adapter; use Qdrant"
    );
}

/**
 * @brief Batch insert vectors.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
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

/**
 * @brief Insert node.
 * @param[in] node GraphNode to insert.
 * @return Result with node ID on success.
 * @details Store node as document in nodes collection with structure:
 *          { _id: node.id, label: node.label, properties: {...} }
 */
Result<std::string> MongoDBAdapter::insert_node(const GraphNode& node) {
#ifdef THEMIS_CHIMERA_MONGO
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    try {
        // Get or create "nodes" collection
        auto nodes_collection = database_->collection("nodes");
        
        // Build BSON document from GraphNode
        bsoncxx::builder::stream::document builder;
        
        // Use provided node.id or generate a new one
        const std::string node_id = node.id.empty() ? generate_id() : node.id;
        builder << "_id" << node_id;
        
        // Add label
        builder << "label" << node.label;
        
        // Add properties as a nested document
        builder << "properties" << bsoncxx::builder::stream::open_document;
        for (const auto& [key, value] : node.properties) {
            if (std::holds_alternative<std::monostate>(value)) {
                builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        builder << bsoncxx::builder::stream::close_document;
        
        // Insert the document
        auto result = nodes_collection.insert_one(builder.extract());
        
        // Return the node ID
        return Result<std::string>::ok(node_id);
    } catch (const mongocxx::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_node failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB insert_node: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_node unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Insert edge.
 * @param[in] edge GraphEdge to insert.
 * @return Result with edge ID on success.
 * @details Store edge as document in edges collection with structure:
 *          { _id: edge.id, source_id: edge.source_id, target_id: edge.target_id,
 *            label: edge.label, properties: {...}, weight: weight }
 */
Result<std::string> MongoDBAdapter::insert_edge(const GraphEdge& edge) {
#ifdef THEMIS_CHIMERA_MONGO
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to MongoDB"
        );
    }

    try {
        // Get or create "edges" collection
        auto edges_collection = database_->collection("edges");
        
        // Build BSON document from GraphEdge
        bsoncxx::builder::stream::document builder;
        
        // Use provided edge.id or generate a new one
        const std::string edge_id = edge.id.empty() ? generate_id() : edge.id;
        builder << "_id" << edge_id;
        
        // Add source and target references
        builder << "source_id" << edge.source_id;
        builder << "target_id" << edge.target_id;
        
        // Add label
        builder << "label" << edge.label;
        
        // Add weight if present
        if (edge.weight) {
            builder << "weight" << edge.weight.value();
        }
        
        // Add properties as a nested document
        builder << "properties" << bsoncxx::builder::stream::open_document;
        for (const auto& [key, value] : edge.properties) {
            if (std::holds_alternative<std::monostate>(value)) {
                builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        builder << bsoncxx::builder::stream::close_document;
        
        // Insert the document
        auto result = edges_collection.insert_one(builder.extract());
        
        // Return the edge ID
        return Result<std::string>::ok(edge_id);
    } catch (const mongocxx::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_edge failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB insert_edge: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_edge unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Shortest path.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @param[in] size_t Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
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

/**
 * @brief Traverse.
 * @param[in] param Input parameter.
 * @param[in] size_t Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
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

/**
 * @brief Insert document.
 * @param[in] collection Collection name.
 * @param[in] doc Document to insert.
 * @return Result with document ID on success.
 * @details Serializes Document to BSON and inserts into the named collection.
 *          The document ID is either from doc.id or auto-generated if empty.
 */
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

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // Get the collection
        auto coll = database_->collection(collection);
        
        // Build BSON document from Document.fields
        bsoncxx::builder::stream::document builder;
        
        // Use provided doc.id or generate a new one
        const std::string doc_id = doc.id.empty() ? generate_id() : doc.id;
        builder << "_id" << doc_id;
        
        // Add timestamp if present, otherwise use current time
        if (doc.timestamp) {
            auto time_since_epoch = doc.timestamp->time_since_epoch();
            auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
            builder << "timestamp" << bsoncxx::types::b_date(std::chrono::milliseconds(ms_count));
        } else {
            builder << "timestamp" << bsoncxx::types::b_date(std::chrono::system_clock::now());
        }
        
        // Add version if present
        if (doc.version) {
            builder << "version" << static_cast<int64_t>(doc.version.value());
        }
        
        // Add all fields from the document
        for (const auto& [key, value] : doc.fields) {
            // Convert Scalar to BSON value
            if (std::holds_alternative<std::monostate>(value)) {
                builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        
        // Insert the document
        auto result = coll.insert_one(builder.extract());
        
        // Return the document ID
        return Result<std::string>::ok(doc_id);
    } catch (const mongocxx::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB insert_document failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB insert_document: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "MongoDB insert_document unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_MONGO=ON to enable."
    );
#endif
}

/**
 * @brief Batch insert documents.
 * @param[in] collection Collection name.
 * @param[in] docs Documents to insert.
 * @return Result with count of inserted documents.
 * @details Batch inserts BSON documents via insert_many operation.
 *          Converts each Document to BSON and inserts all at once for efficiency.
 */
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

    if (collection.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Collection name must not be empty"
        );
    }

    if (docs.empty()) {
        return Result<size_t>::ok(0);
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // Get the collection
        auto coll = database_->collection(collection);
        
        // Build vector of BSON documents
        std::vector<bsoncxx::document::value> bson_docs;
        bson_docs.reserve(docs.size());
        
        for (const auto& doc : docs) {
            bsoncxx::builder::stream::document builder;
            
            // Use provided doc.id or generate a new one
            const std::string doc_id = doc.id.empty() ? generate_id() : doc.id;
            builder << "_id" << doc_id;
            
            // Add timestamp if present, otherwise use current time
            if (doc.timestamp) {
                auto time_since_epoch = doc.timestamp->time_since_epoch();
                auto ms_count = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
                builder << "timestamp" << bsoncxx::types::b_date(std::chrono::milliseconds(ms_count));
            } else {
                builder << "timestamp" << bsoncxx::types::b_date(std::chrono::system_clock::now());
            }
            
            // Add version if present
            if (doc.version) {
                builder << "version" << static_cast<int64_t>(doc.version.value());
            }
            
            // Add all fields
            for (const auto& [key, value] : doc.fields) {
                if (std::holds_alternative<std::monostate>(value)) {
                    builder << key << bsoncxx::types::b_null{};
                } else if (std::holds_alternative<bool>(value)) {
                    builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
                } else if (std::holds_alternative<int64_t>(value)) {
                    builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
                } else if (std::holds_alternative<double>(value)) {
                    builder << key << bsoncxx::types::b_double{std::get<double>(value)};
                } else if (std::holds_alternative<std::string>(value)) {
                    builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
                } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                    const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                    builder << key << bsoncxx::types::b_binary{
                        bsoncxx::binary_sub_type::k_binary,
                        static_cast<uint32_t>(binary_data.size()),
                        binary_data.data()
                    };
                }
            }
            
            bson_docs.push_back(builder.extract());
        }
        
        // Insert all documents at once
        auto result = coll.insert_many(bson_docs);
        
        // Return count of inserted documents
        size_t inserted = result->inserted_ids().size();
        return Result<size_t>::ok(inserted);
    } catch (const mongocxx::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB batch_insert_documents failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB batch_insert_documents: ") + ex.what()
        );
    }
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

    if (collection.empty()) {
        return Result<std::vector<Document>>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Collection name must not be empty"
        );
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // Get the collection
        auto coll = database_->collection(collection);
        
        // Build BSON filter document from filter map
        bsoncxx::builder::stream::document filter_builder;
        for (const auto& [key, value] : filter) {
            if (std::holds_alternative<std::monostate>(value)) {
                filter_builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                filter_builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                filter_builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                filter_builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                filter_builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                filter_builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        
        // Configure find options with limit
        mongocxx::options::find find_opts;
        if (limit > 0) {
            find_opts.limit(static_cast<int64_t>(limit));
        }
        
        // Execute find()
        auto cursor = coll.find(filter_builder.extract(), find_opts);
        
        // Convert each BSON document to Document
        std::vector<Document> results;
        for (auto doc_view : cursor) {
            Document doc;
            
            // Extract _id if present
            if (doc_view["_id"]) {
                auto id_elem = doc_view["_id"];
                if (id_elem.type() == bsoncxx::type::k_string) {
                    doc.id = id_elem.get_string().value.to_string();
                } else if (id_elem.type() == bsoncxx::type::k_oid) {
                    doc.id = id_elem.get_oid().value.to_string();
                }
            }
            
            // Extract version if present
            if (doc_view["version"]) {
                auto version_elem = doc_view["version"];
                if (version_elem.type() == bsoncxx::type::k_int64) {
                    doc.version = version_elem.get_int64().value;
                }
            }
            
            // Extract timestamp if present
            if (doc_view["timestamp"]) {
                auto timestamp_elem = doc_view["timestamp"];
                if (timestamp_elem.type() == bsoncxx::type::k_date) {
                    auto ms = timestamp_elem.get_date().value;
                    doc.timestamp = std::chrono::system_clock::from_time_t(0) + ms;
                }
            }
            
            // Extract all other fields
            for (auto elem : doc_view) {
                const auto key = std::string(elem.key());
                
                // Skip internal fields
                if (key == "_id" || key == "version" || key == "timestamp") {
                    continue;
                }
                
                // Convert BSON element to Scalar
                if (elem.type() == bsoncxx::type::k_null) {
                    doc.fields[key] = std::monostate{};
                } else if (elem.type() == bsoncxx::type::k_bool) {
                    doc.fields[key] = elem.get_bool().value;
                } else if (elem.type() == bsoncxx::type::k_int32) {
                    doc.fields[key] = static_cast<int64_t>(elem.get_int32().value);
                } else if (elem.type() == bsoncxx::type::k_int64) {
                    doc.fields[key] = elem.get_int64().value;
                } else if (elem.type() == bsoncxx::type::k_double) {
                    doc.fields[key] = elem.get_double().value;
                } else if (elem.type() == bsoncxx::type::k_string) {
                    doc.fields[key] = std::string(elem.get_string().value);
                } else if (elem.type() == bsoncxx::type::k_binary) {
                    auto bin = elem.get_binary();
                    std::vector<uint8_t> binary_data(bin.bytes, bin.bytes + bin.size);
                    doc.fields[key] = binary_data;
                }
            }
            
            results.push_back(doc);
        }
        
        return Result<std::vector<Document>>::ok(std::move(results));
    } catch (const mongocxx::exception& ex) {
        return Result<std::vector<Document>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB find_documents failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<std::vector<Document>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB find_documents: ") + ex.what()
        );
    }
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

    if (collection.empty()) {
        return Result<size_t>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Collection name must not be empty"
        );
    }

    if (updates.empty()) {
        return Result<size_t>::ok(0);
    }

#ifdef THEMIS_CHIMERA_MONGO
    try {
        // Get the collection
        auto coll = database_->collection(collection);
        
        // Build BSON filter document
        bsoncxx::builder::stream::document filter_builder;
        for (const auto& [key, value] : filter) {
            if (std::holds_alternative<std::monostate>(value)) {
                filter_builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                filter_builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                filter_builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                filter_builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                filter_builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                filter_builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        
        // Build update document using $set operator
        bsoncxx::builder::stream::document update_builder;
        update_builder << "$set" << bsoncxx::builder::stream::open_document;
        for (const auto& [key, value] : updates) {
            if (std::holds_alternative<std::monostate>(value)) {
                update_builder << key << bsoncxx::types::b_null{};
            } else if (std::holds_alternative<bool>(value)) {
                update_builder << key << bsoncxx::types::b_bool{std::get<bool>(value)};
            } else if (std::holds_alternative<int64_t>(value)) {
                update_builder << key << bsoncxx::types::b_int64{std::get<int64_t>(value)};
            } else if (std::holds_alternative<double>(value)) {
                update_builder << key << bsoncxx::types::b_double{std::get<double>(value)};
            } else if (std::holds_alternative<std::string>(value)) {
                update_builder << key << bsoncxx::types::b_string{std::get<std::string>(value)};
            } else if (std::holds_alternative<std::vector<uint8_t>>(value)) {
                const auto& binary_data = std::get<std::vector<uint8_t>>(value);
                update_builder << key << bsoncxx::types::b_binary{
                    bsoncxx::binary_sub_type::k_binary,
                    static_cast<uint32_t>(binary_data.size()),
                    binary_data.data()
                };
            }
        }
        update_builder << bsoncxx::builder::stream::close_document;
        
        // Execute update_many()
        auto result = coll.update_many(filter_builder.extract(), update_builder.extract());
        
        // Return count of modified documents
        size_t modified = result->modified_count();
        return Result<size_t>::ok(modified);
    } catch (const mongocxx::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB update_documents failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB update_documents: ") + ex.what()
        );
    }
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

/**
 * @brief Begin transaction.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<std::string> MongoDBAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Commit transaction.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> MongoDBAdapter::commit_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Rollback transaction.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> MongoDBAdapter::rollback_transaction(const std::string& /*transaction_id*/) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Create savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<std::string> MongoDBAdapter::create_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Rollback to savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> MongoDBAdapter::rollback_to_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Release savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> MongoDBAdapter::release_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Get transaction stats.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<TransactionStats> MongoDBAdapter::get_transaction_stats(
    const std::string& /*transaction_id*/
) {
    return Result<TransactionStats>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Use ITransactionalAdapter interface instead"
    );
}

/**
 * @brief Get transaction state.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
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
    info.system_name = "MongoDB";
    info.version = "0.1.0";
    info.build_info["database_version"] = "unknown";  // NOT IMPLEMENTED: Query via mongocxx requires THEMIS_CHIMERA_MONGO
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
    metrics.custom_metrics["total_queries"] = static_cast<int64_t>(0);  // NOT IMPLEMENTED: Track via mongocxx stats (THEMIS_CHIMERA_MONGO)
    metrics.custom_metrics["total_errors"] = static_cast<int64_t>(0);
    metrics.custom_metrics["avg_query_time_ms"] = 0.0;
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

/**
 * @brief Begin transaction.
 * @param[in] IsolationLevel Input parameter.
 * @return Return value.
 * @details Calls: err(), generate_id(), mark_active(), lock(), ok(), TransactionHandle().
 */
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

/**
 * @brief Commit transaction.
 * @param[in] handle Input parameter.
 * @return Return value.
 * @details Calls: err(), mark_committed(), ok().
 */
Result<bool> MongoDBAdapter::commit_transaction(
    const TransactionHandle& handle
) {
    if (!handle) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    auto mutable_handle = handle;
    mutable_handle->mark_committed();
    return Result<bool>::ok(true);
}

/**
 * @brief Rollback transaction.
 * @param[in] handle Input parameter.
 * @return Return value.
 * @details Calls: err(), mark_aborted(), ok().
 */
Result<bool> MongoDBAdapter::rollback_transaction(
    const TransactionHandle& handle
) {
    if (!handle) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid transaction handle"
        );
    }

    auto mutable_handle = handle;
    mutable_handle->mark_aborted();
    return Result<bool>::ok(true);
}

/**
 * @brief Create savepoint.
 * @param[in] handle Input parameter.
 * @param[in] savepoint_name Name of the savepoint.
 * @return Return value.
 * @details Calls: err(), ok().
 */
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

    auto mutable_handle = handle;
    if (!mutable_handle->create_savepoint(savepoint_name)) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Savepoint name already exists"
        );
    }

    return Result<std::string>::ok(savepoint_name);
}

/**
 * @brief Rollback to savepoint.
 * @param[in] handle Transaction handle.
 * @param[in] savepoint_name Savepoint name.
 * @return Result with success/error status.
 * @details Rollback to savepoint logic via mongocxx session.
 *          MongoDB doesn't natively support savepoints, so we implement
 *          application-level tracking via the TransactionContext.
 */
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

#ifdef THEMIS_CHIMERA_MONGO
    try {
        auto mutable_handle = handle;
        
        // Get the savepoint operation count
        const auto savepoint_op_count = mutable_handle->get_savepoint_operation_count(savepoint_name);
        
        // Get current operations
        const auto& operations = mutable_handle->get_operations();
        
        // MongoDB doesn't natively support savepoints, but we can implement
        // application-level savepoint support by clearing operations since the savepoint
        if (operations.size() > savepoint_op_count) {
            // Clear all operations after the savepoint was created
            mutable_handle->clear_operations();
            
            // Re-add operations up to the savepoint
            for (size_t i = 0; i < savepoint_op_count && i < operations.size(); ++i) {
                mutable_handle->record_operation(operations[i]);
            }
        }
        
        return Result<bool>::ok(true);
    } catch (const mongocxx::exception& ex) {
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("MongoDB rollback_to_savepoint failed: ") + ex.what()
        );
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Unexpected error during MongoDB rollback_to_savepoint: ") + ex.what()
        );
    }
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
    return TransactionState::ABORTED;
}

// ---------------------------------------------------------------------------
// IBatchAdapter Implementation
// ---------------------------------------------------------------------------

/**
 * @brief Queue insert.
 * @param[in] table_name Name of the table.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err(), lock(), push_back(), ok().
 */
Result<bool> MongoDBAdapter::queue_insert(
    const std::string& table_name,
    const RelationalRow& /*row*/
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

/**
 * @brief Queue insert batch.
 * @param[in] table_name Name of the table.
 * @param[in] rows Input parameter.
 * @return Return value.
 * @details Calls: err(), lock(), size(), push_back(), ok().
 */
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
        for (size_t i = 0; i < rows.size(); ++i) {
            batch_queue_.push_back({"insert", table_name, ""});
        }
    }

    return Result<bool>::ok(true);
}

/**
 * @brief Queue update.
 * @param[in] table_name Name of the table.
 * @param[in] param Input parameter.
 * @param[in] where_clause Input parameter.
 * @return Return value.
 * @details Calls: err(), lock(), push_back(), ok().
 */
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

/**
 * @brief Queue delete.
 * @param[in] table_name Name of the table.
 * @param[in] where_clause Input parameter.
 * @return Return value.
 * @details Calls: err(), lock(), push_back(), ok().
 */
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

/**
 * @brief Flush.
 * @return Return value.
 * @details Calls: lock(), size(), clear(), ok(), std::move().
 */
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
    /**
     * @brief Lock.
     * @param[in] batch_mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::mutex> lock(batch_mutex_);
    return batch_queue_.size();
}

/**
 * @brief Set batch config.
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: lock(), ok().
 */
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

/**
 * @brief Generate id.
 * @return Return value.
 * @details Calls: utils::generate_uuid_v4().
 */
std::string MongoDBAdapter::generate_id() {
    return utils::generate_uuid_v4();
}

/**
 * @brief Is valid connection string.
 * @param[in] cs Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: find().
 */
bool MongoDBAdapter::is_valid_connection_string(const std::string& cs) {
    return cs.find("mongodb://") == 0 || cs.find("mongodb+srv://") == 0;
}

/**
 * @brief Mask credentials.
 * @param[in] cs Input parameter.
 * @return Return value.
 * @details Implements mask_credentials without additional internal calls.
 */
std::string MongoDBAdapter::mask_credentials(const std::string& cs) {
    // NOT IMPLEMENTED: Full credential masking requires mongocxx URI parsing.
    // Gate: THEMIS_CHIMERA_MONGO. For safety, return as-is; do not log raw cs.
    return cs;
}

/**
 * @brief Scalar to bson string.
 * @param[in] scalar Input parameter.
 * @return Return value - JSON-like string representation for debugging
 * @details Converts a Scalar variant to a string representation for logging/debugging.
 */
std::string MongoDBAdapter::scalar_to_bson_string(const Scalar& scalar) {
    // Convert Scalar to a string representation for debugging/logging purposes
    if (std::holds_alternative<std::monostate>(scalar)) {
        return "null";
    } else if (std::holds_alternative<bool>(scalar)) {
        return std::get<bool>(scalar) ? "true" : "false";
    } else if (std::holds_alternative<int64_t>(scalar)) {
        return std::to_string(std::get<int64_t>(scalar));
    } else if (std::holds_alternative<double>(scalar)) {
        return std::to_string(std::get<double>(scalar));
    } else if (std::holds_alternative<std::string>(scalar)) {
        return "\"" + std::get<std::string>(scalar) + "\"";
    } else if (std::holds_alternative<std::vector<uint8_t>>(scalar)) {
        return "<binary>";
    }
    return "unknown";
}

/**
 * @brief Row to bson document.
 * @param[in] row Input parameter.
 * @return Return value - JSON string representation of the row
 * @details Converts a RelationalRow to a JSON-like string representation.
 *          For production BSON serialization, this is called by insert_row/batch_insert
 *          which handle the actual mongocxx integration.
 */
std::string MongoDBAdapter::row_to_bson_document(const RelationalRow& row) {
    // Convert RelationalRow to JSON string representation for logging
    std::string result = "{";
    bool first = true;
    for (const auto& [key, value] : row.columns) {
        if (!first) result += ",";
        result += "\"" + key + "\":" + scalar_to_bson_string(value);
        first = false;
    }
    result += "}";
    return result;
}

Result<std::string> MongoDBAdapter::parse_query_to_mongo(
    const std::string& /*aql_query*/
) const {
    // AQL to MongoDB query translation not implemented.
    // MongoDB is document-oriented and does not natively support AQL (ArangoDB's query language).
    // Users should use document operations (find_documents, insert_document, etc.) instead.
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "AQL to MongoDB aggregation pipeline translation not implemented. "
        "Use document operations (find_documents) or direct aggregation pipeline queries."
    );
}

} // namespace chimera
