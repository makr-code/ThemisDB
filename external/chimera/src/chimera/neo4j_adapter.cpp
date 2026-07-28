/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            neo4j_adapter.cpp                                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:15:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     1258                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 566ec90912  2026-03-12  Changes before error encountered        ║
    • 29a0ee06c5  2026-03-12  feat(chimera): wire THEMIS_ENABLE_* driver blocks, add Co... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c12588b7a1  2026-02-28  feat(chimera): add Neo4j native graph database adapter ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file neo4j_adapter.cpp
 * @brief Neo4j native graph database adapter for CHIMERA Suite
 *
 * @details
 * Implements the IDatabaseAdapter interface for Neo4j 5.x. When the Bolt
 * driver is not available the adapter operates in an in-process simulation
 * mode backed by std::unordered_map, which is sufficient for unit testing
 * without a live Neo4j server.
 *
 * Production deployments should link against the Neo4j C++ driver (or an
 * HTTP client library) and replace the in-process simulation blocks with
 * real Bolt/HTTP API calls.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Keep graph adapter testable without live Neo4j infrastructure.
 * Activation: Active when THEMIS_ENABLE_NEO4J is not defined.
 * Production Delta: Uses in-memory simulation rather than Bolt/HTTP-backed graph operations.
 * Removal Plan: Keep as fallback for tests; production builds should enable THEMIS_ENABLE_NEO4J.
 *
 * @copyright MIT License
 */

#include "chimera/neo4j_adapter.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <sstream>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Real Neo4j HTTP Transactional API integration (cpp-httplib + nlohmann/json)
// ---------------------------------------------------------------------------
// When THEMIS_ENABLE_NEO4J is defined (set by CMake when cpp-httplib is
// found) the adapter performs real HTTP calls to the Neo4j Transactional
// Cypher endpoint (http://host:7474/db/neo4j/tx/commit).
// Without the macro the adapter falls back to the in-process simulation.
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_NEO4J
#include <httplib.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace chimera {

// ---------------------------------------------------------------------------
// Neo4jState – driver-specific state
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_NEO4J

/// Encode "user:pass" in Base64 for HTTP Basic Auth.
static std::string base64_encode(const std::string& input) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    for (size_t i = 0; i < input.size(); i += 3) {
        const unsigned char b0 = static_cast<unsigned char>(input[i]);
        const unsigned char b1 =
            (i + 1 < input.size())
                ? static_cast<unsigned char>(input[i + 1])
                : 0;
        const unsigned char b2 =
            (i + 2 < input.size())
                ? static_cast<unsigned char>(input[i + 2])
                : 0;
        out.push_back(table[b0 >> 2]);
        out.push_back(table[((b0 & 3) << 4) | (b1 >> 4)]);
        out.push_back((i + 1 < input.size()) ? table[((b1 & 15) << 2) | (b2 >> 6)] : '=');
        out.push_back((i + 2 < input.size()) ? table[b2 & 63] : '=');
    }
    return out;
}

/// Parse host and HTTP port from a Neo4j endpoint string.
///
/// For bolt://, neo4j://, and neo4j+s:// schemes the Bolt port is irrelevant
/// for the HTTP Transactional API; this function always maps those schemes to
/// Neo4j's default HTTP API port (7474) and strips any :port from the URL.
///
/// For bare host[:port] or http://host[:port] inputs the explicit port is
/// honoured, defaulting to 7474 when absent.
static std::pair<std::string, int> parse_neo4j_http_endpoint(
    const std::string& url)
{
    std::string rest = url;
    bool ignore_port = false; // true for bolt/neo4j schemes

    // Detect and strip scheme
    if (url.rfind("bolt://", 0) == 0) {
        rest = url.substr(7);
        ignore_port = true;
    } else if (url.rfind("neo4j://", 0) == 0) {
        rest = url.substr(8);
        ignore_port = true;
    } else if (url.rfind("neo4j+s://", 0) == 0) {
        rest = url.substr(10);
        ignore_port = true;
    } else if (url.rfind("http://", 0) == 0) {
        rest = url.substr(7);
    } else if (url.rfind("https://", 0) == 0) {
        rest = url.substr(8);
    }

    // Strip any path component after host[:port]
    const auto slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }

    // For bolt/neo4j schemes, strip the Bolt port and always use HTTP 7474.
    if (ignore_port) {
        const auto colon = rest.find(':');
        const std::string host =
            (colon != std::string::npos) ? rest.substr(0, colon) : rest;
        return {host, 7474};
    }

    // For http/https or bare host[:port], honour an explicit :port.
    const auto colon = rest.find(':');
    if (colon != std::string::npos) {
        return {rest.substr(0, colon), std::stoi(rest.substr(colon + 1))};
    }
    return {rest, 7474};
}

struct Neo4jAdapter::Neo4jState {
    std::string host;
    int         port;
    std::string auth_header;
    ConnectionPool<httplib::Client> pool;

    Neo4jState(const std::string& bolt_url,
               const std::string& username,
               const std::string& password,
               const ConnectionPoolConfig& pool_cfg)
        : host([&] { return parse_neo4j_http_endpoint(bolt_url).first; }())
        , port([&] { return parse_neo4j_http_endpoint(bolt_url).second; }())
        , auth_header(
              "Basic " + base64_encode(username + ":" + password))
        , pool(
            [this]() -> std::unique_ptr<httplib::Client> {
                auto cli = std::make_unique<httplib::Client>(host, port);
                cli->set_connection_timeout(30);
                cli->set_default_headers({
                    {"Authorization", auth_header},
                    {"Content-Type",  "application/json"},
                    {"Accept",        "application/json"}
                });
                return cli;
            },
            [](httplib::Client& cli) -> bool {
                // Health-check: GET / (200 means server up)
                auto res = cli.Get("/");
                return res != nullptr;
            },
            pool_cfg) {}

    /// Execute a Cypher statement and return the raw JSON response.
    json run_cypher(const std::string& cypher,
                    const json& params = json::object()) {
        json body = {
            {"statements", json::array({
                {{"statement", cypher}, {"parameters", params}}
            })}
        };
        auto conn = pool.acquire();
        if (!conn) {
            throw std::runtime_error("Neo4j pool exhausted");
        }
        auto res = conn->Post(
            "/db/neo4j/tx/commit",
            body.dump(),
            "application/json");
        pool.release(std::move(conn));

        if (!res || res->status != 200) {
            const std::string msg = res ? res->body : "no response";
            throw std::runtime_error("Neo4j HTTP error: " + msg);
        }
        return json::parse(res->body);
    }
};

#else

struct Neo4jAdapter::Neo4jState {};

#endif // THEMIS_ENABLE_NEO4J

// ---------------------------------------------------------------------------
// Auto-registration
// ---------------------------------------------------------------------------

namespace {
// Register Neo4jAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
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
// Construction / Destruction
// ---------------------------------------------------------------------------

Neo4jAdapter::Neo4jAdapter() = default;

Neo4jAdapter::~Neo4jAdapter() {
    if (connected_) {
        disconnect();
    }
    // neo4j_state_ destructor runs here.
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
            "Invalid Neo4j connection string: must start with "
            "bolt://, neo4j://, or neo4j+s://"
        );
    }

    connection_string_ = connection_string;

    // Extract credentials from options if provided.
    // The credentials are stored only as a presence flag – they are
    // intentionally NOT copied into connection_string_ or any field surfaced
    // by get_system_info() so that they cannot be leaked through logs or
    // memory inspection.
    auto user_it = options.find("username");
    auto pass_it = options.find("password");
    has_credentials_ = (user_it != options.end() && !user_it->second.empty()) ||
                       (pass_it != options.end() && !pass_it->second.empty());

    const std::string username =
        (user_it != options.end()) ? user_it->second : "neo4j";
    const std::string password =
        (pass_it != options.end()) ? pass_it->second : "";

#ifdef THEMIS_ENABLE_NEO4J
    try {
        neo4j_state_ = std::make_unique<Neo4jState>(
            connection_string, username, password, pool_config_);
    } catch (const std::exception& ex) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            std::string("Neo4j pool init failed: ") + ex.what()
        );
    }
#endif

    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> Neo4jAdapter::disconnect() {
#ifdef THEMIS_ENABLE_NEO4J
    neo4j_state_.reset();
#endif
    {
        std::lock_guard<std::mutex> lock(txn_mutex_);
        active_transactions_.clear();
    }
    connected_ = false;
    connection_string_.clear();
    has_credentials_ = false;
    return Result<bool>::ok(true);
}

bool Neo4jAdapter::is_connected() const {
    return connected_;
}

// ---------------------------------------------------------------------------
// IRelationalAdapter – not supported by Neo4j
// ---------------------------------------------------------------------------

Result<RelationalTable> Neo4jAdapter::execute_query(
    const std::string& /*query*/,
    const std::vector<Scalar>& /*params*/
) {
    return Result<RelationalTable>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support relational/SQL queries"
    );
}

Result<size_t> Neo4jAdapter::insert_row(
    const std::string& /*table_name*/,
    const RelationalRow& /*row*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support relational row insertion"
    );
}

Result<size_t> Neo4jAdapter::batch_insert(
    const std::string& /*table_name*/,
    const std::vector<RelationalRow>& /*rows*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support relational batch insertion"
    );
}

Result<QueryStatistics> Neo4jAdapter::get_query_statistics() const {
    QueryStatistics stats;
    stats.execution_time = std::chrono::microseconds(0);
    stats.rows_read = 0;
    stats.rows_returned = 0;
    stats.bytes_read = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// ---------------------------------------------------------------------------
// IVectorAdapter – not natively supported by Neo4j
// ---------------------------------------------------------------------------

Result<std::string> Neo4jAdapter::insert_vector(
    const std::string& /*collection*/,
    const Vector& /*vector*/
) {
    return Result<std::string>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support native vector insertion"
    );
}

Result<size_t> Neo4jAdapter::batch_insert_vectors(
    const std::string& /*collection*/,
    const std::vector<Vector>& /*vectors*/
) {
    return Result<size_t>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support native vector batch insertion"
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
        "Neo4j does not support native vector similarity search"
    );
}

Result<bool> Neo4jAdapter::create_index(
    const std::string& /*collection*/,
    size_t /*dimensions*/,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    return Result<bool>::err(
        ErrorCode::NOT_IMPLEMENTED,
        "Neo4j does not support vector index creation"
    );
}

// ---------------------------------------------------------------------------
// IGraphAdapter – primary capability
// ---------------------------------------------------------------------------

Result<std::string> Neo4jAdapter::insert_node(const GraphNode& node) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_ENABLE_NEO4J
    // Real path: CREATE (n:Node {id: $id, ...}) RETURN n.id
    try {
        const std::string nid =
            node.id.empty() ? generate_node_id() : node.id;
        json params = {{"id", nid}};
        for (const auto& kv : node.properties) {
            std::visit([&](const auto& v) { params[kv.first] = v; }, kv.second);
        }

        const std::string label =
            node.label.empty() ? "Node" : node.label;

        // Build CREATE statement with label and id only; extra properties
        // are set via SET to keep the statement generic.
        std::string cypher =
            "MERGE (n:`" + label + "` {id: $id}) SET n += $props RETURN n.id";
        json props_param = json::object();
        for (const auto& kv : node.properties) {
            std::visit(
                [&](const auto& v) { props_param[kv.first] = v; },
                kv.second);
        }
        json full_params = {{"id", nid}, {"props", props_param}};

        auto resp = neo4j_state_->run_cypher(cypher, full_params);
        if (resp.contains("errors") && !resp["errors"].empty()) {
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j insert_node: " +
                    resp["errors"][0].value("message", "unknown error")
            );
        }
        return Result<std::string>::ok(nid);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j insert_node failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    std::lock_guard<std::mutex> lock(graph_mutex_);

    GraphNode stored = node;
    if (stored.id.empty()) {
        stored.id = generate_node_id();
    } else if (node_store_.count(stored.id) > 0) {
        return Result<std::string>::err(
            ErrorCode::ALREADY_EXISTS,
            "Node with id '" + stored.id + "' already exists"
        );
    }

    const std::string inserted_id = stored.id;
    node_store_[inserted_id] = std::move(stored);
    adjacency_.emplace(inserted_id, std::vector<std::string>{});
    return Result<std::string>::ok(inserted_id);
#endif
}

Result<std::string> Neo4jAdapter::insert_edge(const GraphEdge& edge) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

#ifdef THEMIS_ENABLE_NEO4J
    // Real path: MATCH source and target nodes, then MERGE the relationship.
    try {
        const std::string eid =
            edge.id.empty() ? generate_edge_id() : edge.id;
        const std::string rel_type =
            edge.label.empty() ? "RELATES_TO" : edge.label;

        json params = {
            {"src",  edge.source_id},
            {"tgt",  edge.target_id},
            {"eid",  eid},
            // Always provide $weight so the Cypher parameter is never missing.
            // Use null when the edge has no weight so Neo4j stores a null value.
            {"weight", edge.weight.has_value()
                           ? json(*edge.weight)
                           : json(nullptr)}
        };

        const std::string cypher =
            "MATCH (a {id: $src}), (b {id: $tgt}) "
            "MERGE (a)-[r:`" + rel_type + "` {id: $eid}]->(b) "
            "SET r.weight = $weight "
            "RETURN r.id";

        auto resp = neo4j_state_->run_cypher(cypher, params);
        if (resp.contains("errors") && !resp["errors"].empty()) {
            return Result<std::string>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j insert_edge: " +
                    resp["errors"][0].value("message", "unknown error")
            );
        }
        return Result<std::string>::ok(eid);
    } catch (const std::exception& ex) {
        return Result<std::string>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j insert_edge failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    std::lock_guard<std::mutex> lock(graph_mutex_);

    if (node_store_.find(edge.source_id) == node_store_.end()) {
        return Result<std::string>::err(
            ErrorCode::NOT_FOUND,
            "Source node '" + edge.source_id + "' not found"
        );
    }
    if (node_store_.find(edge.target_id) == node_store_.end()) {
        return Result<std::string>::err(
            ErrorCode::NOT_FOUND,
            "Target node '" + edge.target_id + "' not found"
        );
    }

    GraphEdge stored = edge;
    if (stored.id.empty()) {
        stored.id = generate_edge_id();
    } else if (edge_store_.count(stored.id) > 0) {
        return Result<std::string>::err(
            ErrorCode::ALREADY_EXISTS,
            "Edge with id '" + stored.id + "' already exists"
        );
    }

    const std::string inserted_id = stored.id;
    adjacency_[stored.source_id].push_back(inserted_id);
    edge_store_[inserted_id] = std::move(stored);
    return Result<std::string>::ok(inserted_id);
#endif
}

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

#ifdef THEMIS_ENABLE_NEO4J
    // Real path: Cypher shortestPath query via HTTP Transactional API.
    try {
        const std::string cypher =
            "MATCH p=shortestPath((a {id: $src})-[*1.." +
            std::to_string(max_depth) +
            "]->(b {id: $tgt})) "
            "RETURN [n IN nodes(p) | n.id] AS node_ids, "
            "       [r IN relationships(p) | r.id] AS rel_ids, "
            "       length(p) AS path_len";

        json params = {{"src", source_id}, {"tgt", target_id}};
        auto resp = neo4j_state_->run_cypher(cypher, params);

        if (resp.contains("errors") && !resp["errors"].empty()) {
            return Result<GraphPath>::err(
                ErrorCode::NOT_FOUND,
                "Neo4j shortestPath: " +
                    resp["errors"][0].value("message", "no path found")
            );
        }

        GraphPath path;
        path.total_weight = 0.0;
        if (resp.contains("results") && !resp["results"].empty()) {
            auto& rows = resp["results"][0]["data"];
            if (!rows.empty()) {
                auto& row = rows[0]["row"];
                // row[0] = node_ids, row[1] = rel_ids, row[2] = path_len
                if (row.size() >= 1 && row[0].is_array()) {
                    for (auto& nid : row[0]) {
                        GraphNode n;
                        n.id = nid.get<std::string>();
                        path.nodes.push_back(std::move(n));
                    }
                }
                if (row.size() >= 2 && row[1].is_array()) {
                    for (auto& rid : row[1]) {
                        GraphEdge e;
                        e.id = rid.is_null() ? "" : rid.get<std::string>();
                        path.edges.push_back(std::move(e));
                    }
                }
            }
        }
        return Result<GraphPath>::ok(std::move(path));
    } catch (const std::exception& ex) {
        return Result<GraphPath>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j shortest_path failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    std::lock_guard<std::mutex> lock(graph_mutex_);

    if (node_store_.find(source_id) == node_store_.end()) {
        return Result<GraphPath>::err(
            ErrorCode::NOT_FOUND,
            "Source node '" + source_id + "' not found"
        );
    }
    if (node_store_.find(target_id) == node_store_.end()) {
        return Result<GraphPath>::err(
            ErrorCode::NOT_FOUND,
            "Target node '" + target_id + "' not found"
        );
    }

    return bfs_shortest_path(source_id, target_id, max_depth);
#endif
}

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

#ifdef THEMIS_ENABLE_NEO4J
    // Real path: BFS-style traversal via MATCH … -[*1..depth]-> RETURN nodes
    try {
        std::string rel_filter = "*1.." + std::to_string(max_depth);
        if (!edge_labels.empty()) {
            // Cypher type alternation: :`T1`|`T2`|... (colon only once).
            std::string type_expr;
            for (size_t i = 0; i < edge_labels.size(); ++i) {
                if (i > 0) type_expr += "|";
                type_expr += "`" + edge_labels[i] + "`";
            }
            rel_filter = ":" + type_expr + "*1.." + std::to_string(max_depth);
        }

        const std::string cypher =
            "MATCH (start {id: $sid})-[" + rel_filter + "]->(n) "
            "RETURN DISTINCT n.id AS nid, labels(n) AS lbls";

        json params = {{"sid", start_id}};
        auto resp = neo4j_state_->run_cypher(cypher, params);

        if (resp.contains("errors") && !resp["errors"].empty()) {
            return Result<std::vector<GraphNode>>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j traverse: " +
                    resp["errors"][0].value("message", "unknown error")
            );
        }

        std::vector<GraphNode> nodes;
        if (resp.contains("results") && !resp["results"].empty()) {
            for (auto& row_obj : resp["results"][0]["data"]) {
                auto& row = row_obj["row"];
                GraphNode n;
                n.id    = row[0].get<std::string>();
                if (row.size() > 1 && row[1].is_array() && !row[1].empty()) {
                    n.label = row[1][0].get<std::string>();
                }
                nodes.push_back(std::move(n));
            }
        }
        // Include the start node itself
        GraphNode start;
        start.id = start_id;
        nodes.insert(nodes.begin(), std::move(start));
        return Result<std::vector<GraphNode>>::ok(std::move(nodes));
    } catch (const std::exception& ex) {
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j traverse failed: ") + ex.what()
        );
    }
#else
    // Simulation path
    std::lock_guard<std::mutex> lock(graph_mutex_);

    if (node_store_.find(start_id) == node_store_.end()) {
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::NOT_FOUND,
            "Start node '" + start_id + "' not found"
        );
    }

    auto nodes = bfs_traverse(start_id, max_depth, edge_labels);
    return Result<std::vector<GraphNode>>::ok(std::move(nodes));
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
    if (query.empty()) {
        return Result<std::vector<GraphPath>>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Cypher query must not be empty"
        );
    }

#ifdef THEMIS_ENABLE_NEO4J
    // Real path: submit the Cypher statement to the Neo4j Transactional HTTP API.
    try {
        json cypher_params = json::object();
        for (const auto& kv : params) {
            std::visit(
                [&](const auto& v) { cypher_params[kv.first] = v; },
                kv.second);
        }

        auto resp = neo4j_state_->run_cypher(query, cypher_params);

        if (resp.contains("errors") && !resp["errors"].empty()) {
            return Result<std::vector<GraphPath>>::err(
                ErrorCode::INTERNAL_ERROR,
                "Neo4j execute_graph_query: " +
                    resp["errors"][0].value("message", "unknown error")
            );
        }

        // Return a single GraphPath containing node IDs found in results.
        GraphPath path;
        path.total_weight = 0.0;
        if (resp.contains("results") && !resp["results"].empty()) {
            for (auto& row_obj : resp["results"][0]["data"]) {
                auto& row = row_obj["row"];
                for (auto& cell : row) {
                    if (cell.is_string()) {
                        GraphNode n;
                        n.id = cell.get<std::string>();
                        path.nodes.push_back(std::move(n));
                    }
                }
            }
        }
        return Result<std::vector<GraphPath>>::ok({std::move(path)});
    } catch (const std::exception& ex) {
        return Result<std::vector<GraphPath>>::err(
            ErrorCode::INTERNAL_ERROR,
            std::string("Neo4j execute_graph_query failed: ") + ex.what()
        );
    }
#else
    // Simulation path: return a single path containing all nodes.
    std::lock_guard<std::mutex> lock(graph_mutex_);
    (void)params;
    GraphPath path;
    path.total_weight = 0.0;
    for (const auto& kv : node_store_) {
        path.nodes.push_back(kv.second);
    }
    for (const auto& kv : edge_store_) {
        path.edges.push_back(kv.second);
        if (kv.second.weight.has_value()) {
            path.total_weight += *kv.second.weight;
        }
    }
    return Result<std::vector<GraphPath>>::ok({std::move(path)});
#endif
}

// ---------------------------------------------------------------------------
// IDocumentAdapter – node property store
// ---------------------------------------------------------------------------

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

    std::lock_guard<std::mutex> lock(doc_mutex_);
    auto& store = document_store_[collection];

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
        stored.id = generate_node_id();
    }
    const std::string inserted_id = stored.id;
    store.push_back(std::move(stored));
    return Result<std::string>::ok(inserted_id);
}

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

    size_t inserted = 0;
    for (const auto& doc : docs) {
        auto result = insert_document(collection, doc);
        if (result.is_ok()) {
            ++inserted;
        }
    }
    return Result<size_t>::ok(inserted);
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
// ITransactionAdapter – ACID transactions supported
// ---------------------------------------------------------------------------

Result<std::string> Neo4jAdapter::begin_transaction(
    const TransactionOptions& /*options*/
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to Neo4j"
        );
    }

    const std::string txn_id = generate_transaction_id();
    std::lock_guard<std::mutex> lock(txn_mutex_);
    active_transactions_.insert(txn_id);
    return Result<std::string>::ok(txn_id);
}

Result<bool> Neo4jAdapter::commit_transaction(const std::string& transaction_id) {
    if (transaction_id.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found or already closed"
        );
    }
    active_transactions_.erase(it);
    return Result<bool>::ok(true);
}

Result<bool> Neo4jAdapter::rollback_transaction(const std::string& transaction_id) {
    if (transaction_id.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found or already closed"
        );
    }
    active_transactions_.erase(it);
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// ISystemInfoAdapter
// ---------------------------------------------------------------------------

Result<SystemInfo> Neo4jAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "Neo4j";
    info.version = "5.x";
    info.build_info["query_language"] = "Cypher";
#ifdef THEMIS_ENABLE_NEO4J
    info.build_info["protocol"] = "HTTP Transactional API (real)";
    info.build_info["driver"]   = "cpp-httplib";
#else
    info.build_info["protocol"] = "Bolt (simulation)";
    info.build_info["driver"]   = "cpp-httplib (simulation)";
#endif
    info.build_info["platform"]    = "Linux/Windows/macOS";
    info.configuration["endpoint"] = Scalar{connection_string_};
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> Neo4jAdapter::get_metrics() const {
    SystemMetrics metrics;
    metrics.memory.total_bytes        = 0;
    metrics.memory.used_bytes         = 0;
    metrics.memory.available_bytes    = 0;
    metrics.storage.total_bytes       = 0;
    metrics.storage.used_bytes        = 0;
    metrics.storage.available_bytes   = 0;
    metrics.cpu.utilization_percent   = 0.0;
#ifdef THEMIS_ENABLE_NEO4J
    // Report pool active connections as a proxy for driver thread usage.
    if (neo4j_state_) {
        const auto stats = neo4j_state_->pool.get_stats();
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

bool Neo4jAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::GRAPH_TRAVERSAL:
        case Capability::TRANSACTIONS:
        case Capability::BATCH_OPERATIONS:
        case Capability::SECONDARY_INDEXES:
            return true;
        default:
            return false;
    }
}

std::vector<Capability> Neo4jAdapter::get_capabilities() const {
    return {
        Capability::GRAPH_TRAVERSAL,
        Capability::TRANSACTIONS,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool Neo4jAdapter::is_valid_connection_string(
    const std::string& connection_string
) {
    return connection_string.rfind("bolt://", 0) == 0 ||
           connection_string.rfind("neo4j://", 0) == 0 ||
           connection_string.rfind("neo4j+s://", 0) == 0;
}

std::string Neo4jAdapter::generate_node_id() {
    std::ostringstream oss;
    oss << "neo4j_node_" << next_node_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::string Neo4jAdapter::generate_edge_id() {
    std::ostringstream oss;
    oss << "neo4j_edge_" << next_edge_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::string Neo4jAdapter::generate_transaction_id() {
    std::ostringstream oss;
    oss << "neo4j_txn_" << next_txn_id_.fetch_add(1, std::memory_order_relaxed);
    return oss.str();
}

std::vector<GraphNode> Neo4jAdapter::bfs_traverse(
    const std::string& start_id,
    size_t max_depth,
    const std::vector<std::string>& edge_labels
) const {
    // graph_mutex_ must be held by the caller.
    std::vector<GraphNode> visited_nodes;
    std::unordered_set<std::string> visited_ids;
    // BFS queue: (node_id, current_depth)
    std::deque<std::pair<std::string, size_t>> queue;

    queue.push_back({start_id, 0});
    visited_ids.insert(start_id);

    while (!queue.empty()) {
        auto [current_id, depth] = queue.front();
        queue.pop_front();

        auto node_it = node_store_.find(current_id);
        if (node_it != node_store_.end()) {
            visited_nodes.push_back(node_it->second);
        }

        if (depth >= max_depth) continue;

        auto adj_it = adjacency_.find(current_id);
        if (adj_it == adjacency_.end()) continue;

        for (const auto& edge_id : adj_it->second) {
            auto edge_it = edge_store_.find(edge_id);
            if (edge_it == edge_store_.end()) continue;

            const GraphEdge& edge = edge_it->second;

            // Apply edge label filter if specified
            if (!edge_labels.empty()) {
                bool label_match = false;
                for (const auto& lbl : edge_labels) {
                    if (edge.label == lbl) {
                        label_match = true;
                        break;
                    }
                }
                if (!label_match) continue;
            }

            const std::string& neighbor_id = edge.target_id;
            if (visited_ids.count(neighbor_id) == 0) {
                visited_ids.insert(neighbor_id);
                queue.push_back({neighbor_id, depth + 1});
            }
        }
    }

    return visited_nodes;
}

Result<GraphPath> Neo4jAdapter::bfs_shortest_path(
    const std::string& source_id,
    const std::string& target_id,
    size_t max_depth
) const {
    // graph_mutex_ must be held by the caller.
    if (source_id == target_id) {
        GraphPath path;
        auto node_it = node_store_.find(source_id);
        if (node_it != node_store_.end()) {
            path.nodes.push_back(node_it->second);
        }
        path.total_weight = 0.0;
        return Result<GraphPath>::ok(std::move(path));
    }

    // parent map: node_id -> (parent_node_id, edge_id used to reach it)
    std::unordered_map<std::string, std::pair<std::string, std::string>> parent;
    std::deque<std::pair<std::string, size_t>> queue;

    queue.push_back({source_id, 0});
    parent[source_id] = {"", ""};

    bool found = false;
    while (!queue.empty() && !found) {
        auto [current_id, depth] = queue.front();
        queue.pop_front();

        if (depth >= max_depth) continue;

        auto adj_it = adjacency_.find(current_id);
        if (adj_it == adjacency_.end()) continue;

        for (const auto& edge_id : adj_it->second) {
            auto edge_it = edge_store_.find(edge_id);
            if (edge_it == edge_store_.end()) continue;

            const std::string& neighbor_id = edge_it->second.target_id;
            if (parent.count(neighbor_id) == 0) {
                parent[neighbor_id] = {current_id, edge_id};
                if (neighbor_id == target_id) {
                    found = true;
                    break;
                }
                queue.push_back({neighbor_id, depth + 1});
            }
        }
    }

    if (!found) {
        return Result<GraphPath>::err(
            ErrorCode::NOT_FOUND,
            "No path found between '" + source_id + "' and '" + target_id + "'"
        );
    }

    // Reconstruct path from target back to source
    GraphPath path;
    path.total_weight = 0.0;
    std::vector<std::string> node_order;
    std::vector<std::string> edge_order;

    std::string current = target_id;
    while (current != source_id) {
        node_order.push_back(current);
        auto& [prev, edge_id] = parent[current];
        if (!edge_id.empty()) {
            edge_order.push_back(edge_id);
            auto edge_it = edge_store_.find(edge_id);
            if (edge_it != edge_store_.end() && edge_it->second.weight.has_value()) {
                path.total_weight += *edge_it->second.weight;
            }
        }
        current = prev;
    }
    node_order.push_back(source_id);

    // Reverse to get source -> target order
    std::reverse(node_order.begin(), node_order.end());
    std::reverse(edge_order.begin(), edge_order.end());

    for (const auto& nid : node_order) {
        auto node_it = node_store_.find(nid);
        if (node_it != node_store_.end()) {
            path.nodes.push_back(node_it->second);
        }
    }
    for (const auto& eid : edge_order) {
        auto edge_it = edge_store_.find(eid);
        if (edge_it != edge_store_.end()) {
            path.edges.push_back(edge_it->second);
        }
    }

    return Result<GraphPath>::ok(std::move(path));
}

bool Neo4jAdapter::document_matches(
    const Document& doc,
    const std::map<std::string, Scalar>& filter
) {
    for (const auto& kv : filter) {
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
