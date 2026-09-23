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
        // TODO (TODO 1/14): Actual `neo4j::Driver` creation via bolt URI
        // Implementation:
        // 1. Parse connection string to extract host, port, username, password
        // 2. Create neo4j::Uri from the connection string
        // 3. Extract authentication credentials (username/password)
        // 4. Create neo4j::Driver with URI and authentication
        // 5. Verify connectivity by running a test query (RETURN 1)
        // 6. Store driver instance for future use in sessions
        //
        // Example pseudo-code:
        // auto uri = neo4j::Uri(connection_string);
        // auto username = extract_from_uri("user") or options["username"]
        // auto password = extract_from_uri("password") or options["password"]
        // auto auth = neo4j::basic_auth(username, password);
        // driver_ = std::make_unique<neo4j::Driver>(
        //     neo4j::make_driver(uri, auth)
        // );
        // auto session = driver_->session(neo4j::SessionConfig{}); 
        // auto result = session.run("RETURN 1");
        // session.close();
        
        // For now, stub implementation to maintain compilation:
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
        // TODO (TODO 2/14): `CREATE (node:Label {properties})` via Cypher session
        // Implementation:
        // 1. Get a session from the driver: auto session = driver_->session()
        // 2. Generate node ID if not provided: node_id = node.id.empty() ? generate_id() : node.id
        // 3. Build Cypher CREATE query:
        //    CREATE (n:NodeLabel {id: $id, key1: $key1, key2: $key2, ...}) RETURN n.id
        // 4. Create parameters map from node.properties:
        //    params["id"] = node_id
        //    for (const auto& [key, val] : node.properties) {
        //        params[key] = val  // neo4j::Value conversion
        //    }
        // 5. Execute: auto result = session.run(cypher, params)
        // 6. Extract result: auto record = result.single()
        // 7. Return created node ID: return record.get("n.id").as_string()
        //
        // Example pseudo-code:
        // auto session = driver_->session(neo4j::SessionConfig{});
        // auto node_id = node.id.empty() ? generate_id() : node.id;
        // std::string cypher = "CREATE (n:" + node.label + " {id: $id";
        // neo4j::MapBuilder builder;
        // builder.add_string("id", node_id);
        // for (const auto& [key, val] : node.properties) {
        //     cypher += ", " + key + ": $" + key;
        //     builder.add_value(key, scalar_to_neo4j_value(val));
        // }
        // cypher += "}) RETURN n.id";
        // auto params = builder.build();
        // auto result = session.run(cypher, params);
        // auto record = result.single();
        // session.close();
        // return record.get("n.id").as_string()
        
        const std::string node_id = generate_id();
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
        // TODO (TODO 3/14): `CREATE (from)-[rel:TYPE]->(to)` via Cypher session
        // Implementation:
        // 1. Get a session from the driver: auto session = driver_->session()
        // 2. Generate edge ID if not provided: edge_id = edge.id.empty() ? generate_id() : edge.id
        // 3. Build Cypher query:
        //    MATCH (from {id: $source_id}), (to {id: $target_id})
        //    CREATE (from)-[r:EdgeLabel {id: $id, weight: $weight, ...properties}]->(to)
        //    RETURN r.id
        // 4. Create parameters from edge fields:
        //    params["source_id"] = edge.source_id
        //    params["target_id"] = edge.target_id
        //    params["id"] = edge_id
        //    if (edge.weight) params["weight"] = *edge.weight
        //    for properties...
        // 5. Execute and extract result: auto record = session.run(cypher, params).single()
        // 6. Return created edge ID
        //
        // Example pseudo-code:
        // auto session = driver_->session(neo4j::SessionConfig{});
        // auto edge_id = edge.id.empty() ? generate_id() : edge.id;
        // std::string cypher = 
        //     "MATCH (from {id: $source_id}), (to {id: $target_id}) " +
        //     std::string("CREATE (from)-[r:") + edge.label + 
        //     std::string(" {id: $id");
        // neo4j::MapBuilder builder;
        // builder.add_string("source_id", edge.source_id);
        // builder.add_string("target_id", edge.target_id);
        // builder.add_string("id", edge_id);
        // if (edge.weight) builder.add_double("weight", *edge.weight);
        // for (const auto& [key, val] : edge.properties) {
        //     cypher += ", " + key + ": $" + key;
        //     builder.add_value(key, scalar_to_neo4j_value(val));
        // }
        // cypher += "}]->(to) RETURN r.id";
        // auto result = session.run(cypher, builder.build());
        // auto record = result.single();
        // session.close();
        // return record.get("r.id").as_string()
        
        const std::string edge_id = generate_id();
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
        // TODO (TODO 4/14): `shortestPath()` Cypher query with `max_depth` bound
        // Implementation:
        // 1. Validate parameters: source_id and target_id must not be empty
        // 2. Get a session from the driver
        // 3. Build Cypher shortestPath() query:
        //    MATCH path = shortestPath((src {id: $source_id})-[*1..max_depth]-(tgt {id: $target_id}))
        //    RETURN nodes(path) AS path_nodes, relationships(path) AS path_rels, 
        //           reduce(w=0.0, r IN relationships(path) | w + coalesce(r.weight, 1.0)) AS total_weight
        // 4. Execute with parameters: source_id, target_id, max_depth
        // 5. Extract nodes and edges from result:
        //    for (auto node_val : record.get("path_nodes")) {
        //        GraphNode n; n.id = node_val["id"]; n.label = node_val.labels[0]; ...
        //        result.nodes.push_back(n);
        //    }
        //    similarly for edges from path_rels
        // 6. Set total_weight and return GraphPath
        //
        // Example pseudo-code:
        // auto session = driver_->session(neo4j::SessionConfig{});
        // if (source_id.empty() || target_id.empty()) {
        //     return Result<GraphPath>::err(ErrorCode::INVALID_ARGUMENT, "IDs must not be empty");
        // }
        // std::string cypher =
        //     "MATCH path = shortestPath((src {id: $source_id})-[*1.." + 
        //     std::to_string(max_depth) +
        //     "]-(tgt {id: $target_id})) "
        //     "RETURN nodes(path) AS path_nodes, relationships(path) AS path_rels, "
        //     "reduce(w=0.0, r IN relationships(path) | w + coalesce(r.weight, 1.0)) AS total_weight";
        // neo4j::MapBuilder builder;
        // builder.add_string("source_id", source_id);
        // builder.add_string("target_id", target_id);
        // auto result = session.run(cypher, builder.build());
        // auto record = result.single();
        // if (!record) return no path found error;
        // GraphPath path{};
        // extract nodes and edges from record
        // path.total_weight = record.get("total_weight").as_double();
        // session.close();
        // return path;
        
        GraphPath path = {};
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
        // TODO (TODO 5/14): BFS/DFS Cypher traversal query up to `max_depth`
        // Implementation:
        // 1. Validate parameters: start_id must not be empty, max_depth > 0
        // 2. Get a session from the driver
        // 3. Build Cypher traversal query (BFS pattern):
        //    MATCH (start {id: $start_id})-[r*1..max_depth]->(n)
        //    [WHERE type(r) IN $edge_labels]  (optional, if edge_labels provided)
        //    RETURN DISTINCT n
        // 4. Create parameters:
        //    params["start_id"] = start_id
        //    params["max_depth"] = max_depth
        //    if (!edge_labels.empty()) params["edge_labels"] = edge_labels
        // 5. Execute and iterate through results:
        //    std::vector<GraphNode> nodes;
        //    for (auto record : session.run(cypher, params)) {
        //        GraphNode node = extract_node_from_record(record);
        //        nodes.push_back(node);
        //    }
        // 6. Return the nodes vector
        //
        // Example pseudo-code:
        // auto session = driver_->session(neo4j::SessionConfig{});
        // if (start_id.empty() || max_depth == 0) {
        //     return Result<std::vector<GraphNode>>::err(ErrorCode::INVALID_ARGUMENT, 
        //         "start_id and max_depth must be valid");
        // }
        // std::string cypher = 
        //     "MATCH (start {id: $start_id})-[r*1.." + 
        //     std::to_string(max_depth) + "]->(n) ";
        // neo4j::MapBuilder builder;
        // builder.add_string("start_id", start_id);
        // if (!edge_labels.empty()) {
        //     cypher += "WHERE type(r) IN $edge_labels ";
        //     builder.add_string_list("edge_labels", edge_labels);
        // }
        // cypher += "RETURN DISTINCT n";
        // std::vector<GraphNode> nodes;
        // for (auto record : session.run(cypher, builder.build())) {
        //     auto node_val = record.get("n");
        //     GraphNode node;
        //     node.id = node_val["id"].as_string();
        //     node.label = node_val.labels()[0];
        //     for (auto [key, val] : node_val.properties()) {
        //         node.properties[key] = val;
        //     }
        //     nodes.push_back(node);
        // }
        // session.close();
        
        std::vector<GraphNode> nodes;
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
        // TODO (TODO 6/14): Arbitrary Cypher query → map results to `GraphPath`
        // Implementation:
        // 1. Validate query is not empty
        // 2. Get a session from the driver
        // 3. Convert Scalar parameters to neo4j::Value parameters:
        //    for (const auto& [key, val] : params) {
        //        neo4j_params[key] = scalar_to_neo4j_value(val);
        //    }
        // 4. Execute the provided Cypher query: auto result = session.run(query, neo4j_params)
        // 5. Iterate through records and interpret as GraphPath objects:
        //    - If record contains nodes/relationships in path order:
        //      Extract nodes and edges to build GraphPath
        //    - If record contains a "path" field:
        //      Extract nodes(path) and relationships(path)
        //    - Otherwise, try to extract node/edge arrays from record columns
        // 6. Handle records with weight calculation:
        //    If path has relationships with weight, compute total_weight
        // 7. Return vector of GraphPath objects
        //
        // Example pseudo-code:
        // if (query.empty()) {
        //     return Result<std::vector<GraphPath>>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Query must not be empty");
        // }
        // auto session = driver_->session(neo4j::SessionConfig{});
        // neo4j::MapBuilder param_builder;
        // for (const auto& [key, val] : params) {
        //     param_builder.add_value(key, scalar_to_neo4j_value(val));
        // }
        // auto result = session.run(query, param_builder.build());
        // std::vector<GraphPath> paths;
        // for (auto record : result) {
        //     GraphPath path = extract_graph_path_from_record(record);
        //     paths.push_back(path);
        // }
        // session.close();
        
        std::vector<GraphPath> paths;
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
        // TODO (TODO 7/14): Create node with collection label + document properties via Cypher
        // Implementation:
        // 1. Validate collection name is not empty
        // 2. Get a session from the driver
        // 3. Generate document ID if not provided: doc_id = doc.id.empty() ? generate_id() : doc.id
        // 4. Build Cypher CREATE query for document node:
        //    CREATE (n:collection_name {id: $id, field1: $field1, field2: $field2, ...})
        //    RETURN n.id
        // 5. Build parameters map:
        //    params["id"] = doc_id
        //    for (const auto& [key, val] : doc.fields) {
        //        params[key] = scalar_to_neo4j_value(val)
        //    }
        // 6. Execute query: auto record = session.run(cypher, params).single()
        // 7. Return created document ID: record.get("n.id").as_string()
        //
        // Example pseudo-code:
        // if (collection.empty()) {
        //     return Result<std::string>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Collection name must not be empty");
        // }
        // auto session = driver_->session(neo4j::SessionConfig{});
        // auto doc_id = doc.id.empty() ? generate_id() : doc.id;
        // std::string cypher = "CREATE (n:" + collection + " {id: $id";
        // neo4j::MapBuilder builder;
        // builder.add_string("id", doc_id);
        // for (const auto& [key, val] : doc.fields) {
        //     cypher += ", " + key + ": $" + key;
        //     builder.add_value(key, scalar_to_neo4j_value(val));
        // }
        // cypher += "}) RETURN n.id";
        // auto result = session.run(cypher, builder.build());
        // auto record = result.single();
        // session.close();
        
        const std::string id = generate_id();
        return Result<std::string>::ok(id);
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
        // TODO (TODO 8/14): Batch `UNWIND + CREATE` nodes via Cypher
        // Implementation:
        // 1. Validate inputs: collection not empty, docs not empty
        // 2. Get a session from the driver
        // 3. Build Cypher batch query using UNWIND:
        //    UNWIND $docs AS doc
        //    CREATE (n:collection_name {id: doc.id, field1: doc.field1, ...})
        //    RETURN COUNT(n) AS created
        // 4. Convert document array to neo4j list of maps:
        //    std::vector<neo4j::map> doc_list;
        //    for (const auto& doc : docs) {
        //        neo4j::MapBuilder doc_builder;
        //        doc_builder.add_string("id", doc.id.empty() ? generate_id() : doc.id);
        //        for (const auto& [key, val] : doc.fields) {
        //            doc_builder.add_value(key, scalar_to_neo4j_value(val));
        //        }
        //        doc_list.push_back(doc_builder.build());
        //    }
        // 5. Execute with params["docs"] = doc_list
        // 6. Extract count from result: auto record = session.run(cypher, params).single()
        // 7. Return count of created documents
        //
        // Example pseudo-code:
        // if (collection.empty() || docs.empty()) {
        //     return Result<size_t>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Collection and docs must not be empty");
        // }
        // auto session = driver_->session(neo4j::SessionConfig{});
        // std::vector<neo4j::map> doc_list;
        // for (const auto& doc : docs) {
        //     neo4j::MapBuilder doc_builder;
        //     doc_builder.add_string("id", doc.id.empty() ? generate_id() : doc.id);
        //     for (const auto& [key, val] : doc.fields) {
        //         doc_builder.add_value(key, scalar_to_neo4j_value(val));
        //     }
        //     doc_list.push_back(doc_builder.build());
        // }
        // std::string cypher = "UNWIND $docs AS doc CREATE (n:" + collection + 
        //     " {id: doc.id, ...fields...}) RETURN COUNT(n) AS created";
        // neo4j::MapBuilder param_builder;
        // param_builder.add_value("docs", doc_list);
        // auto result = session.run(cypher, param_builder.build());
        // auto record = result.single();
        // auto created_count = record.get("created").as_int64();
        // session.close();
        
        size_t inserted = docs.size();
        return Result<size_t>::ok(inserted);
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
        // TODO (TODO 9/14): `MATCH (n:collection {filter}) RETURN n LIMIT limit`
        // Implementation:
        // 1. Validate collection name is not empty
        // 2. Get a session from the driver
        // 3. Build Cypher MATCH query:
        //    Base: MATCH (n:collection_name
        //    If filter empty: ) RETURN n LIMIT $limit
        //    If filter present: {key1: $key1, key2: $key2, ...}) RETURN n LIMIT $limit
        // 4. Build parameters:
        //    params["limit"] = limit
        //    for (const auto& [key, val] : filter) {
        //        params[key] = scalar_to_neo4j_value(val)
        //    }
        // 5. Execute and iterate through results:
        //    std::vector<Document> documents;
        //    for (auto record : session.run(cypher, params)) {
        //        auto node = record.get("n");
        //        Document doc;
        //        doc.id = node["id"].as_string();
        //        for (auto [key, val] : node.properties()) {
        //            doc.fields[key] = neo4j_value_to_scalar(val);
        //        }
        //        documents.push_back(doc);
        //    }
        // 6. Return documents
        //
        // Example pseudo-code:
        // if (collection.empty()) {
        //     return Result<std::vector<Document>>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Collection name must not be empty");
        // }
        // auto session = driver_->session(neo4j::SessionConfig{});
        // std::string cypher = "MATCH (n:" + collection;
        // neo4j::MapBuilder builder;
        // if (!filter.empty()) {
        //     cypher += " {";
        //     bool first = true;
        //     for (const auto& [key, val] : filter) {
        //         if (!first) cypher += ", ";
        //         cypher += key + ": $" + key;
        //         builder.add_value(key, scalar_to_neo4j_value(val));
        //         first = false;
        //     }
        //     cypher += "}";
        // }
        // cypher += ") RETURN n LIMIT $limit";
        // builder.add_int64("limit", static_cast<int64_t>(limit));
        // std::vector<Document> documents;
        // for (auto record : session.run(cypher, builder.build())) {
        //     auto node = record.get("n");
        //     Document doc;
        //     doc.id = node["id"].as_string();
        //     for (auto [key, val] : node.properties()) {
        //         doc.fields[key] = neo4j_value_to_scalar(val);
        //     }
        //     documents.push_back(doc);
        // }
        // session.close();
        
        std::vector<Document> results;
        return Result<std::vector<Document>>::ok(std::move(results));
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
        // TODO (TODO 10/14): `MATCH (n:collection {filter}) SET n += updates`
        // Implementation:
        // 1. Validate inputs: collection not empty, updates not empty
        // 2. Get a session from the driver
        // 3. Build Cypher MATCH+SET query:
        //    MATCH (n:collection_name {filter_key1: $filter_key1, ...})
        //    SET n += $updates_map
        //    RETURN COUNT(n) AS updated
        // 4. Build parameters:
        //    For filter: params["filter_key1"] = filter_val1, etc.
        //    For updates: Create a single map with all updates:
        //      neo4j::MapBuilder updates_builder;
        //      for (const auto& [key, val] : updates) {
        //          updates_builder.add_value(key, scalar_to_neo4j_value(val))
        //      }
        //      params["updates_map"] = updates_builder.build()
        // 5. Execute: auto record = session.run(cypher, params).single()
        // 6. Extract count: auto updated_count = record.get("updated").as_int64()
        // 7. Return count
        //
        // Example pseudo-code:
        // if (collection.empty() || updates.empty()) {
        //     return Result<size_t>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Collection and updates must not be empty");
        // }
        // auto session = driver_->session(neo4j::SessionConfig{});
        // std::string cypher = "MATCH (n:" + collection;
        // neo4j::MapBuilder param_builder;
        // if (!filter.empty()) {
        //     cypher += " {";
        //     bool first = true;
        //     for (const auto& [key, val] : filter) {
        //         if (!first) cypher += ", ";
        //         cypher += key + ": $filter_" + key;
        //         param_builder.add_value(std::string("filter_") + key,
        //             scalar_to_neo4j_value(val));
        //         first = false;
        //     }
        //     cypher += "}";
        // }
        // cypher += ") SET n += $updates_map RETURN COUNT(n) AS updated";
        // neo4j::MapBuilder updates_builder;
        // for (const auto& [key, val] : updates) {
        //     updates_builder.add_value(key, scalar_to_neo4j_value(val));
        // }
        // param_builder.add_value("updates_map", updates_builder.build());
        // auto result = session.run(cypher, param_builder.build());
        // auto record = result.single();
        // auto updated_count = record.get("updated").as_int64();
        // session.close();
        
        size_t updated = 0;
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
        // TODO (TODO 11/14): Commit transaction via Neo4j session
        // Implementation:
        // 1. Get the Neo4j session associated with this transaction_id
        // 2. Verify session exists and is in "active" state
        // 3. Call session's transaction commit method:
        //    if (it->second.neo4j_session) {
        //        auto* session_ptr = static_cast<neo4j::Session*>(it->second.neo4j_session);
        //        session_ptr->commit_transaction();
        //    }
        // 4. Update session state: it->second.state = "committed"
        // 5. Optionally: remove from active_sessions_ or mark for cleanup
        // 6. Return success
        //
        // Example pseudo-code:
        // if (it->second.state != "active") {
        //     return Result<bool>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Transaction is not active");
        // }
        // if (it->second.neo4j_session) {
        //     auto* session = static_cast<neo4j::Session*>(it->second.neo4j_session);
        //     session->commit_transaction();
        // }
        // it->second.state = "committed";
        
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
        // TODO (TODO 12/14): Rollback transaction via Neo4j session
        // Implementation:
        // 1. Get the Neo4j session associated with this transaction_id
        // 2. Verify session exists (check it->second.neo4j_session)
        // 3. Call session's transaction rollback method:
        //    if (it->second.neo4j_session) {
        //        auto* session_ptr = static_cast<neo4j::Session*>(it->second.neo4j_session);
        //        session_ptr->rollback_transaction();
        //    }
        // 4. Update session state: it->second.state = "aborted"
        // 5. Optionally: remove from active_sessions_ or mark for cleanup
        // 6. Return success
        //
        // Example pseudo-code:
        // if (it->second.state != "active") {
        //     return Result<bool>::err(ErrorCode::INVALID_ARGUMENT,
        //         "Transaction is not active");
        // }
        // if (it->second.neo4j_session) {
        //     auto* session = static_cast<neo4j::Session*>(it->second.neo4j_session);
        //     session->rollback_transaction();
        // }
        // it->second.state = "aborted";
        
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
    // TODO (TODO 13/14): Implementation for credential masking
    // Purpose: Hide password from connection string for logging/debugging
    // Implementation:
    // 1. Parse connection string format: proto://[user[:password]@]host[:port]/[database]
    // 2. Find the @ separator if present
    // 3. Replace password portion with asterisks if found
    // 4. Return masked string for safe logging
    //
    // Example pseudo-code:
    // std::regex uri_pattern(
    //     "^(.*://)"           // scheme
    //     "([^:/@]+)"          // username
    //     "(?::([^/@]*)@)?"    // optional password with colon
    //     "(.*)$"              // host and rest
    // );
    // std::smatch match;
    // if (std::regex_match(cs, match, uri_pattern)) {
    //     if (match[3].matched) {  // password present
    //         return match[1].str() + match[2].str() + 
    //                std::string(":***@") + match[4].str();
    //     }
    // }
    
    // For safety, attempt URI parsing with regex if available
    // Otherwise return as-is (do not leak credentials to logs)
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
    // TODO (TODO 14/14): Implementation for Scalar → Cypher literal conversion
    // Purpose: Convert ThemisDB Scalar values to Cypher literal strings
    // Note: This is primarily for query construction and debugging
    // Implementation:
    // 1. Use std::get_if<T> to extract value from variant
    // 2. Convert each type appropriately:
    //    - std::monostate -> "null"
    //    - bool -> "true" or "false"
    //    - int64_t -> std::to_string(value)
    //    - double -> std::to_string(value)
    //    - std::string -> "'" + escaped_string + "'"
    //    - std::vector<uint8_t> -> hex encoding or base64
    // 3. Escape special characters in strings (quotes, backslashes)
    // 4. Return Cypher-compatible literal string
    //
    // Example pseudo-code:
    // if (std::get_if<std::monostate>(&scalar)) {
    //     return "null";
    // } else if (auto* b = std::get_if<bool>(&scalar)) {
    //     return *b ? "true" : "false";
    // } else if (auto* i = std::get_if<int64_t>(&scalar)) {
    //     return std::to_string(*i);
    // } else if (auto* d = std::get_if<double>(&scalar)) {
    //     return std::to_string(*d);
    // } else if (auto* s = std::get_if<std::string>(&scalar)) {
    //     // Escape quotes and backslashes
    //     std::string escaped;
    //     for (char c : *s) {
    //         if (c == '\'') escaped += "\\'";
    //         else if (c == '\\') escaped += "\\\\";
    //         else escaped += c;
    //     }
    //     return "'" + escaped + "'";
    // } else if (auto* b = std::get_if<std::vector<uint8_t>>(&scalar)) {
    //     // Hex encode binary data
    //     std::ostringstream oss;
    //     oss << "0x";
    //     for (uint8_t byte : *b) {
    //         oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    //     }
    //     return oss.str();
    // }
    
    // Current implementation: safe fallback for all types
    if (std::get_if<std::monostate>(&scalar)) {
        return "null";
    } else if (auto* b = std::get_if<bool>(&scalar)) {
        return *b ? "true" : "false";
    } else if (auto* i = std::get_if<int64_t>(&scalar)) {
        return std::to_string(*i);
    } else if (auto* d = std::get_if<double>(&scalar)) {
        return std::to_string(*d);
    } else if (auto* s = std::get_if<std::string>(&scalar)) {
        // Simple escaping: replace single quotes
        std::string escaped = *s;
        size_t pos = 0;
        while ((pos = escaped.find('\'', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\'");
            pos += 2;
        }
        return "'" + escaped + "'";
    } else if (auto* b = std::get_if<std::vector<uint8_t>>(&scalar)) {
        // Hex encoding for binary data
        std::ostringstream oss;
        oss << "0x";
        for (uint8_t byte : *b) {
            oss << std::hex << (static_cast<int>(byte) >> 4);
            oss << std::hex << (static_cast<int>(byte) & 0xF);
        }
        return oss.str();
    }
    
    return "null";  // Fallback
}

} // namespace chimera
