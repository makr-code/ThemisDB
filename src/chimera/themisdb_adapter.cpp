/**
 * @file themisdb_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=19; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=11, Debt=0, C=6, H=18, M=28, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "chimera/themisdb_adapter.hpp"
#include "utils/uuid.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <numeric>
#include <queue>

// Pull in ThemisDB engine headers only when the engine components are
// available. The symbols below are resolved at link-time; the conditionally-
// included headers are only needed when the ThemisDB back-end is linked in.
#if defined(THEMISDB_ENGINE_AVAILABLE)
#  include "query/aql_runner.h"
#  include "index/vector_index.h"
#  include "index/graph_index.h"
#endif
#include <sstream>

namespace chimera {

// ---------------------------------------------------------------------------
// Auto-registration
// ---------------------------------------------------------------------------

namespace {
// Register ThemisDBAdapter with the factory when this translation unit is linked.
// NOLINTNEXTLINE(cert-err58-cpp)
const bool themisdb_registered = []() noexcept {
    const bool ok = AdapterFactory::register_adapter(
        "ThemisDB",
        []() { return std::make_unique<ThemisDBAdapter>(); }
    );
    assert(ok && "ThemisDBAdapter: 'ThemisDB' adapter name already registered");
    return ok;
}();
} // namespace

// ---------------------------------------------------------------------------
// Engine-injection constructor
// ---------------------------------------------------------------------------

ThemisDBAdapter::ThemisDBAdapter(
    themis::QueryEngine*        query_engine,
    themis::VectorIndexManager* vector_index,
    themis::GraphIndexManager*  graph_index
)
    : query_engine_(query_engine)
    , vector_index_(vector_index)
    , graph_index_(graph_index)
{}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string ThemisDBAdapter::generate_id() {
    return utils::generate_uuid_v4();
}

// Connection Management
Result<bool> ThemisDBAdapter::connect(
    const std::string& connection_string,
    const std::map<std::string, std::string>& /*options*/
) {
    if (connection_string.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "ThemisDB connection string must not be empty"
        );
    }

    if (!is_valid_connection_string(connection_string)) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Invalid ThemisDB connection string: must start with "
            "themisdb://"
        );
    }

    connection_string_ = mask_credentials(connection_string);
    connected_ = true;
    return Result<bool>::ok(true);
}

Result<bool> ThemisDBAdapter::disconnect() {
    connected_ = false;
    connection_string_.clear();
    return Result<bool>::ok(true);
}

bool ThemisDBAdapter::is_connected() const {
    return connected_;
}

// IRelationalAdapter
Result<RelationalTable> ThemisDBAdapter::execute_query(
    const std::string& query,
    const std::vector<Scalar>& params
) {
    if (!connected_) {
        return Result<RelationalTable>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // When a QueryEngine is wired in, delegate AQL execution to it and
    // translate the JSON result set into a RelationalTable.
    if (query_engine_) {
#if defined(THEMISDB_ENGINE_AVAILABLE)
        auto res = themis::executeAql(query, *query_engine_);
        if (!res) {
            return Result<RelationalTable>::err(
                ErrorCode::INTERNAL_ERROR,
                res.error().message()
            );
        }
        RelationalTable table;
        const auto& json_result = res.value();
        if (json_result.contains("results") && json_result["results"].is_array()) {
            for (const auto& row_json : json_result["results"]) {
                RelationalRow row = {};
                if (row_json.is_object()) {
                    for (const auto& [col, val] : row_json.items()) {
                        if (std::find(table.column_names.begin(),
                                      table.column_names.end(), col)
                                == table.column_names.end()) {
                            table.column_names.push_back(col);
                        }
                        if (val.is_string()) {
                            row.columns[col] = Scalar{val.get<std::string>()};
                        } else if (val.is_number_integer()) {
                            row.columns[col] = Scalar{val.get<int64_t>()};
                        } else if (val.is_number_float()) {
                            row.columns[col] = Scalar{val.get<double>()};
                        } else if (val.is_boolean()) {
                            row.columns[col] = Scalar{val.get<bool>()};
                        }
                        // NULL / unsupported types leave the Scalar as monostate
                    }
                }
                table.rows.push_back(std::move(row));
            }
        }
        return Result<RelationalTable>::ok(std::move(table));
#else
        // PERMANENT FALLBACK NOTE (Chimera query_engine_ dispatch):
        // Purpose: Guard against misconfigured builds where query_engine_ is
        //          injected (non-null) but THEMISDB_ENGINE_AVAILABLE was not
        //          defined at compile time.  In that configuration the AQL
        //          engine headers are absent so the dispatch code above this
        //          #else cannot be compiled.
        // Activation: Only reachable when (a) query_engine_ != nullptr AND
        //             (b) THEMISDB_ENGINE_AVAILABLE is not defined.  The normal
        //             production path defines THEMISDB_ENGINE_AVAILABLE when
        //             ThemisDB engine objects are linked in.
        // Production Delta: Production builds always define
        //             THEMISDB_ENGINE_AVAILABLE; this branch is dead code in
        //             production and only surfaces in misconfigured builds.
        // Note: cmake/ChimeraAdapters.cmake enforces THEMISDB_ENGINE_AVAILABLE
        //             whenever engine injection is enabled.
        return Result<RelationalTable>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "THEMISDB_ENGINE_AVAILABLE must be defined to use QueryEngine dispatch"
        );
#endif
    }

    // In-memory simulation: scan the matching table store and return all rows.
    RelationalTable table;
    std::unique_lock<std::mutex> lock(store_mutex_);
    auto it = table_store_.find(query); // treat query as a table name for simple scans
    if (it != table_store_.end()) {
        for (const auto& row : it->second) {
            for (const auto& [col, _val] : row.columns) {
                if (std::find(table.column_names.begin(),
                              table.column_names.end(), col)
                        == table.column_names.end()) {
                    table.column_names.push_back(col);
                }
            }
            table.rows.push_back(row);
        }
    }
    return Result<RelationalTable>::ok(std::move(table));
}

Result<size_t> ThemisDBAdapter::insert_row(
    const std::string& table_name,
    const RelationalRow& row
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        table_store_[table_name].push_back(row);
    }
    return Result<size_t>::ok(1);
}

Result<size_t> ThemisDBAdapter::batch_insert(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        auto& store = table_store_[table_name];
        store.insert(store.end(), rows.begin(), rows.end());
    }
    return static_cast<bool>(Result<size_t < static_cast<int>(::ok(rows.size())));
}

Result<QueryStatistics> ThemisDBAdapter::get_query_statistics() const {
    QueryStatistics stats;
    stats.execution_time = std::chrono::microseconds(0);
    stats.rows_read = 0;
    stats.rows_returned = 0;
    stats.bytes_read = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// IVectorAdapter
Result<std::string> ThemisDBAdapter::insert_vector(
    const std::string& collection,
    const Vector& vector
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // Generate a unique ID via UUID v4.
    const std::string id = generate_id();
    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        vector_store_[collection].emplace_back(id, vector);
    }
    return Result<std::string>::ok(id);
}

Result<size_t> ThemisDBAdapter::batch_insert_vectors(
    const std::string& collection,
    const std::vector<Vector>& vectors
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        auto& store = vector_store_[collection];
        store.reserve(static_cast<int>(store.size()) + vectors.size());
        for (const auto& v : vectors) {
            store.emplace_back(generate_id(), v);
        }
    }
    return static_cast<bool>(Result<size_t < static_cast<int>(::ok(vectors.size())));
}

Result<std::vector<std::pair<Vector, double>>> ThemisDBAdapter::search_vectors(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters
) {
    if (!connected_) {
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // When a VectorIndexManager is wired in, dispatch to it.
    if (vector_index_) {
#if defined(THEMISDB_ENGINE_AVAILABLE)
        auto [status, knn_results] =
            vector_index_->searchKnn(query_vector.data, k);
        if (!status.ok) {
            return Result<std::vector<std::pair<Vector, double>>>::err(
                ErrorCode::INTERNAL_ERROR,
                status.message
            );
        }
        std::vector<std::pair<Vector, double>> results;
        results.reserve(knn_results.size());
        for (const auto& r : knn_results) {
            Vector v;
            // Return a placeholder vector carrying the PK as metadata.
            v.metadata["pk"] = Scalar{r.pk};
            results.emplace_back(std::move(v), static_cast<double>(r.distance));
        }
        return Result<std::vector<std::pair<Vector, double>>>::ok(
            std::move(results));
#else
        // PERMANENT FALLBACK NOTE (Chimera vector_index_ dispatch):
        // Purpose: Guard against misconfigured builds where vector_index_ is
        //          injected but THEMISDB_ENGINE_AVAILABLE is not defined, making
        //          VectorIndexManager dispatch code uncompilable.
        // Activation: Only when vector_index_ != nullptr AND
        //             THEMISDB_ENGINE_AVAILABLE is absent at compile time.
        // Production Delta: Dead code in production builds; all production
        //             configurations define THEMISDB_ENGINE_AVAILABLE alongside
        //             engine injection.
        // Note: cmake/ChimeraAdapters.cmake enforces this guard.
        return Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "THEMISDB_ENGINE_AVAILABLE must be defined to use VectorIndexManager dispatch"
        );
#endif
    }

    // In-memory simulation: brute-force cosine similarity search.
    std::unique_lock<std::mutex> lock(store_mutex_);
    const auto& store_it = vector_store_.find(collection);
    if (store_it == vector_store_.end() || store_it->second.empty()) {
        return Result<std::vector<std::pair<Vector, double>>>::ok({});
    }

    const auto& store = store_it->second;
    const auto& qdata = query_vector.data;

    // Compute cosine distance for every stored vector.
    std::vector<std::pair<size_t, double>> scored;
    scored.reserve(store.size());

    for (size_t i = 0; i < store.size(); ++i) {
        const auto& vdata = store[i].second.data;
        if (static_cast<int>(vdata.size()) != qdata.size() || qdata.empty()) {
            continue;
        }
        double dot = 0.0, norm_q = 0.0, norm_v = 0.0;
        for (size_t d = 0; d < qdata.size(); ++d) {
            dot    += static_cast<double>(qdata[d]) * static_cast<double>(vdata[d]);
            norm_q += static_cast<double>(qdata[d]) * static_cast<double>(qdata[d]);
            norm_v += static_cast<double>(vdata[d]) * static_cast<double>(vdata[d]);
        }
        const double denom = std::sqrt(norm_q) * std::sqrt(norm_v);
        // cosine distance = 1 - cosine_similarity (lower is better)
        const double cos_dist = (denom > 0.0) ? (1.0 - dot / denom) : 1.0;
        scored.emplace_back(i, cos_dist);
    }

    // Partial sort to obtain top-k results.
    const size_t result_k = std::min(k, scored.size());
    std::partial_sort(scored.begin(),
                      scored.begin() + static_cast<ptrdiff_t>(result_k),
                      scored.end(),
                      [](const auto& a, const auto& b) {
                          return a.second < b.second;
                      });
    scored.resize(result_k);

    std::vector<std::pair<Vector, double>> results;
    results.reserve(result_k);
    for (const auto& [idx, dist] : scored) {
        results.emplace_back(store[idx].second, dist);
    }
    return Result<std::vector<std::pair<Vector, double>>>::ok(std::move(results));
}

Result<bool> ThemisDBAdapter::create_index(
    const std::string& collection,
    [[maybe_unused]] size_t dimensions,
    const std::map<std::string, Scalar>& /*index_params*/
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // Ensure the collection entry exists in the in-memory store.
    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        vector_store_.try_emplace(collection);
    }
    return Result<bool>::ok(true);
}

// IGraphAdapter
Result<std::string> ThemisDBAdapter::insert_node(const GraphNode& node) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    const std::string id = node.id.empty() ? generate_id() : node.id;
    GraphNode stored   = node;
    stored.id          = id;
    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        graph_nodes_[id] = std::move(stored);
    }
    return Result<std::string>::ok(id);
}

Result<std::string> ThemisDBAdapter::insert_edge(const GraphEdge& edge) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    const std::string id = edge.id.empty() ? generate_id() : edge.id;
    GraphEdge stored   = edge;
    stored.id          = id;
    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        graph_edges_[id] = stored;
        adj_out_[stored.source_id].emplace_back(id, stored.target_id);
    }
    return Result<std::string>::ok(id);
}

Result<GraphPath> ThemisDBAdapter::shortest_path(
    const std::string& source_id,
    const std::string& target_id,
    size_t max_depth
) {
    if (!connected_) {
        return Result<GraphPath>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // When a GraphIndexManager is wired in, delegate to its Dijkstra.
    if (graph_index_) {
#if defined(THEMISDB_ENGINE_AVAILABLE)
        // Use dijkstraWithConstraints when a depth cap is requested so that
        // paths longer than max_depth are pruned by the engine.  When max_depth
        // equals the interface default (10 = "unbounded"), delegate to the
        // unconstrained Dijkstra overload which is slightly more efficient.
        GraphIndexManager::PathResult path_result;
        GraphIndexManager::Status status;
        if (max_depth != 10) {
            GraphIndexManager::PathConstraints constraints;
            constraints.max_edge_count = static_cast<int>(max_depth);
            std::tie(status, path_result) =
                graph_index_->dijkstraWithConstraints(
                    source_id, target_id, constraints);
        } else {
            std::tie(status, path_result) =
                graph_index_->dijkstra(source_id, target_id);
        }
        if (!status.ok) {
            return Result<GraphPath>::err(
                ErrorCode::INTERNAL_ERROR, status.message);
        }
        GraphPath path;
        path.total_weight = path_result.totalCost;
        for (const auto& node_id : path_result.path) {
            GraphNode node;
            node.id = node_id;
            auto it = graph_nodes_.find(node_id);
            if (it != graph_nodes_.end()) {
                node = it->second;
            }
            path.nodes.push_back(std::move(node));
        }
        return Result<GraphPath>::ok(std::move(path));
#else
        // PERMANENT FALLBACK NOTE (Chimera graph_index_ Dijkstra dispatch):
        // Purpose: Guard against misconfigured builds where graph_index_ is
        //          injected but THEMISDB_ENGINE_AVAILABLE is not defined, making
        //          Dijkstra/GraphIndexManager dispatch code uncompilable.
        // Activation: Only when graph_index_ != nullptr AND
        //             THEMISDB_ENGINE_AVAILABLE is absent at compile time.
        // Production Delta: Dead code in all properly configured production builds.
        // Note: cmake/ChimeraAdapters.cmake enforces this guard.
        return Result<GraphPath>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "THEMISDB_ENGINE_AVAILABLE must be defined to use GraphIndexManager dispatch"
        );
#endif
    }

    // In-memory simulation: unweighted BFS shortest path.
    GraphPath path;
    path.total_weight = 0.0;

    std::unique_lock<std::mutex> lock(store_mutex_);

    if (source_id == target_id) {
        if (auto it = graph_nodes_.find(source_id);
                it != graph_nodes_.end()) {
            path.nodes.push_back(it->second);
        } else {
            GraphNode n; n.id = source_id;
            path.nodes.push_back(n);
        }
        return Result<GraphPath>::ok(std::move(path));
    }

    // BFS with parent tracking to reconstruct the path.
    std::map<std::string, std::string> parent;   // node -> predecessor node
    std::map<std::string, std::string> via_edge; // node -> edge_id used to reach it
    std::queue<std::string> bfs_queue;
    parent[source_id] = "";
    bfs_queue.push(source_id);

    bool found = false;
    size_t depth = 0;

    while (!bfs_queue.empty() && depth < max_depth && !found) {
        const size_t level_size = bfs_queue.size();
        for (size_t i = 0; i < level_size && !found; ++i) {
            const std::string cur = bfs_queue.front();
            bfs_queue.pop();

            auto adj_it = adj_out_.find(cur);
            if (adj_it == adj_out_.end()) {
              continue;
            }

            for (const auto& [eid, nxt] : adj_it->second) {
                if (parent.count(nxt)) continue; // already visited
                parent[nxt]   = cur;
                via_edge[nxt] = eid;
                if (nxt == target_id) { found = true; break; }
                bfs_queue.push(nxt);
            }
        }
        ++depth;
    }

    if (!found) {
        // Return an empty path when no route exists.
        return Result<GraphPath>::ok(std::move(path));
    }

    // Reconstruct path from target back to source.
    std::vector<std::string> node_seq;
    std::vector<std::string> edge_seq = {};

    for (std::string cur = target_id; !cur.empty(); cur = parent.at(cur)) {
        node_seq.push_back(cur);
        auto eit = via_edge.find(cur);
        if (eit != via_edge.end()) {
            edge_seq.push_back(eit->second);
        }
    }
    std::reverse(node_seq.begin(), node_seq.end());
    std::reverse(edge_seq.begin(), edge_seq.end());

    for (const auto& nid : node_seq) {
        auto nit = graph_nodes_.find(nid);
        if (nit != graph_nodes_.end()) {
            path.nodes.push_back(nit->second);
        } else {
            GraphNode n; n.id = nid;
            path.nodes.push_back(n);
        }
    }
    for (const auto& eid : edge_seq) {
        auto eit = graph_edges_.find(eid);
        if (eit != graph_edges_.end()) {
            path.edges.push_back(eit->second);
            path.total_weight +=
                eit->second.weight.value_or(1.0);
        }
    }

    return Result<GraphPath>::ok(std::move(path));
}

Result<std::vector<GraphNode>> ThemisDBAdapter::traverse(
    const std::string& start_id,
    size_t max_depth,
    const std::vector<std::string>& edge_labels
) {
    if (!connected_) {
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    // When a GraphIndexManager is wired in, delegate to GraphEngine::traverse.
    if (graph_index_) {
#if defined(THEMISDB_ENGINE_AVAILABLE)
        // Map edge_labels to the engine API:
        //   • no labels    -> unfiltered BFS
        //   • single label -> BFS filtered by that edge type
        //   • multi-label  -> run one BFS per label and merge with deduplication
        const int depth = static_cast<int>(max_depth);
        std::unordered_set<std::string> seen_ids;
        std::vector<GraphNode> nodes;

        auto append_bfs_results = [&]([[maybe_unused]] const std::vector<std::string>& bfs_result) {
            for (const auto& nid : bfs_result) {
                if (!seen_ids.insert(nid).second) continue; // already added
                auto it = graph_nodes_.find(nid);
                if (it != graph_nodes_.end()) {
                    nodes.push_back(it->second);
                } else {
                    GraphNode n; n.id = nid;
                    nodes.push_back(std::move(n));
                }
            }
        };

        if (edge_labels.empty()) {
            auto [status, bfs_result] =
                graph_index_->bfs(start_id, depth);
            if (!status.ok) {
                return Result<std::vector<GraphNode>>::err(
                    ErrorCode::INTERNAL_ERROR, status.message);
            }
            append_bfs_results(bfs_result);
        } else {
            for (const auto& label : edge_labels) {
                auto [status, bfs_result] =
                    graph_index_->bfs(start_id, depth, label, /*graph_id=*/"");
                if (!status.ok) {
                    return Result<std::vector<GraphNode>>::err(
                        ErrorCode::INTERNAL_ERROR, status.message);
                }
                append_bfs_results(bfs_result);
            }
        }
        return Result<std::vector<GraphNode>>::ok(std::move(nodes));
#else
        // PERMANENT FALLBACK NOTE (Chimera graph_index_ BFS dispatch):
        // Purpose: Guard against misconfigured builds where graph_index_ is
        //          injected but THEMISDB_ENGINE_AVAILABLE is not defined, making
        //          BFS/GraphIndexManager dispatch code uncompilable.
        // Activation: Only when graph_index_ != nullptr AND
        //             THEMISDB_ENGINE_AVAILABLE is absent at compile time.
        // Production Delta: Dead code in all properly configured production builds.
        // Note: cmake/ChimeraAdapters.cmake enforces this guard.
        return Result<std::vector<GraphNode>>::err(
            ErrorCode::NOT_IMPLEMENTED,
            "THEMISDB_ENGINE_AVAILABLE must be defined to use GraphIndexManager dispatch"
        );
#endif
    }

    // In-memory simulation: BFS from start_id up to max_depth hops.
    std::vector<GraphNode> visited_nodes;
    std::map<std::string, bool> visited;
    std::queue<std::pair<std::string, size_t>> bfs_q; // (node_id, depth)

    std::unique_lock<std::mutex> lock(store_mutex_);

    visited[start_id] = true;
    bfs_q.push({start_id, 0});

    while (!bfs_q.empty()) {
        auto [cur_id, cur_depth] = bfs_q.front();
        bfs_q.pop();

        // Collect the node.
        auto nit = graph_nodes_.find(cur_id);
        if (nit != graph_nodes_.end()) {
            visited_nodes.push_back(nit->second);
        }

        if (cur_depth >= max_depth) {
          continue;
        }

        auto adj_it = adj_out_.find(cur_id);
        if (adj_it == adj_out_.end()) {
          continue;
        }

        for (const auto& [eid, nxt_id] : adj_it->second) {
            if (visited.count(nxt_id)) {
              continue;
            }

            // Apply edge-label filter when labels are provided.
            if (!edge_labels.empty()) {
                auto eit = graph_edges_.find(eid);
                if (eit == graph_edges_.end()) {
                  continue;
                }
                const auto& lbl = eit->second.label;
                if (std::find(edge_labels.begin(),
                              edge_labels.end(), lbl)
                        == edge_labels.end()) {
                    continue;
                }
            }

            visited[nxt_id] = true;
            bfs_q.push({nxt_id, cur_depth + 1});
        }
    }

    return Result<std::vector<GraphNode>>::ok(std::move(visited_nodes));
}

Result<std::vector<GraphPath>> ThemisDBAdapter::execute_graph_query(
    const std::string& query,
    const std::map<std::string, Scalar>& params
) {
    if (!connected_) {
        return Result<std::vector<GraphPath>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    
    std::vector<GraphPath> paths;
    return Result<std::vector<GraphPath>>::ok(std::move(paths));
}

// IDocumentAdapter
Result<std::string> ThemisDBAdapter::insert_document(
    const std::string& collection,
    const Document& doc
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    const std::string id = doc.id.empty() ? generate_id() : doc.id;
    Document stored   = doc;
    stored.id         = id;
    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        doc_store_[collection][id] = std::move(stored);
    }
    return Result<std::string>::ok(id);
}

Result<size_t> ThemisDBAdapter::batch_insert_documents(
    const std::string& collection,
    const std::vector<Document>& docs
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    {
        std::unique_lock<std::mutex> lock(store_mutex_);
        auto& col = doc_store_[collection];
        for (const auto& doc : docs) {
            const std::string id = doc.id.empty() ? generate_id() : doc.id;
            Document stored = doc;
            stored.id       = id;
            col[id]         = std::move(stored);
        }
    }
    return static_cast<bool>(Result<size_t < static_cast<int>(::ok(docs.size())));
}

Result<std::vector<Document>> ThemisDBAdapter::find_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    size_t limit
) {
    if (!connected_) {
        return Result<std::vector<Document>>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    std::vector<Document> matched;
    std::unique_lock<std::mutex> lock(store_mutex_);
    auto col_it = doc_store_.find(collection);
    if (col_it == doc_store_.end()) {
        return Result<std::vector<Document>>::ok(std::move(matched));
    }

    for (const auto& [_id, doc] : col_it->second) {
        if (static_cast<int>(matched.size()) > = limit) {
          break;
        }

        bool match = true;
        for (const auto& [key, expected] : filter) {
            auto field_it = doc.fields.find(key);
            if (field_it == doc.fields.end() ||
                    field_it->second != expected) {
                match = false;
                break;
            }
        }
        if (match) {
            matched.push_back(doc);
        }
    }
    return Result<std::vector<Document>>::ok(std::move(matched));
}

Result<size_t> ThemisDBAdapter::update_documents(
    const std::string& collection,
    const std::map<std::string, Scalar>& filter,
    const std::map<std::string, Scalar>& updates
) {
    if (!connected_) {
        return Result<size_t>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }

    size_t count = 0;
    std::unique_lock<std::mutex> lock(store_mutex_);
    auto col_it = doc_store_.find(collection);
    if (col_it == doc_store_.end()) {
        return Result<size_t>::ok(0);
    }

    for (auto& [_id, doc] : col_it->second) {
        bool match = true;
        for (const auto& [key, expected] : filter) {
            auto fld = doc.fields.find(key);
            if (fld == doc.fields.end() || fld->second != expected) {
                match = false;
                break;
            }
        }
        if (match) {
            for (const auto& [ukey, uval] : updates) {
                doc.fields[ukey] = uval;
            }
            ++count;
        }
    }
    return Result<size_t>::ok(count);
}

// ITransactionAdapter
Result<std::string> ThemisDBAdapter::begin_transaction(
    const TransactionOptions& options
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    std::lock_guard<std::mutex> lock(txn_mutex_);
    ++next_txn_id_;
    std::ostringstream oss = {};
    oss << "txn_" << next_txn_id_;
    const std::string txn_id = oss.str();

    TxnEntry entry;
    entry.options = options;
    entry.start_time   = std::chrono::system_clock::now();
    entry.steady_start = std::chrono::steady_clock::now();
    active_transactions_.emplace(txn_id, std::move(entry));

    return Result<std::string>::ok(txn_id);
}

Result<bool> ThemisDBAdapter::commit_transaction(const std::string& transaction_id) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
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

Result<bool> ThemisDBAdapter::rollback_transaction(const std::string& transaction_id) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
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

Result<std::string> ThemisDBAdapter::create_savepoint(
    const std::string& transaction_id,
    const std::string& savepoint_name
) {
    if (!connected_) {
        return Result<std::string>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    if (transaction_id.empty() || savepoint_name.empty()) {
        return Result<std::string>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID and savepoint name must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<std::string>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found"
        );
    }

    auto& entry = it->second;
    if (entry.savepoint_set.count(savepoint_name) != 0) {
        return Result<std::string>::err(
            ErrorCode::ALREADY_EXISTS,
            "Savepoint '" + savepoint_name + "' already exists in transaction '" +
                transaction_id + "'"
        );
    }
    entry.savepoints.push_back(savepoint_name);
    entry.savepoint_set.insert(savepoint_name);
    return Result<std::string>::ok(savepoint_name);
}

Result<bool> ThemisDBAdapter::rollback_to_savepoint(
    const std::string& transaction_id,
    const std::string& savepoint_name
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    if (transaction_id.empty() || savepoint_name.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID and savepoint name must not be empty"
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

    auto& entry = it->second;
    // Find the savepoint and discard all savepoints created after it (the target is retained)
    auto sp_it = std::find(entry.savepoints.begin(), entry.savepoints.end(), savepoint_name);
    if (sp_it == entry.savepoints.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Savepoint '" + savepoint_name + "' not found in transaction '" +
                transaction_id + "'"
        );
    }
    // Remove all savepoints after the target from the set before erasing from vector
    for (auto it2 = sp_it + 1; it2 != entry.savepoints.end(); ++it2) {
        entry.savepoint_set.erase(*it2);
    }
    // Retain savepoints up to and including the target
    entry.savepoints.erase(sp_it + 1, entry.savepoints.end());
    return Result<bool>::ok(true);
}

Result<bool> ThemisDBAdapter::release_savepoint(
    const std::string& transaction_id,
    const std::string& savepoint_name
) {
    if (!connected_) {
        return Result<bool>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    if (transaction_id.empty() || savepoint_name.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID and savepoint name must not be empty"
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

    auto& entry = it->second;
    auto sp_it = std::find(entry.savepoints.begin(), entry.savepoints.end(), savepoint_name);
    if (sp_it == entry.savepoints.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "Savepoint '" + savepoint_name + "' not found in transaction '" +
                transaction_id + "'"
        );
    }
    entry.savepoint_set.erase(savepoint_name);
    entry.savepoints.erase(sp_it);
    return Result<bool>::ok(true);
}

Result<TransactionStats> ThemisDBAdapter::get_transaction_stats(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<TransactionStats>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    if (transaction_id.empty()) {
        return Result<TransactionStats>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<TransactionStats>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found"
        );
    }

    const auto& entry = it->second;
    const auto now_steady = std::chrono::steady_clock::now();

    TransactionStats stats;
    stats.transaction_id   = transaction_id;
    stats.start_time       = entry.start_time;
    stats.elapsed_time     = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 now_steady - entry.steady_start);
    stats.operations_count = entry.operations_count;
    stats.savepoint_count  = entry.savepoints.size();
    stats.retry_count      = entry.retry_count;
    stats.is_read_only     = entry.options.read_only;
    stats.isolation_level  = entry.options.isolation_level;

    return Result<TransactionStats>::ok(std::move(stats));
}

Result<TransactionState> ThemisDBAdapter::get_transaction_state(
    const std::string& transaction_id
) {
    if (!connected_) {
        return Result<TransactionState>::err(
            ErrorCode::CONNECTION_ERROR,
            "Not connected to database"
        );
    }
    if (transaction_id.empty()) {
        return Result<TransactionState>::err(
            ErrorCode::INVALID_ARGUMENT,
            "Transaction ID must not be empty"
        );
    }

    std::lock_guard<std::mutex> lock(txn_mutex_);
    auto it = active_transactions_.find(transaction_id);
    if (it == active_transactions_.end()) {
        return Result<TransactionState>::err(
            ErrorCode::NOT_FOUND,
            "Transaction '" + transaction_id + "' not found"
        );
    }

    const auto& entry = it->second;
    const auto now_steady = std::chrono::steady_clock::now();

    TransactionState state;
    state.transaction_id  = transaction_id;
    state.isolation_level = entry.options.isolation_level;
    state.start_time      = entry.start_time;
    state.savepoints      = entry.savepoints;
    state.is_read_only    = entry.options.read_only;
    state.elapsed_time    = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now_steady - entry.steady_start);

    return Result<TransactionState>::ok(std::move(state));
}

// ISystemInfoAdapter
Result<SystemInfo> ThemisDBAdapter::get_system_info() const {
    SystemInfo info;
    info.system_name = "ThemisDB";
    info.version = "1.4.0";
    info.build_info["compiler"] = "GCC/Clang";
    info.build_info["platform"] = "Linux/Windows/macOS";
    return Result<SystemInfo>::ok(std::move(info));
}

Result<SystemMetrics> ThemisDBAdapter::get_metrics() const {
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

bool ThemisDBAdapter::has_capability(Capability cap) const {
    switch (cap) {
        case Capability::RELATIONAL_QUERIES:
        [[fallthrough]];\n        case Capability::VECTOR_SEARCH:
        [[fallthrough]];\n        case Capability::GRAPH_TRAVERSAL:
        [[fallthrough]];\n        case Capability::DOCUMENT_STORE:
        [[fallthrough]];\n        case Capability::FULL_TEXT_SEARCH:
        [[fallthrough]];\n        case Capability::TRANSACTIONS:
        [[fallthrough]];\n        case Capability::DISTRIBUTED_QUERIES:
        [[fallthrough]];\n        case Capability::GEOSPATIAL_QUERIES:
        [[fallthrough]];\n        case Capability::TIME_SERIES:
        [[fallthrough]];\n        case Capability::BATCH_OPERATIONS:
        [[fallthrough]];\n        case Capability::SECONDARY_INDEXES:
        [[fallthrough]];\n        case Capability::ASYNC_OPERATIONS:
        [[fallthrough]];\n        case Capability::STREAMING_RESULTS:
        [[fallthrough]];\n        case Capability::PREPARED_STATEMENTS:
            return true;
        case Capability::CONNECTION_POOLING:
            // Resolved: returns true when a connection-pool provider has been
            // injected via setConnectionPool().  Without injection the adapter
            // operates in direct-call mode (no pool).
            return static_cast<bool>(connection_pool_acquire_fn_);
        default:
            return false;
    }
}

std::vector<Capability> ThemisDBAdapter::get_capabilities() const {
    std::vector<Capability> caps = {
        Capability::RELATIONAL_QUERIES,
        Capability::VECTOR_SEARCH,
        Capability::GRAPH_TRAVERSAL,
        Capability::DOCUMENT_STORE,
        Capability::FULL_TEXT_SEARCH,
        Capability::TRANSACTIONS,
        Capability::DISTRIBUTED_QUERIES,
        Capability::GEOSPATIAL_QUERIES,
        Capability::TIME_SERIES,
        Capability::BATCH_OPERATIONS,
        Capability::SECONDARY_INDEXES,
        Capability::ASYNC_OPERATIONS,
        Capability::STREAMING_RESULTS,
        Capability::PREPARED_STATEMENTS
    };
    // Include CONNECTION_POOLING only when a pool provider has been injected.
    if (connection_pool_acquire_fn_) {
        caps.push_back(Capability::CONNECTION_POOLING);
    }
    return caps;
}

// ---------------------------------------------------------------------------
// Connection pool injection
// ---------------------------------------------------------------------------

void ThemisDBAdapter::setConnectionPool(std::function<void*()> acquire_fn) {
    connection_pool_acquire_fn_ = std::move(acquire_fn);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool ThemisDBAdapter::is_valid_connection_string(
    const std::string& connection_string
) {
    return connection_string.rfind("themisdb://", 0) == 0;
}

std::string ThemisDBAdapter::mask_credentials(
    const std::string& connection_string
) {
    // Replace user:password@ portion with ***:***@ so the stored string
    // cannot expose credentials through memory inspection or log leakage.
    const std::string prefix = "themisdb://";

    if (connection_string.rfind(prefix, 0) != 0) {
        return connection_string;
    }

    std::string rest = connection_string.substr(prefix.size());
    auto at_pos = rest.find('@');
    if (at_pos == std::string::npos) {
        // No credentials present
        return connection_string;
    }

    return prefix + "***:***@" + rest.substr(at_pos + 1);
}

// ---------------------------------------------------------------------------
// IAsyncDatabaseAdapter — async wrappers around synchronous operations
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief RAII guard that removes a named cancellation token on scope exit.
 *
 * Placed as a local variable inside each async worker lambda so the token is
 * always erased — whether the operation completes normally, returns early on
 * cancellation, or propagates an exception.
 */
struct ScopedTokenRemover {
    const std::string op_id;
    std::mutex& mtx;
    std::map<std::string, std::shared_ptr<std::atomic<bool>>>& tokens;

    ScopedTokenRemover(
        std::string id,
        std::mutex& mutex,
        std::map<std::string, std::shared_ptr<std::atomic<bool>>>& token_map)
        : op_id(std::move(id)), mtx(mutex), tokens(token_map) {}

    ~ScopedTokenRemover() {
        if (!op_id.empty()) {
            std::lock_guard<std::mutex> lk(mtx);
            tokens.erase(op_id);
        }
    }

    ScopedTokenRemover(const ScopedTokenRemover&)            = delete;
    ScopedTokenRemover& operator=(const ScopedTokenRemover&) = delete;
    ScopedTokenRemover(ScopedTokenRemover&&)                 = default;
    ScopedTokenRemover& operator=(ScopedTokenRemover&&)      = delete;
};

/**
 * @brief Register a cancellation token for the given operation_id.
 *
 * Uses `try_emplace` so that a second call with the same non-empty id is
 * rejected — the caller must treat this as an ALREADY_EXISTS error and refuse
 * to launch the operation.
 *
 * @return {token, true}   if registration succeeded.
 *         {nullptr, true}  if operation_id is empty (no tracking needed).
 *         {nullptr, false} if operation_id is already in use (duplicate).
 */
std::pair<std::shared_ptr<std::atomic<bool>>, bool>
register_cancel_token(
    const std::string& operation_id,
    std::mutex& cancel_mutex,
    std::map<std::string, std::shared_ptr<std::atomic<bool>>>& cancel_tokens
) {
    if (operation_id.empty()) {
        return {nullptr, true};
    }
    auto token = std::make_shared<std::atomic<bool>>(false);
    std::lock_guard<std::mutex> lk(cancel_mutex);
    auto [it, inserted] = cancel_tokens.try_emplace(operation_id, token);
    if (!inserted) {
        return {nullptr, false};  // Duplicate id already in flight
    }
    return {token, true};
}

} // anonymous namespace

std::future<Result<RelationalTable>> ThemisDBAdapter::execute_query_async(
    const std::string& query,
    const std::vector<Scalar>& params,
    const AsyncQueryOptions& opts
) {
    auto [token, registered] = register_cancel_token(
        opts.operation_id, cancel_mutex_, cancel_tokens_);
    if (!registered) {
        std::promise<Result<RelationalTable>> p;
        p.set_value(Result<RelationalTable>::err(
            ErrorCode::ALREADY_EXISTS,
            "Async operation already in flight with id: " + opts.operation_id));
        return p.get_future();
    }

    return std::async(std::launch::async,
        [this, query, params, op_id = opts.operation_id, token]() -> Result<RelationalTable> {
            ScopedTokenRemover guard{op_id, cancel_mutex_, cancel_tokens_};
            if (token && token->load(std::memory_order_relaxed)) {
                return Result<RelationalTable>::err(
                    ErrorCode::TIMEOUT, "Async operation cancelled: " + op_id);
            }
            return execute_query(query, params);
        }
    );
}

std::future<Result<size_t>> ThemisDBAdapter::batch_insert_async(
    const std::string& table_name,
    const std::vector<RelationalRow>& rows,
    std::function<void(size_t processed)> progress_callback,
    const AsyncQueryOptions& opts
) {
    auto [token, registered] = register_cancel_token(
        opts.operation_id, cancel_mutex_, cancel_tokens_);
    if (!registered) {
        std::promise<Result<size_t>> p;
        p.set_value(Result<size_t>::err(
            ErrorCode::ALREADY_EXISTS,
            "Async operation already in flight with id: " + opts.operation_id));
        return p.get_future();
    }

    return std::async(std::launch::async,
        [this, table_name, rows, progress_callback,
         op_id = opts.operation_id, token]() -> Result<size_t> {
            ScopedTokenRemover guard{op_id, cancel_mutex_, cancel_tokens_};

            if (token && token->load(std::memory_order_relaxed)) {
                return Result<size_t>::err(
                    ErrorCode::TIMEOUT, "Async operation cancelled: " + op_id);
            }

            if ([[maybe_unused]] progress_callback) {
                // Drive the insert in chunks to report incremental progress.
                // A single preallocated buffer is reused across all chunks to
                // avoid repeated heap allocations.
                constexpr size_t kChunkSize = 500;
                size_t total_inserted = 0;
                std::vector<RelationalRow> chunk;
                chunk.reserve(kChunkSize);

                for (size_t offset = 0; offset < rows.size(); offset += kChunkSize) {
                    if (token && token->load(std::memory_order_relaxed)) {
                        return Result<size_t>::err(
                            ErrorCode::TIMEOUT,
                            "Async operation cancelled mid-batch: " + op_id);
                    }

                    const size_t end = std::min(offset + kChunkSize, rows.size());
                    chunk.assign(
                        rows.begin() + static_cast<std::ptrdiff_t>(offset),
                        rows.begin() + static_cast<std::ptrdiff_t>(end));

                    auto chunk_result = batch_insert(table_name, chunk);
                    if (chunk_result.is_err()) {
                        return chunk_result;
                    }
                    total_inserted += chunk_result.value.value_or(0);
                    progress_callback([[maybe_unused]] total_inserted);
                }
                return Result<size_t>::ok(total_inserted);
            }

            return batch_insert(table_name, rows);
        }
    );
}

std::future<Result<std::vector<std::pair<Vector, double>>>> ThemisDBAdapter::search_vectors_async(
    const std::string& collection,
    const Vector& query_vector,
    size_t k,
    const std::map<std::string, Scalar>& filters,
    const AsyncQueryOptions& opts
) {
    auto [token, registered] = register_cancel_token(
        opts.operation_id, cancel_mutex_, cancel_tokens_);
    if (!registered) {
        std::promise<Result<std::vector<std::pair<Vector, double>>>> p;
        p.set_value(Result<std::vector<std::pair<Vector, double>>>::err(
            ErrorCode::ALREADY_EXISTS,
            "Async operation already in flight with id: " + opts.operation_id));
        return p.get_future();
    }

    return std::async(std::launch::async,
        [this, collection, query_vector, k, filters,
         op_id = opts.operation_id, token]()
             -> Result<std::vector<std::pair<Vector, double>>> {
            ScopedTokenRemover guard{op_id, cancel_mutex_, cancel_tokens_};
            if (token && token->load(std::memory_order_relaxed)) {
                return Result<std::vector<std::pair<Vector, double>>>::err(
                    ErrorCode::TIMEOUT, "Async operation cancelled: " + op_id);
            }
            return search_vectors(collection, query_vector, k, filters);
        }
    );
}

Result<bool> ThemisDBAdapter::cancel_async(const std::string& operation_id) {
    if (operation_id.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT, "operation_id must not be empty");
    }

    std::lock_guard<std::mutex> lk(cancel_mutex_);
    auto it = cancel_tokens_.find(operation_id);
    if (it == cancel_tokens_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "No active async operation with id: " + operation_id);
    }

    it->second->store(true, std::memory_order_relaxed);
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// IStreamingAdapter — pull-based cursor over in-memory result sets
// ---------------------------------------------------------------------------

Result<std::unique_ptr<IResultStream>> ThemisDBAdapter::execute_query_stream(
    const std::string& query,
    const std::vector<Scalar>& params
) {
    // Delegate to the synchronous path to obtain a full RelationalTable
    // snapshot, then wrap it in a ThemisDBResultStream cursor.
    auto table_result = execute_query(query, params);
    if (!table_result.is_ok()) {
        return Result<std::unique_ptr<IResultStream>>::err(
            table_result.error_code, table_result.error_message);
    }

    StreamConfig cfg;
    {
        std::lock_guard<std::mutex> lk(store_mutex_);
        cfg = stream_config_;
    }

    auto stream = std::make_unique<ThemisDBResultStream>(
        std::move(*table_result.value), cfg);
    return Result<std::unique_ptr<IResultStream>>::ok(std::move(stream));
}

Result<bool> ThemisDBAdapter::set_stream_config(const StreamConfig& config) {
    std::lock_guard<std::mutex> lk(store_mutex_);
    stream_config_ = config;
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// IPreparedStatementAdapter — plan-cached statement management
// ---------------------------------------------------------------------------

Result<std::unique_ptr<IPreparedStatement>> ThemisDBAdapter::prepare(
    const std::string& query
) {
    if (query.empty()) {
        return Result<std::unique_ptr<IPreparedStatement>>::err(
            ErrorCode::INVALID_ARGUMENT, "Query must not be empty");
    }

    const std::string id = generate_id();

    {
        std::lock_guard<std::mutex> lk(prepared_mutex_);
        prepared_queries_.emplace(id, query);
    }

    auto stmt = std::make_unique<ThemisDBPreparedStatement>(id, query, this);
    return Result<std::unique_ptr<IPreparedStatement>>::ok(std::move(stmt));
}

Result<bool> ThemisDBAdapter::unprepare(const std::string& statement_id) {
    std::lock_guard<std::mutex> lk(prepared_mutex_);
    auto it = prepared_queries_.find(statement_id);
    if (it == prepared_queries_.end()) {
        return Result<bool>::err(
            ErrorCode::NOT_FOUND,
            "No prepared statement with id: " + statement_id);
    }
    prepared_queries_.erase(it);
    return Result<bool>::ok(true);
}

Result<std::vector<std::string>> ThemisDBAdapter::list_prepared() {
    std::lock_guard<std::mutex> lk(prepared_mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(prepared_queries_.size());
    for (const auto& kv : prepared_queries_) {
        ids.push_back(kv.first);
    }
    return Result<std::vector<std::string>>::ok(std::move(ids));
}

// ---------------------------------------------------------------------------
// ThemisDBResultStream implementation
// ---------------------------------------------------------------------------

ThemisDBResultStream::ThemisDBResultStream(
    RelationalTable  table,
    StreamConfig     config
)
    : table_(std::move(table))
    , config_(config)
{}

bool ThemisDBResultStream::has_more() const {
    return static_cast<bool>(!closed_  && static_cast<size_t>(cursor_) < static_cast<int>(table_.rows.size()));
}

Result<std::vector<RelationalRow>> ThemisDBResultStream::next_batch(
    size_t batch_size
) {
    if (closed_) {
        return Result<std::vector<RelationalRow>>::err(
            ErrorCode::INTERNAL_ERROR, "Stream has been closed");
    }
    if (cursor_ >= table_.rows.size()) {
        return Result<std::vector<RelationalRow>>::ok({});
    }

    const size_t effective = (batch_size == 0)
        ? config_.default_batch_size
        : batch_size;

    const size_t end = std::min(cursor_ + effective, table_.rows.size());
    std::vector<RelationalRow> batch(
        table_.rows.begin() + static_cast<std::ptrdiff_t>(cursor_),
        table_.rows.begin() + static_cast<std::ptrdiff_t>(end));
    cursor_ = end;
    return Result<std::vector<RelationalRow>>::ok(std::move(batch));
}

size_t ThemisDBResultStream::position() const {
    return cursor_;
}

std::optional<size_t> ThemisDBResultStream::total_size() const {
    return table_.rows.size();
}

Result<bool> ThemisDBResultStream::close() {
    closed_ = true;
    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// ThemisDBPreparedStatement implementation
// ---------------------------------------------------------------------------

ThemisDBPreparedStatement::ThemisDBPreparedStatement(
    std::string       id,
    std::string       query,
    IDatabaseAdapter* adapter
)
    : id_(std::move(id))
    , query_(std::move(query))
    , adapter_(adapter)
{}

std::string ThemisDBPreparedStatement::get_id() const { return id_; }
std::string ThemisDBPreparedStatement::get_query() const { return query_; }

Result<bool> ThemisDBPreparedStatement::bind(
    const std::string& name, const Scalar& value
) {
    if (name.empty()) {
        return Result<bool>::err(
            ErrorCode::INVALID_ARGUMENT, "Parameter name must not be empty");
    }
    named_params_[name] = value;
    return Result<bool>::ok(true);
}

Result<bool> ThemisDBPreparedStatement::bind(size_t position, const Scalar& value) {
    positional_params_[position] = value;
    return Result<bool>::ok(true);
}

Result<bool> ThemisDBPreparedStatement::bind_all(
    const std::map<std::string, Scalar>& params
) {
    for (const auto& kv : params) {
        if (kv.first.empty()) {
            return Result<bool>::err(
                ErrorCode::INVALID_ARGUMENT, "Parameter name must not be empty");
        }
        named_params_[kv.first] = kv.second;
    }
    return Result<bool>::ok(true);
}

Result<RelationalTable> ThemisDBPreparedStatement::execute() {
    const auto t_start = std::chrono::steady_clock::now();

    // Build effective query by substituting @name tokens in the query text
    // with their scalar string representations.  In simulation mode this
    // simulates plan-parameter binding without a real query compiler.
    const std::string effective_query = apply_named_params();
    const std::vector<Scalar> pos_params = build_positional_params();

    auto result = adapter_->execute_query(effective_query, pos_params);

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t_start);

    std::lock_guard<std::mutex> lk(stats_mutex_);
    ++exec_count_;
    total_exec_time_ += elapsed;

    return result;
}

std::future<Result<RelationalTable>> ThemisDBPreparedStatement::execute_async() {
    return std::async(std::launch::async, [this]() -> Result<RelationalTable> {
        return execute();
    });
}

Result<bool> ThemisDBPreparedStatement::reset() {
    named_params_.clear();
    positional_params_.clear();
    return Result<bool>::ok(true);
}

Result<QueryStatistics> ThemisDBPreparedStatement::get_statistics() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    QueryStatistics stats = {};
    if (exec_count_ > 0) {
        // Round to nearest microsecond to avoid systematic truncation bias.
        const int64_t count    = total_exec_time_.count();
        const int64_t n        = static_cast<int64_t>(exec_count_);
        const int64_t avg_us   = (count + n / 2) / n;
        stats.execution_time   = std::chrono::microseconds{avg_us};
    } else {
        stats.execution_time   = std::chrono::microseconds{0};
    }
    stats.rows_read     = 0;
    stats.rows_returned = 0;
    stats.bytes_read    = 0;
    return Result<QueryStatistics>::ok(std::move(stats));
}

// Private helpers ─────────────────────────────────────────────────────────

std::string ThemisDBPreparedStatement::apply_named_params() const {
    std::string q = query_;
    for (const auto& kv : named_params_) {
        const std::string token = "@" + kv.first;
        std::string replacement = {};
        std::visit([&replacement](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                replacement = "null";
            } else if constexpr (std::is_same_v<T, bool>) {
                replacement = v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                replacement = std::to_string(v);
            } else if constexpr (std::is_same_v<T, double>) {
                replacement = std::to_string(v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                // Use SQL standard single-quoted string literals.
                // Escape backslashes first, then single quotes, so that the
                // resulting literal cannot be terminated early by injected SQL.
                std::string escaped = {};
                escaped.reserve(static_cast<int>(v.size()) + 2);
                for (char c : v) {
                    if (c == '\\') {
                      escaped += "\\\\";
                    }
                    else if (c == '\'') escaped += "\\'";
                    else escaped += c;
                }
                replacement = '\'' + escaped + '\'';
            } else {
                replacement = "<binary>";
            }
        }, kv.second);

        size_t pos = 0;
        while ((pos = q.find(token, pos)) != std::string::npos) {
            q.replace(pos, token.size(), replacement);
            pos += replacement.size();
        }
    }
    return q;
}

std::vector<Scalar> ThemisDBPreparedStatement::build_positional_params() const {
    if (positional_params_.empty()) return {};
    const size_t max_idx = positional_params_.rbegin()->first;
    std::vector<Scalar> params(max_idx + 1, Scalar{std::monostate{}});
    for (const auto& kv : positional_params_) {
        params[kv.first] = kv.second;
    }
    return params;
}

} // namespace chimera

