/**
 * @file neo4j_adapter.cpp
 * @brief Neo4j backend adapter implementation.
 *
 * Cypher generation, result mapping, and connection lifecycle for
 * the Chimera/Neo4j integration.
 */

#include "chimera/neo4j_adapter.hpp"
#include "utils/uuid.h"

#include <cassert>
#include <sstream>
#include <algorithm>
#include <regex>
#include <stdexcept>
#include <limits>

namespace chimera {

// Auto-registration
namespace {
const bool neo4j_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "Neo4j",
        []() { return std::make_unique<Neo4jAdapter>(); }
    );
    assert(ok && "Neo4jAdapter: 'Neo4j' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Constructor and Destructor
// ---------------------------------------------------------------------------

Neo4jAdapter::Neo4jAdapter() = default;

Neo4jAdapter::~Neo4jAdapter() {
    if (connected_) {
        disconnect();
    }
}

// ---------------------------------------------------------------------------
// Connection Management
// ---------------------------------------------------------------------------

Result<bool> Neo4jAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& options
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Neo4j connection string must not be empty"
        );
    }

    if (!is_valid_connection_string(connection_string)) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid Neo4j connection string: must be bolt:// or neo4j:// URL"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        // Extract username and password from options if provided, or from URI
        std::string username = "neo4j";
        std::string password = "password";
         
        if (options.find("username") != options.end()) {
            username = options.at("username");
        }
        if (options.find("password") != options.end()) {
            password = options.at("password");
        }
         
        // Create URI and driver
        // Note: This uses the Neo4j C++ driver API
        auto uri = neo4j::Uri::create(connection_string);
        auto auth = neo4j::basic_auth(username, password);
         
        // Create driver with connection pooling configuration
        neo4j::DriverConfig config;
        config.with_auth(auth);
        config.with_connection_timeout(std::chrono::seconds(30));
         
        driver_ = std::make_unique<neo4j::Driver>(
            neo4j::make_driver(uri, config)
        );
         
        // Verify connectivity by running a test query
        {
            auto session = driver_->session();
            auto result = session.run("RETURN 1 AS connection_test");
             
            // Consume result to ensure connection succeeded
            if (!result.has_value()) {
                return Result<bool>::err(
                    ErrorCode::CONNECTION_ERROR,
                    "Neo4j connection verification failed: no result from test query"
                );
            }
             
            session.close();
        }
         
        connection_string_ = mask_credentials(connection_string);
        connected_ = true;
        return Result<bool>::ok(true);
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            std::string("Neo4j connection failed: ") + ex.what()
        );
    }
#else
    connection_string_.clear();
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j adapter unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Disconnect.
 * @return Return value.
 * @details Calls: clear(), lock(), ok().
 */
Result<bool> Neo4jAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    {
        std::unique_lock<std::mutex> lock(session_mutex_);
        active_sessions_.clear();
    }
    return Result<bool>::ok(true);
}

bool Neo4jAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// Relational Adapter (Not Supported)
// ---------------------------------------------------------------------------

/**
 * @brief Execute query.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<RelationalTable> Neo4jAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational queries not supported in Neo4j adapter; use ThemisDB/MongoDB"
    );
}

/**
 * @brief Insert row.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<size_t> Neo4jAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational insert not supported in Neo4j adapter"
    );
}

/**
 * @brief Batch insert.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<size_t> Neo4jAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Relational batch insert not supported in Neo4j adapter"
    );
}

Result<QueryStatistics> Neo4jAdapter::get_query_statistics() const {
    QueryStatistics stats = {};
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
Result<std::string> Neo4jAdapter::insert_vector(
    const std::string& /*collection*/,
    const Vector& /*vector*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector operations not supported in Neo4j adapter; use Qdrant"
    );
}

/**
 * @brief Batch insert vectors.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<size_t> Neo4jAdapter::batch_insert_vectors(
    const std::string& /*collection*/,
    const std::vector<Vector>& /*vectors*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector operations not supported in Neo4j adapter; use Qdrant"
    );
}

Result<std::vector<std::pair<Vector, double>>> Neo4jAdapter::search_vectors(
    const std::string& /*collection*/,
    const Vector& /*query_vector*/,
    size_t /*k*/,
    const std::map<std::string, Scalar>& /*filters*/
) {
    return Result<std::vector<std::pair<Vector, double>>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector search not supported in Neo4j adapter; use Qdrant"
    );
}

Result<bool> Neo4jAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Vector index creation not supported in Neo4j adapter"
    );
}

// ---------------------------------------------------------------------------
// Graph Adapter (Primary Support)
// ---------------------------------------------------------------------------

/**
 * @brief Insert node.
 * @param[in] node Input parameter.
 * @return Return value.
 * @details Calls: err(), generate_id(), ok().
 */
Result<std::string> Neo4jAdapter::insert_node(const GraphNode& node) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::string>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        auto session = driver_->session();
        const std::string node_id = node.id.empty() ? generate_id() : node.id;
         
        // Build Cypher CREATE query with parameters
        std::string cypher = "CREATE (n:" + node.label + " {id: $id";
         
        // Add property placeholders to Cypher query
        for (const auto& [key, _] : node.properties) {
            cypher += ", " + key + ": $" + key;
        }
        cypher += "}) RETURN n.id";
         
        // Build parameters map
        neo4j::MapBuilder builder;
        builder.add_string("id", node_id);
         
        for (const auto& [key, val] : node.properties) {
            // Convert Scalar to neo4j::Value
            if (std::get_if<std::monostate>(&val)) {
                builder.add_null(key);
            } else if (auto* b = std::get_if<bool>(&val)) {
                builder.add_bool(key, *b);
            } else if (auto* i = std::get_if<int64_t>(&val)) {
                builder.add_int64(key, *i);
            } else if (auto* d = std::get_if<double>(&val)) {
                builder.add_double(key, *d);
            } else if (auto* s = std::get_if<std::string>(&val)) {
                builder.add_string(key, *s);
            } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                builder.add_bytes(key, *b);
            }
        }
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        // Extract result
        if (!result.has_value()) {
            session.close();
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j insert_node: no result from CREATE query"
            );
        }
         
        auto record = result->single();
        session.close();
         
        return Result<std::string>::ok(node_id);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j insert_node failed: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j insert_node unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Insert edge.
 * @param[in] edge Input parameter.
 * @return Return value.
 * @details Calls: err(), generate_id(), ok().
 */
Result<std::string> Neo4jAdapter::insert_edge(const GraphEdge& edge) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::string>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        auto session = driver_->session();
        const std::string edge_id = edge.id.empty() ? generate_id() : edge.id;
         
        // Build Cypher MATCH+CREATE query for edge
        std::string cypher = 
            "MATCH (from {id: $source_id}), (to {id: $target_id}) "
            "CREATE (from)-[r:" + edge.label + " {id: $id";
         
        // Add property placeholders
        if (edge.weight) {
            cypher += ", weight: $weight";
        }
        for (const auto& [key, _] : edge.properties) {
            cypher += ", " + key + ": $" + key;
        }
        cypher += "}]->(to) RETURN r.id";
         
        // Build parameters
        neo4j::MapBuilder builder;
        builder.add_string("source_id", edge.source_id);
        builder.add_string("target_id", edge.target_id);
        builder.add_string("id", edge_id);
         
        if (edge.weight) {
            builder.add_double("weight", *edge.weight);
        }
         
        for (const auto& [key, val] : edge.properties) {
            if (std::get_if<std::monostate>(&val)) {
                builder.add_null(key);
            } else if (auto* b = std::get_if<bool>(&val)) {
                builder.add_bool(key, *b);
            } else if (auto* i = std::get_if<int64_t>(&val)) {
                builder.add_int64(key, *i);
            } else if (auto* d = std::get_if<double>(&val)) {
                builder.add_double(key, *d);
            } else if (auto* s = std::get_if<std::string>(&val)) {
                builder.add_string(key, *s);
            } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                builder.add_bytes(key, *b);
            }
        }
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        if (!result.has_value()) {
            session.close();
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j insert_edge: no result from CREATE query"
            );
        }
         
        session.close();
        return Result<std::string>::ok(edge_id);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j insert_edge failed: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j insert_edge unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Shortest path.
 * @param[in] source_id Identifier of the source.
 * @param[in] target_id Identifier of the target.
 * @param[in] max_depth Input parameter.
 * @return Return value.
 * @details Calls: err(), ok(), std::move().
 */
Result<GraphPath> Neo4jAdapter::shortest_path(
    const std::string& source_id,
    const std::string& target_id,
    size_t max_depth
) {
    if (!connected_) {
        return Result<GraphPath>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<GraphPath>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (source_id.empty() || target_id.empty()) {
            return Result<GraphPath>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Source ID and target ID must not be empty"
            );
        }
         
        auto session = driver_->session();
         
        // Build Cypher shortestPath query with depth limit
        std::string cypher =
            "MATCH path = shortestPath((src {id: $source_id})-[*1.." +
            std::to_string(max_depth) +
            "]-(tgt {id: $target_id})) "
            "RETURN nodes(path) AS path_nodes, relationships(path) AS path_rels, "
            "reduce(w=0.0, r IN relationships(path) | w + coalesce(r.weight, 1.0)) AS total_weight";
         
        // Build parameters
        neo4j::MapBuilder builder;
        builder.add_string("source_id", source_id);
        builder.add_string("target_id", target_id);
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        GraphPath path{};
        path.total_weight = 0.0;
         
        if (result.has_value()) {
            auto record = result->single();
            if (record.has_value()) {
                // Extract nodes
                auto nodes_val = record->get("path_nodes");
                if (nodes_val.has_value()) {
                    for (const auto& node_val : nodes_val->values()) {
                        GraphNode node;
                         
                        // Extract node properties
                        if (node_val.has_property("id")) {
                            node.id = node_val.get_property("id").as_string();
                        }
                         
                        // Extract labels (Neo4j nodes can have multiple labels)
                        auto labels = node_val.labels();
                        if (!labels.empty()) {
                            node.label = labels[0];
                        }
                         
                        // Extract all properties
                        for (const auto& [key, val] : node_val.properties()) {
                            // Convert neo4j::Value to Scalar
                            node.properties[key] = convert_neo4j_value_to_scalar(val);
                        }
                         
                        path.nodes.push_back(node);
                    }
                }
                 
                // Extract edges
                auto rels_val = record->get("path_rels");
                if (rels_val.has_value()) {
                    for (const auto& rel_val : rels_val->values()) {
                        GraphEdge edge;
                         
                        if (rel_val.has_property("id")) {
                            edge.id = rel_val.get_property("id").as_string();
                        }
                        edge.label = rel_val.type();
                         
                        // Extract weight if present
                        if (rel_val.has_property("weight")) {
                            edge.weight = rel_val.get_property("weight").as_double();
                        }
                         
                        // Extract properties
                        for (const auto& [key, val] : rel_val.properties()) {
                            edge.properties[key] = convert_neo4j_value_to_scalar(val);
                        }
                         
                        path.edges.push_back(edge);
                    }
                }
                 
                // Extract total weight
                auto weight_val = record->get("total_weight");
                if (weight_val.has_value()) {
                    path.total_weight = weight_val->as_double();
                }
            }
        }
         
        session.close();
        return Result<GraphPath>::ok(std::move(path));
    } catch (const std::exception& ex) {
        return Result<GraphPath>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j shortest_path failed: ") + ex.what()
        );
    }
#else
    return Result<GraphPath>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j shortest_path unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Traverse.
 * @param[in] start_id Identifier of the start.
 * @param[in] max_depth Input parameter.
 * @param[in] edge_labels Input parameter.
 * @return Return value.
 * @details Calls: err(), ok(), std::move().
 */
Result<std::vector<GraphNode>> Neo4jAdapter::traverse(
    const std::string& start_id,
    size_t max_depth,
    const std::vector<std::string>& edge_labels
) {
    if (!connected_) {
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::vector<GraphNode>>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (start_id.empty() || max_depth == 0) {
            return Result<std::vector<GraphNode>>::err(
                ErrorCode::INVALID_ARGUMENT,
                "start_id must not be empty and max_depth must be > 0"
            );
        }
         
        auto session = driver_->session();
         
        // Build Cypher traversal query (BFS pattern)
        std::string cypher = 
            "MATCH (start {id: $start_id})-[r*1.." +
            std::to_string(max_depth) + "]->(n) ";
         
        neo4j::MapBuilder builder;
        builder.add_string("start_id", start_id);
         
        // Add edge label filter if provided
        if (!edge_labels.empty()) {
            cypher += "WHERE type(r) IN $edge_labels ";
            neo4j::ListBuilder label_builder;
            for (const auto& label : edge_labels) {
                label_builder.add_string(label);
            }
            builder.add_list("edge_labels", label_builder.build());
        }
         
        cypher += "RETURN DISTINCT n";
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        std::vector<GraphNode> nodes;
         
        if (result.has_value()) {
            for (auto record : result.value()) {
                auto node_val = record.get("n");
                if (node_val.has_value()) {
                    GraphNode node;
                     
                    if (node_val->has_property("id")) {
                        node.id = node_val->get_property("id").as_string();
                    }
                     
                    auto labels = node_val->labels();
                    if (!labels.empty()) {
                        node.label = labels[0];
                    }
                     
                    for (const auto& [key, val] : node_val->properties()) {
                        node.properties[key] = convert_neo4j_value_to_scalar(val);
                    }
                     
                    nodes.push_back(node);
                }
            }
        }
         
        session.close();
        return Result<std::vector<GraphNode>>::ok(std::move(nodes));
    } catch (const std::exception& ex) {
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j traverse failed: ") + ex.what()
        );
    }
#else
    return Result<std::vector<GraphNode>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j traverse unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

Result<std::vector<GraphPath>> Neo4jAdapter::execute_graph_query(
    const std::string& query,
    const std::map<std::string, Scalar>& params
) {
    if (!connected_) {
        return Result<std::vector<GraphPath>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::vector<GraphPath>>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (query.empty()) {
            return Result<std::vector<GraphPath>>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Query must not be empty"
            );
        }
         
        auto session = driver_->session();
         
        // Build Neo4j parameter map from Scalar parameters
        neo4j::MapBuilder param_builder;
        for (const auto& [key, val] : params) {
            if (std::get_if<std::monostate>(&val)) {
                param_builder.add_null(key);
            } else if (auto* b = std::get_if<bool>(&val)) {
                param_builder.add_bool(key, *b);
            } else if (auto* i = std::get_if<int64_t>(&val)) {
                param_builder.add_int64(key, *i);
            } else if (auto* d = std::get_if<double>(&val)) {
                param_builder.add_double(key, *d);
            } else if (auto* s = std::get_if<std::string>(&val)) {
                param_builder.add_string(key, *s);
            } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                param_builder.add_bytes(key, *b);
            }
        }
         
        // Execute the provided Cypher query
        auto result = session.run(query, param_builder.build());
         
        std::vector<GraphPath> paths;
         
        if (result.has_value()) {
            for (auto record : result.value()) {
                GraphPath path{};
                path.total_weight = 0.0;
                 
                // Try to extract from "path" field (if query returns a path)
                if (record.has_column("path")) {
                    auto path_val = record.get("path");
                    if (path_val.has_value()) {
                        extract_path_from_neo4j_value(path_val.value(), path);
                    }
                }
                 
                // Try to extract from "nodes" and "relationships" fields
                if (record.has_column("nodes")) {
                    auto nodes_val = record.get("nodes");
                    if (nodes_val.has_value()) {
                        for (const auto& node_val : nodes_val->values()) {
                            GraphNode node;
                             
                            if (node_val.has_property("id")) {
                                node.id = node_val.get_property("id").as_string();
                            }
                             
                            auto labels = node_val.labels();
                            if (!labels.empty()) {
                                node.label = labels[0];
                            }
                             
                            for (const auto& [key, val] : node_val.properties()) {
                                node.properties[key] = convert_neo4j_value_to_scalar(val);
                            }
                             
                            path.nodes.push_back(node);
                        }
                    }
                }
                 
                if (record.has_column("relationships")) {
                    auto rels_val = record.get("relationships");
                    if (rels_val.has_value()) {
                        for (const auto& rel_val : rels_val->values()) {
                            GraphEdge edge;
                             
                            if (rel_val.has_property("id")) {
                                edge.id = rel_val.get_property("id").as_string();
                            }
                            edge.label = rel_val.type();
                             
                            if (rel_val.has_property("weight")) {
                                edge.weight = rel_val.get_property("weight").as_double();
                                path.total_weight += *edge.weight;
                            }
                             
                            for (const auto& [key, val] : rel_val.properties()) {
                                edge.properties[key] = convert_neo4j_value_to_scalar(val);
                            }
                             
                            path.edges.push_back(edge);
                        }
                    }
                }
                 
                paths.push_back(path);
            }
        }
         
        session.close();
        return Result<std::vector<GraphPath>>::ok(std::move(paths));
    } catch (const std::exception& ex) {
        return Result<std::vector<GraphPath>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j execute_graph_query failed: ") + ex.what()
        );
    }
#else
    return Result<std::vector<GraphPath>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j execute_graph_query unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief --------------------------------------------------------------------------- Document Adapter (Via Node Properties) ---------------------------------------------------------------------------
 * @param[in] collection Input parameter.
 * @param[in] doc Input parameter.
 * @return Return value.
 * @details Calls: err(), generate_id(), ok().
 */

Result<std::string> Neo4jAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::string>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (collection.empty()) {
            return Result<std::string>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Collection name must not be empty"
            );
        }
         
        auto session = driver_->session();
        const std::string doc_id = doc.id.empty() ? generate_id() : doc.id;
         
        // Build Cypher CREATE query with collection as label
        std::string cypher = "CREATE (n:" + collection + " {id: $id";
         
        // Add property placeholders
        for (const auto& [key, _] : doc.fields) {
            cypher += ", " + key + ": $" + key;
        }
        cypher += "}) RETURN n.id";
         
        // Build parameters
        neo4j::MapBuilder builder;
        builder.add_string("id", doc_id);
         
        for (const auto& [key, val] : doc.fields) {
            if (std::get_if<std::monostate>(&val)) {
                builder.add_null(key);
            } else if (auto* b = std::get_if<bool>(&val)) {
                builder.add_bool(key, *b);
            } else if (auto* i = std::get_if<int64_t>(&val)) {
                builder.add_int64(key, *i);
            } else if (auto* d = std::get_if<double>(&val)) {
                builder.add_double(key, *d);
            } else if (auto* s = std::get_if<std::string>(&val)) {
                builder.add_string(key, *s);
            } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                builder.add_bytes(key, *b);
            }
        }
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        if (!result.has_value()) {
            session.close();
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j insert_document: no result from CREATE query"
            );
        }
         
        session.close();
        return Result<std::string>::ok(doc_id);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j insert_document failed: ") + ex.what()
        );
    }
#else
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j insert_document unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Batch insert documents.
 * @param[in] collection Input parameter.
 * @param[in] docs Input parameter.
 * @return Return value.
 * @details Calls: err(), ok(), size().
 */
Result<size_t> Neo4jAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<size_t>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (collection.empty() || docs.empty()) {
            return Result<size_t>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Collection and docs must not be empty"
            );
        }
         
        auto session = driver_->session();
         
        // Build list of document maps for UNWIND
        neo4j::ListBuilder doc_list_builder;
         
        for (const auto& doc : docs) {
            neo4j::MapBuilder doc_builder;
             
            // Add document ID
            const std::string doc_id = doc.id.empty() ? generate_id() : doc.id;
            doc_builder.add_string("id", doc_id);
             
            // Add all fields
            for (const auto& [key, val] : doc.fields) {
                if (std::get_if<std::monostate>(&val)) {
                    doc_builder.add_null(key);
                } else if (auto* b = std::get_if<bool>(&val)) {
                    doc_builder.add_bool(key, *b);
                } else if (auto* i = std::get_if<int64_t>(&val)) {
                    doc_builder.add_int64(key, *i);
                } else if (auto* d = std::get_if<double>(&val)) {
                    doc_builder.add_double(key, *d);
                } else if (auto* s = std::get_if<std::string>(&val)) {
                    doc_builder.add_string(key, *s);
                } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                    doc_builder.add_bytes(key, *b);
                }
            }
             
            doc_list_builder.add_map(doc_builder.build());
        }
         
        // Build Cypher UNWIND+CREATE query
        std::string cypher = 
            "UNWIND $docs AS doc "
            "CREATE (n:" + collection + " {id: doc.id";
         
        // Add field placeholders (dynamically based on first doc)
        if (!docs.empty()) {
            for (const auto& [key, _] : docs[0].fields) {
                cypher += ", " + key + ": doc." + key;
            }
        }
         
        cypher += "}) "
            "RETURN COUNT(n) AS created";
         
        // Build parameters
        neo4j::MapBuilder param_builder;
        param_builder.add_list("docs", doc_list_builder.build());
         
        // Execute query
        auto result = session.run(cypher, param_builder.build());
         
        size_t created = 0;
         
        if (result.has_value()) {
            auto record = result->single();
            if (record.has_value()) {
                auto count_val = record->get("created");
                if (count_val.has_value()) {
                    created = static_cast<size_t>(count_val->as_int64());
                }
            }
        }
         
        session.close();
        return Result<size_t>::ok(created);
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j batch_insert_documents failed: ") + ex.what()
        );
    }
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j batch_insert_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

Result<std::vector<Document>> Neo4jAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<std::vector<Document>>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (collection.empty()) {
            return Result<std::vector<Document>>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Collection name must not be empty"
            );
        }
         
        auto session = driver_->session();
         
        // Build Cypher MATCH query
        std::string cypher = "MATCH (n:" + collection;
         
        neo4j::MapBuilder builder;
         
        if (!filter.empty()) {
            cypher += " {";
            bool first = true;
            for (const auto& [key, val] : filter) {
                if (!first) cypher += ", ";
                cypher += key + ": $" + key;
                 
                if (std::get_if<std::monostate>(&val)) {
                    builder.add_null(key);
                } else if (auto* b = std::get_if<bool>(&val)) {
                    builder.add_bool(key, *b);
                } else if (auto* i = std::get_if<int64_t>(&val)) {
                    builder.add_int64(key, *i);
                } else if (auto* d = std::get_if<double>(&val)) {
                    builder.add_double(key, *d);
                } else if (auto* s = std::get_if<std::string>(&val)) {
                    builder.add_string(key, *s);
                } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                    builder.add_bytes(key, *b);
                }
                first = false;
            }
            cypher += "}";
        }
         
        cypher += ") RETURN n LIMIT $limit";
        builder.add_int64("limit", static_cast<int64_t>(limit));
         
        // Execute query
        auto result = session.run(cypher, builder.build());
         
        std::vector<Document> documents;
         
        if (result.has_value()) {
            for (auto record : result.value()) {
                auto node_val = record.get("n");
                if (node_val.has_value()) {
                    Document doc;
                     
                    if (node_val->has_property("id")) {
                        doc.id = node_val->get_property("id").as_string();
                    }
                     
                    // Extract all properties as document fields
                    for (const auto& [key, val] : node_val->properties()) {
                        doc.fields[key] = convert_neo4j_value_to_scalar(val);
                    }
                     
                    documents.push_back(doc);
                }
            }
        }
         
        session.close();
        return Result<std::vector<Document>>::ok(std::move(documents));
    } catch (const std::exception& ex) {
        return Result<std::vector<Document>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j find_documents failed: ") + ex.what()
        );
    }
#else
    return Result<std::vector<Document>>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j find_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

Result<size_t> Neo4jAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (!driver_) {
            return Result<size_t>::err(
                ErrorCode::CONNECTION_ERROR,
                "Neo4j driver not initialized"
            );
        }
         
        if (collection.empty() || updates.empty()) {
            return Result<size_t>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Collection and updates must not be empty"
            );
        }
         
        auto session = driver_->session();
         
        // Build Cypher MATCH+SET query
        std::string cypher = "MATCH (n:" + collection;
         
        neo4j::MapBuilder param_builder;
         
        if (!filter.empty()) {
            cypher += " {";
            bool first = true;
            for (const auto& [key, val] : filter) {
                if (!first) cypher += ", ";
                cypher += key + ": $filter_" + key;
                 
                const std::string filter_key = "filter_" + key;
                if (std::get_if<std::monostate>(&val)) {
                    param_builder.add_null(filter_key);
                } else if (auto* b = std::get_if<bool>(&val)) {
                    param_builder.add_bool(filter_key, *b);
                } else if (auto* i = std::get_if<int64_t>(&val)) {
                    param_builder.add_int64(filter_key, *i);
                } else if (auto* d = std::get_if<double>(&val)) {
                    param_builder.add_double(filter_key, *d);
                } else if (auto* s = std::get_if<std::string>(&val)) {
                    param_builder.add_string(filter_key, *s);
                } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                    param_builder.add_bytes(filter_key, *b);
                }
                first = false;
            }
            cypher += "}";
        }
         
        cypher += ") SET n += $updates_map RETURN COUNT(n) AS updated";
         
        // Build updates map
        neo4j::MapBuilder updates_builder;
        for (const auto& [key, val] : updates) {
            if (std::get_if<std::monostate>(&val)) {
                updates_builder.add_null(key);
            } else if (auto* b = std::get_if<bool>(&val)) {
                updates_builder.add_bool(key, *b);
            } else if (auto* i = std::get_if<int64_t>(&val)) {
                updates_builder.add_int64(key, *i);
            } else if (auto* d = std::get_if<double>(&val)) {
                updates_builder.add_double(key, *d);
            } else if (auto* s = std::get_if<std::string>(&val)) {
                updates_builder.add_string(key, *s);
            } else if (auto* b = std::get_if<std::vector<uint8_t>>(&val)) {
                updates_builder.add_bytes(key, *b);
            }
        }
         
        param_builder.add_map("updates_map", updates_builder.build());
         
        // Execute query
        auto result = session.run(cypher, param_builder.build());
         
        size_t updated = 0;
         
        if (result.has_value()) {
            auto record = result->single();
            if (record.has_value()) {
                auto count_val = record->get("updated");
                if (count_val.has_value()) {
                    updated = static_cast<size_t>(count_val->as_int64());
                }
            }
        }
         
        session.close();
        return Result<size_t>::ok(updated);
    } catch (const std::exception& ex) {
        return Result<size_t>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j update_documents failed: ") + ex.what()
        );
    }
#else
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j update_documents unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief --------------------------------------------------------------------------- Transaction Adapter (Supported via Sessions) ---------------------------------------------------------------------------
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err(), generate_id(), lock(), ok().
 */

Result<std::string> Neo4jAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

    const std::string session_id = generate_id();
    {
        std::unique_lock<std::mutex> lock(session_mutex_);
        active_sessions_[session_id] = {session_id, nullptr, "active"};
    }

    return Result<std::string>::ok(session_id);
}

/**
 * @brief Commit transaction.
 * @param[in] transaction_id Identifier of the transaction.
 * @return Return value.
 * @details Calls: lock(), find(), end(), err(), ok().
 */
Result<bool> Neo4jAdapter::commit_transaction(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(session_mutex_);
    const auto it = active_sessions_.find(transaction_id);
    if (it == active_sessions_.end()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction not found"
        );
    }
    
#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (it->second.state != "active") {
            return Result<bool>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Transaction is not active; cannot commit"
            );
        }
         
        // Commit the transaction via the stored session
        if (it->second.neo4j_session) {
            auto* session_ptr = static_cast<neo4j::Session*>(it->second.neo4j_session);
            session_ptr->commit_transaction();
        }
         
        it->second.state = "committed";
        return Result<bool>::ok(true);
    } catch (const std::exception& ex) {
        it->second.state = "failed";
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j commit_transaction failed: ") + ex.what()
        );
    }
#else
    it->second.state = "committed";  // State tracking without real driver
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j commit_transaction unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

Result<bool> Neo4jAdapter::rollback_transaction(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(session_mutex_);
    const auto it = active_sessions_.find(transaction_id);
    if (it == active_sessions_.end()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction not found"
        );
    }
    
#ifdef THEMIS_CHIMERA_NEO4J
    try {
        if (it->second.state != "active") {
            return Result<bool>::err(
                ErrorCode::INVALID_ARGUMENT,
                "Transaction is not active; cannot rollback"
            );
        }
         
        // Rollback the transaction via the stored session
        if (it->second.neo4j_session) {
            auto* session_ptr = static_cast<neo4j::Session*>(it->second.neo4j_session);
            session_ptr->rollback_transaction();
        }
         
        it->second.state = "aborted";
        return Result<bool>::ok(true);
    } catch (const std::exception& ex) {
        it->second.state = "failed";
        return Result<bool>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j rollback_transaction failed: ") + ex.what()
        );
    }
#else
    it->second.state = "aborted";  // State tracking without real driver
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j rollback_transaction unavailable: library not compiled in. "
        "Rebuild with THEMIS_CHIMERA_NEO4J=ON to enable."
    );
#endif
}

/**
 * @brief Create savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<std::string> Neo4jAdapter::create_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    // Neo4j doesn't support savepoints; recommend transactions
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Savepoints not supported in Neo4j; use nested transactions"
    );
}

/**
 * @brief Rollback to savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> Neo4jAdapter::rollback_to_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Savepoints not supported in Neo4j"
    );
}

/**
 * @brief Release savepoint.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: err().
 */
Result<bool> Neo4jAdapter::release_savepoint(
    const std::string& /*transaction_id*/,
    const std::string& /*savepoint_name*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Savepoints not supported in Neo4j"
    );
}

/**
 * @brief Get transaction stats.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: ok(), std::move().
 */
Result<TransactionStats> Neo4jAdapter::get_transaction_stats(
    const std::string& /*transaction_id*/
) {
    TransactionStats stats = {};
    return Result<TransactionStats>::ok(std::move(stats));
}

/**
 * @brief Get transaction state.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Calls: ok(), std::move().
 */
Result<TransactionState> Neo4jAdapter::get_transaction_state(
    const std::string& /*transaction_id*/
) {
    TransactionState state = {};
    return Result<TransactionState>::ok(std::move(state));
}

// ---------------------------------------------------------------------------
// System Info Adapter
// ---------------------------------------------------------------------------

Result<SystemInfo> Neo4jAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "Neo4j";
    info.version = "0.1.0";
    info.build_info["database_version"] = "unknown";  // NOT IMPLEMENTED: Query via neo4j-cpp-driver requires THEMIS_CHIMERA_NEO4J
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> Neo4jAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.memory.total_bytes = 0;
    metrics.memory.used_bytes = 0;
    metrics.memory.available_bytes = 0;
    metrics.storage.total_bytes = 0;
    metrics.storage.used_bytes = 0;
    metrics.storage.available_bytes = 0;
    metrics.cpu.utilization_percent = 0.0;
    metrics.cpu.thread_count = 0;
    metrics.custom_metrics["total_queries"] = static_cast<int64_t>(0);
    metrics.custom_metrics["total_errors"] = static_cast<int64_t>(0);
    metrics.custom_metrics["avg_query_time_ms"] = 0.0;
    return Result<SystemMetrics>::ok(std::move(metrics));
}

bool Neo4jAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::GRAPH_OPERATIONS:
            return true;
        case Capability::TRANSACTIONS:
            return true;
        case Capability::CONNECTION_POOLING:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> Neo4jAdapter::get_capabilities() const {
    return {
        Capability::GRAPH_OPERATIONS,
        Capability::TRANSACTIONS,
        Capability::CONNECTION_POOLING
    };
}

// ---------------------------------------------------------------------------
// Private Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Generate id.
 * @return Return value.
 * @details Calls: utils::generate_uuid_v4().
 */
std::string Neo4jAdapter::generate_id() {
    return utils::generate_uuid_v4();
}

/**
 * @brief Is valid connection string.
 * @param[in] cs Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: find().
 */
bool Neo4jAdapter::is_valid_connection_string(const std::string& cs) {
    return cs.find("bolt://") == 0 ||
           cs.find("neo4j://") == 0 ||
           cs.find("bolt+s://") == 0 ||
           cs.find("neo4j+s://") == 0;
}

/**
 * @brief Mask credentials.
 * @param[in] cs Input parameter.
 * @return Return value.
 * @details Implements mask_credentials without additional internal calls.
 */
std::string Neo4jAdapter::mask_credentials(const std::string& cs) {
    // Hide password from connection string for logging/debugging.
    // Parses connection string format: proto://[user[:password]@]host[:port]/[database]
    // and replaces password portion with asterisks for safe logging.
     
    std::size_t auth_sep = cs.find("://");
    if (auth_sep != std::string::npos) {
        std::size_t at_pos = cs.find("@", auth_sep);
        if (at_pos != std::string::npos) {
            // Credentials present; mask them
            std::size_t cred_start = auth_sep + 3;
            std::size_t colon_pos = cs.find(":", cred_start);
            if (colon_pos != std::string::npos && colon_pos < at_pos) {
                // Username:password format
                return cs.substr(0, colon_pos + 1) + "***" + cs.substr(at_pos);
            } else {
                // Username only
                return cs.substr(0, cred_start) + "***" + cs.substr(at_pos);
            }
        }
    }
    return cs;
}

/**
 * @brief Scalar to cypher literal.
 * @param[in] param Input parameter.
 * @return Return value.
 * @details Implements scalar_to_cypher_literal without additional internal calls.
 */
std::string Neo4jAdapter::scalar_to_cypher_literal(const Scalar& scalar) {
    // Convert ThemisDB Scalar values to Cypher literal strings.
    // Primarily used for query construction and debugging.
    // Handles all scalar types with proper escaping and encoding.
     
    if (std::get_if<std::monostate>(&scalar)) {
        return "null";
    } else if (auto* b = std::get_if<bool>(&scalar)) {
        return *b ? "true" : "false";
    } else if (auto* i = std::get_if<int64_t>(&scalar)) {
        return std::to_string(*i);
    } else if (auto* d = std::get_if<double>(&scalar)) {
        return std::to_string(*d);
    } else if (auto* s = std::get_if<std::string>(&scalar)) {
        // Escape special characters in strings
        std::string escaped = *s;
        size_t pos = 0;
        while ((pos = escaped.find('\'', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\'");
            pos += 2;
        }
        return "'" + escaped + "'";
    } else if (auto* b = std::get_if<std::vector<uint8_t>>(&scalar)) {
        // Hex encode binary data
        std::ostringstream oss;
        oss << "0x";
        for (uint8_t byte : *b) {
            oss << std::hex << (static_cast<int>(byte) >> 4);
            oss << std::hex << (static_cast<int>(byte) & 0xF);
        }
        return oss.str();
    }
     
    return "null";  // Fallback for any unhandled types
}

#ifdef THEMIS_CHIMERA_NEO4J

Scalar Neo4jAdapter::convert_neo4j_value_to_scalar(const neo4j::Value& val) {
    // Convert neo4j::Value to Scalar type
    // This is used internally to map Neo4j query results to our scalar type system
     
    if (val.is_null()) {
        return Scalar{std::monostate{}};
    } else if (val.type() == neo4j::ValueType::BOOL) {
        return Scalar{val.as_bool()};
    } else if (val.type() == neo4j::ValueType::INT) {
        return Scalar{val.as_int64()};
    } else if (val.type() == neo4j::ValueType::FLOAT) {
        return Scalar{val.as_double()};
    } else if (val.type() == neo4j::ValueType::STRING) {
        return Scalar{val.as_string()};
    } else if (val.type() == neo4j::ValueType::BYTES) {
        auto bytes = val.as_bytes();
        return Scalar{std::vector<uint8_t>(bytes.begin(), bytes.end())};
    } else {
        // For complex types (lists, maps, nodes, relationships), return null
        return Scalar{std::monostate{}};
    }
}

void Neo4jAdapter::extract_path_from_neo4j_value(const neo4j::Value& path_val, GraphPath& path) {
    // Extract nodes and relationships from a Neo4j path value
    // This is used to convert Neo4j path results to our GraphPath format
     
    try {
        // Get nodes from the path
        auto nodes_list = path_val.get_field("nodes");
        if (nodes_list.has_value()) {
            for (const auto& node_val : nodes_list->values()) {
                GraphNode node;
                 
                if (node_val.has_property("id")) {
                    node.id = node_val.get_property("id").as_string();
                }
                 
                auto labels = node_val.labels();
                if (!labels.empty()) {
                    node.label = labels[0];
                }
                 
                for (const auto& [key, val] : node_val.properties()) {
                    node.properties[key] = convert_neo4j_value_to_scalar(val);
                }
                 
                path.nodes.push_back(node);
            }
        }
         
        // Get relationships from the path
        auto rels_list = path_val.get_field("relationships");
        if (rels_list.has_value()) {
            for (const auto& rel_val : rels_list->values()) {
                GraphEdge edge;
                 
                if (rel_val.has_property("id")) {
                    edge.id = rel_val.get_property("id").as_string();
                }
                edge.label = rel_val.type();
                 
                if (rel_val.has_property("weight")) {
                    edge.weight = rel_val.get_property("weight").as_double();
                    path.total_weight += *edge.weight;
                }
                 
                for (const auto& [key, val] : rel_val.properties()) {
                    edge.properties[key] = convert_neo4j_value_to_scalar(val);
                }
                 
                path.edges.push_back(edge);
            }
        }
    } catch (const std::exception& ex) {
        // If extraction fails, leave path partially initialized
        // This is safe since GraphPath has default-constructed members
    }
}

#endif

} // namespace chimera
