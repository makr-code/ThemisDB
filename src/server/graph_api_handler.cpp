/**
 * @file graph_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=19, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/graph_api_handler.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/graph_index.h"
#include "graph/path_constraints.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

using json = nlohmann::json;

namespace themis {
namespace server {

GraphApiHandler::GraphApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , graph_index_(std::move(graph_index))
    , auth_(std::move(auth))
{
    if (graph_index_) {
        optimizer_ = std::make_unique<themis::graph::GraphQueryOptimizer>(*graph_index_);
    }
}

http::response<http::string_body> GraphApiHandler::handleTraverse(
    const http::request<http::string_body>& req
) {
    // Implementation moved from http_server.cpp handleGraphTraverse()
    auto span = Tracer::startSpan("handleGraphTraverse");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/graph/traverse");
    
    try {
        json body_json = json::parse(req.body());
        
        if (!body_json.contains("start_vertex") || !body_json.contains("max_depth")) {
            span.setAttribute("error", "missing_required_fields");
            span.setStatus(false, "Missing required fields");
            return makeErrorResponse(http::status::bad_request,
                "Missing 'start_vertex' or 'max_depth'", req);
        }

        std::string start_vertex = body_json["start_vertex"];
        // GAP-010 fixed: cap max_depth to prevent BFS-induced CPU saturation / OOM.
        static constexpr size_t kMaxBfsDepth = 50;
        size_t max_depth = body_json["max_depth"];
        if (max_depth > kMaxBfsDepth) {
            span.setAttribute("error", "max_depth_exceeded");
            span.setStatus(false, "max_depth exceeds server limit");
            return makeErrorResponse(http::status::bad_request,
                "max_depth exceeds server limit of " + std::to_string(kMaxBfsDepth), req);
        }
        
        span.setAttribute("graph.start_vertex", start_vertex);
        span.setAttribute("graph.max_depth", static_cast<int64_t>(max_depth));

        // Perform BFS traversal
        auto [status, visited] = graph_index_->bfs(start_vertex, static_cast<int>(max_depth));

        if (!status.ok) {
            span.setAttribute("error", "traversal_failed");
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Traversal failed", req);
        }
        
        span.setAttribute("graph.visited_count", static_cast<int64_t>(visited.size()));
        span.setStatus(true);

        json response = {
            {"start_vertex", start_vertex},
            {"max_depth", max_depth},
            {"visited_count", visited.size()},
            {"visited", visited}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        span.recordError("JSON parse error: " + std::string(e.what()));
        span.setStatus(false);
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Graph traverse error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> GraphApiHandler::handleEdgeCreate(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEdgeCreate");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/graph/edge");
    
    try {
        json body_json = json::parse(req.body());
        
        // Validate required fields for edge creation
        if (!body_json.contains("id") || !body_json.contains("_from") || !body_json.contains("_to")) {
            span.setAttribute("error", "missing_required_fields");
            span.setStatus(false, "Missing required fields");
            return makeErrorResponse(http::status::bad_request,
                "Missing required fields: 'id', '_from', '_to'", req);
        }

        std::string edge_id = body_json["id"];
        span.setAttribute("graph.edge_id", edge_id);

        // Create BaseEntity from JSON
        BaseEntity::FieldMap fields;
        for (auto& [key, value] : body_json.items()) {
            if (value.is_string()) {
                fields[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                fields[key] = value.get<int64_t>();
            } else if (value.is_number_float()) {
                fields[key] = value.get<double>();
            } else if (value.is_boolean()) {
                fields[key] = value.get<bool>();
            } else if (value.is_null()) {
                fields[key] = std::monostate{};
            }
        }

        BaseEntity edge(edge_id, fields);

        // Add edge to graph index
        auto status = graph_index_->addEdge(edge);
        
        if (!status.ok) {
            span.setAttribute("error", "edge_creation_failed");
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to create edge: " + status.message, req);
        }

        // Store edge entity
        auto edge_key = KeySchema::makeGraphEdgeKey(edge_id);
        bool stored = storage_->put(edge_key, edge.serialize());
        
        if (!stored) {
            span.setAttribute("error", "edge_storage_failed");
            span.setStatus(false, "Failed to store edge");
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to store edge", req);
        }

        span.setStatus(true);
        json response = {
            {"status", "ok"},
            {"edge_id", edge_id}
        };

        // Notify incremental queries about the new edge.
        if (optimizer_) {
            themis::graph::GraphQueryOptimizer::GraphChangeSet cs;
            const std::string from = body_json.value("_from", std::string{});
            const std::string to   = body_json.value("_to",   std::string{});
            cs.addEdgeAdded(edge_id, from, to);
            optimizer_->onGraphChange(cs);
        }

        return makeResponse(http::status::created, response.dump(), req);

    } catch (const json::exception& e) {
        span.recordError("JSON parse error: " + std::string(e.what()));
        span.setStatus(false);
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Edge create error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> GraphApiHandler::handleEdgeDelete(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEdgeDelete");
    span.setAttribute("http.method", "DELETE");
    span.setAttribute("http.path", "/graph/edge/:id");
    
    try {
        // Extract edge ID from path: /graph/edge/{edge_id}
        std::string target = std::string(req.target());
        std::string edge_id = extractPathParam(target, "/graph/edge/");
        
        if (edge_id.empty()) {
            span.setAttribute("error", "missing_edge_id");
            span.setStatus(false, "Missing edge ID");
            return makeErrorResponse(http::status::bad_request,
                "Missing edge ID in path", req);
        }

        span.setAttribute("graph.edge_id", edge_id);

        // Look up the edge endpoints before deletion so we can notify
        // incremental queries with the correct from/to vertices.
        std::string edge_from;
        std::string edge_to;
        if (optimizer_) {
            auto from_opt = graph_index_->getEdgeField(edge_id, "_from");
            auto to_opt   = graph_index_->getEdgeField(edge_id, "_to");
            if (from_opt) {
              edge_from = std::move(*from_opt);
            }
            if (to_opt) {
              edge_to   = std::move(*to_opt);
            }
        }

        // Delete edge from graph index
        auto status = graph_index_->deleteEdge(edge_id);
        
        if (!status.ok) {
            span.setAttribute("error", "edge_deletion_failed");
            span.setStatus(false, status.message);
            return makeErrorResponse(http::status::internal_server_error,
                "Failed to delete edge: " + status.message, req);
        }

        // Delete edge entity from storage
        auto edge_key = KeySchema::makeGraphEdgeKey(edge_id);
        bool deleted = storage_->del(edge_key);
        
        if (!deleted) {
            // Edge was already deleted or didn't exist in storage
            THEMIS_WARN("Edge key '{}' not found in storage during deletion", edge_key);
        }

        span.setStatus(true);
        json response = {
            {"status", "ok"},
            {"edge_id", edge_id}
        };

        // Notify incremental queries about the removed edge.
        if (optimizer_) {
            themis::graph::GraphQueryOptimizer::GraphChangeSet cs;
            cs.addEdgeRemoved(edge_id, edge_from, edge_to);
            optimizer_->onGraphChange(cs);
        }

        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Edge delete error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> GraphApiHandler::handleMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGraphMetrics");
    span.setAttribute("http.method", "GET");
    span.setAttribute("http.path", "/api/v1/graph/metrics");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    const auto& m = optimizer_->getQueryMetrics();
    json response = {
        {"total_queries",           m.total_queries.load(std::memory_order_relaxed)},
        {"failed_queries",          m.failed_queries.load(std::memory_order_relaxed)},
        {"timed_out_queries",       m.timed_out_queries.load(std::memory_order_relaxed)},
        {"total_execution_time_ms", m.total_execution_time_ms.load(std::memory_order_relaxed)},
        {"max_execution_time_ms",   m.max_execution_time_ms.load(std::memory_order_relaxed)},
        {"avg_execution_time_ms",   m.avgExecutionTimeMs()},
        {"total_nodes_explored",    m.total_nodes_explored.load(std::memory_order_relaxed)},
        {"total_edges_traversed",   m.total_edges_traversed.load(std::memory_order_relaxed)},
        {"plan_cache_hits",         m.plan_cache_hits.load(std::memory_order_relaxed)},
        {"plan_cache_misses",       m.plan_cache_misses.load(std::memory_order_relaxed)},
        {"plan_cache_evictions",    m.plan_cache_evictions.load(std::memory_order_relaxed)},
        {"error_rate",              m.errorRate()}
    };

    span.setAttribute("graph.total_queries",
        static_cast<int64_t>(m.total_queries.load(std::memory_order_relaxed)));
    span.setStatus(true);
    return makeResponse(http::status::ok, response.dump(), req);
}

http::response<http::string_body> GraphApiHandler::handleMetricsPrometheus(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGraphMetricsPrometheus");
    span.setAttribute("http.method", "GET");
    span.setAttribute("http.path", "/api/v1/graph/metrics/prometheus");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    const auto& m = optimizer_->getQueryMetrics();

    // Build Prometheus text exposition format (text/plain; version=0.0.4)
    std::string body;
    body.reserve(2048);

    auto counter = [&](const char* name, const char* help, uint64_t value) {
        body += "# HELP "; body += name; body += ' '; body += help; body += '\n';
        body += "# TYPE "; body += name; body += " counter\n";
        body += name; body += ' '; body += std::to_string(value); body += '\n';
    };
    auto gauge = [&](const char* name, const char* help, double value) {
        body += "# HELP "; body += name; body += ' '; body += help; body += '\n';
        body += "# TYPE "; body += name; body += " gauge\n";
        body += name; body += ' '; body += std::to_string(value); body += '\n';
    };

    counter("themis_graph_queries_total",
            "Total graph traversal executions since startup",
            m.total_queries.load(std::memory_order_relaxed));
    counter("themis_graph_query_errors_total",
            "Graph traversal executions that returned no result",
            m.failed_queries.load(std::memory_order_relaxed));
    counter("themis_graph_query_timeouts_total",
            "Graph traversal executions aborted by timeout_ms SLO",
            m.timed_out_queries.load(std::memory_order_relaxed));
    counter("themis_graph_query_duration_ms_sum",
            "Sum of all graph query execution durations in milliseconds",
            m.total_execution_time_ms.load(std::memory_order_relaxed));
    counter("themis_graph_query_duration_ms_max",
            "Peak single-query execution duration in milliseconds",
            m.max_execution_time_ms.load(std::memory_order_relaxed));
    counter("themis_graph_nodes_explored_total",
            "Cumulative number of graph nodes visited across all queries",
            m.total_nodes_explored.load(std::memory_order_relaxed));
    counter("themis_graph_edges_traversed_total",
            "Cumulative number of graph edges traversed across all queries",
            m.total_edges_traversed.load(std::memory_order_relaxed));
    counter("themis_graph_plan_cache_hits_total",
            "Plan-cache hit count",
            m.plan_cache_hits.load(std::memory_order_relaxed));
    counter("themis_graph_plan_cache_misses_total",
            "Plan-cache miss count",
            m.plan_cache_misses.load(std::memory_order_relaxed));
    counter("themis_graph_plan_cache_evictions_total",
            "Plan-cache entries evicted by LRU or TTL policy",
            m.plan_cache_evictions.load(std::memory_order_relaxed));
    gauge("themis_graph_query_error_rate",
          "Fraction of graph queries that failed (0.0-1.0)",
          m.errorRate());
    gauge("themis_graph_query_avg_duration_ms",
          "Average graph query execution time in milliseconds",
          m.avgExecutionTimeMs());

    // Latency histogram
    const uint64_t total = m.total_queries.load(std::memory_order_relaxed);
    body += "# HELP themis_graph_latency_ms Latency histogram of graph query execution\n";
    body += "# TYPE themis_graph_latency_ms histogram\n";

    uint64_t cumulative = 0;
    const char* bound_labels[9] = {"1","5","10","25","50","100","250","500","1000"};
    for (size_t i = 0; i < 9; ++i) {
        cumulative += m.latency_histogram.counts[i].load(std::memory_order_relaxed);
        body += "themis_graph_latency_ms_bucket{le=\"";
        body += bound_labels[i];
        body += "\"} ";
        body += std::to_string(cumulative);
        body += '\n';
    }
    cumulative += m.latency_histogram.counts[9].load(std::memory_order_relaxed);
    body += "themis_graph_latency_ms_bucket{le=\"+Inf\"} ";
    body += std::to_string(cumulative);
    body += '\n';
    body += "themis_graph_latency_ms_sum ";
    body += std::to_string(m.total_execution_time_ms.load(std::memory_order_relaxed));
    body += '\n';
    body += "themis_graph_latency_ms_count ";
    body += std::to_string(total); body += '\n';

    // p50 / p95 / p99 computed gauges
    gauge("themis_graph_latency_p50_ms",
          "Approximate p50 (median) graph query latency in milliseconds",
          m.latency_histogram.percentileMs(0.50));
    gauge("themis_graph_latency_p95_ms",
          "Approximate p95 graph query latency in milliseconds",
          m.latency_histogram.percentileMs(0.95));
    gauge("themis_graph_latency_p99_ms",
          "Approximate p99 graph query latency in milliseconds",
          m.latency_histogram.percentileMs(0.99));

    span.setAttribute("graph.total_queries",
        static_cast<int64_t>(total));
    span.setStatus(true);

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "text/plain; version=0.0.4");
    res.keep_alive(req.keep_alive());
    res.body() = std::move(body);
    res.prepare_payload();
    return res;
}

std::string GraphApiHandler::extractPathParam(
    const std::string& target,
    const std::string& prefix
) {
    // Extract parameter from path after prefix
    if (target.size() <= prefix.size()) {
      return "";
    }
    if (target.substr(0, prefix.size()) != prefix) {
      return "";
    }
    
    std::string param = target.substr(prefix.size());
    
    // Remove query string if present
    auto qpos = param.find('?');
    if (qpos != std::string::npos) {
        param = param.substr(0, qpos);
    }
    
    return param;
}

// ─────────────────────────────────────────────────────────────────────────────
// Incremental graph query HTTP endpoints (live-update integration)
// ─────────────────────────────────────────────────────────────────────────────

themis::graph::GraphQueryOptimizer::GraphChangeSet
GraphApiHandler::parseChangeSet([[maybe_unused]] const json& changes_array) {
    using CS = themis::graph::GraphQueryOptimizer::GraphChangeSet;
    CS cs;
    if (!changes_array.is_array()) {
      return cs;
    }

    for (const auto& item : changes_array) {
        if (!item.contains("type") || !item.contains("id")) {
          continue;
        }
        const std::string type = item["type"].get<std::string>();
        const std::string id   = item["id"].get<std::string>();
        const std::string from = item.value("from", std::string{});
        const std::string to   = item.value("to",   std::string{});

        if (type == "edge_added") {
            cs.addEdgeAdded(id, from, to);
        } else if (type == "edge_removed") {
            cs.addEdgeRemoved(id, from, to);
        } else if (type == "vertex_added") {
            cs.addVertexAdded(id);
        } else if (type == "vertex_removed") {
            cs.addVertexRemoved(id);
        } else {
            // Unknown type strings are logged as a warning to aid debugging.
            THEMIS_WARN("GraphChanges: unknown change type '{}' — skipped", type);
        }
    }
    return cs;
}

http::response<http::string_body> GraphApiHandler::handleIncrementalQueryRegister(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIncrementalQueryRegister");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/graph/query/incremental");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    try {
        json body_json = json::parse(req.body());

        if (!body_json.contains("start_vertex") || !body_json.contains("max_depth")) {
            span.setStatus(false, "missing required fields");
            return makeErrorResponse(http::status::bad_request,
                "Missing 'start_vertex' or 'max_depth'", req);
        }

        const std::string start_vertex = body_json["start_vertex"].get<std::string>();
        const int max_depth = body_json["max_depth"].get<int>();

        themis::graph::GraphQueryOptimizer::QueryConstraints constraints;
        if (body_json.contains("edge_type")) {
            constraints.edge_type = body_json["edge_type"].get<std::string>();
        }
        if (body_json.contains("max_results")) {
            constraints.max_results = static_cast<size_t>(body_json["max_results"].get<int>());
        }

        // The callback stores the latest IncrementalQueryResult by handle so
        // that clients can observe the updated result after posting /graph/changes.
        // Use a shared_ptr so the handle value outlives this stack frame.
        auto handle_ptr =
            std::make_shared<themis::graph::GraphQueryOptimizer::IncrementalQueryHandle>(0);

        auto handle = optimizer_->registerIncrementalBFS(
            start_vertex, max_depth, constraints,
            [this, handle_ptr](
                const themis::graph::GraphQueryOptimizer::IncrementalQueryResult& result) {
                incremental_results_[*handle_ptr] = result;
            }
        );

        // Back-fill the captured handle so subsequent callbacks can index correctly.
        *handle_ptr = handle;

        // Seed the result cache with the initial result (empty delta, full current).
        themis::graph::GraphQueryOptimizer::ExecutionStats init_stats;
        auto init_bfs = optimizer_->executeBFS(start_vertex, max_depth, constraints, &init_stats);
        std::vector<std::string> initial;
        if (init_bfs) {
          initial = init_bfs.value();
        }

        themis::graph::GraphQueryOptimizer::IncrementalQueryResult seed;
        seed.reexecuted = false;
        seed.current    = initial;
        incremental_results_[handle] = seed;

        span.setAttribute("graph.incremental_handle", static_cast<int64_t>(handle));
        span.setStatus(true);

        json response = {
            {"handle",          handle},
            {"initial_results", initial}
        };
        return makeResponse(http::status::created, response.dump(), req);

    } catch (const json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Incremental register error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> GraphApiHandler::handleIncrementalQueryUnregister(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIncrementalQueryUnregister");
    span.setAttribute("http.method", "DELETE");
    span.setAttribute("http.path", "/graph/query/incremental/:handle");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    const std::string target = std::string(req.target());
    const std::string handle_str =
        extractPathParam(target, "/graph/query/incremental/");

    if (handle_str.empty()) {
        span.setStatus(false, "missing handle");
        return makeErrorResponse(http::status::bad_request,
            "Missing incremental query handle in path", req);
    }

    themis::graph::GraphQueryOptimizer::IncrementalQueryHandle handle = 0;
    try {
        handle = std::stoull(handle_str);
    } catch (...) {
        THEMIS_WARN([[maybe_unused]] "graph_api_handler: unhandled exception caught");
        span.setStatus(false, "invalid handle");
        return makeErrorResponse(http::status::bad_request,
            "Invalid handle: not a valid integer", req);
    }

    if (incremental_results_.find(handle) == incremental_results_.end()) {
        span.setStatus(false, "handle not found");
        return makeErrorResponse(http::status::not_found,
            "Incremental query handle not found: " + handle_str, req);
    }

    optimizer_->unregisterIncrementalQuery(handle);
    incremental_results_.erase(handle);

    span.setAttribute("graph.incremental_handle", static_cast<int64_t>(handle));
    span.setStatus(true);

    json response = {{"status", "ok"}, {"handle", handle}};
    return makeResponse(http::status::ok, response.dump(), req);
}

http::response<http::string_body> GraphApiHandler::handleGraphChanges(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGraphChanges");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/graph/changes");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }
    auto& optimizer = *optimizer_;

    try {
        json body_json = json::parse(req.body());

        if (!body_json.contains("changes") || !body_json["changes"].is_array()) {
            span.setStatus(false, "missing changes array");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid 'changes' array", req);
        }

        auto cs = parseChangeSet(body_json["changes"]);
        const size_t reexecuted = optimizer.onGraphChange(cs);

        span.setAttribute("graph.changes_count",    static_cast<int64_t>(cs.size()));
        span.setAttribute("graph.queries_reexecuted", static_cast<int64_t>(reexecuted));
        span.setStatus(true);

        json response = {
            {"queries_reexecuted", reexecuted},
            {"changes_applied",    cs.size()}
        };
        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Graph changes notify error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Cost model calibration HTTP API (v1.8.0)
// ─────────────────────────────────────────────────────────────────────────────

static const char* costModelAlgoName(
    themis::graph::GraphQueryOptimizer::TraversalAlgorithm algo)
{
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    switch (algo) {
        case Algo::BFS:           return "BFS";
        case Algo::DFS:           return "DFS";
        case Algo::DIJKSTRA:      return "DIJKSTRA";
        case Algo::ASTAR:         return "ASTAR";
        case Algo::BIDIRECTIONAL: return "BIDIRECTIONAL";
        default:                  return "UNKNOWN";
    }
}

http::response<http::string_body> GraphApiHandler::handleCostModelCalibrate(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleCostModelCalibrate");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/api/v1/graph/cost-model/calibrate");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    auto report = optimizer_->calibrateFromHistory();

    json algo_stats_json = json::object();
    for (const auto& [algo, stats] : report.algorithm_stats) {
        json entry = {
            {"mean_execution_ms",        stats.mean_execution_ms},
            {"stddev_execution_ms",      stats.stddev_execution_ms},
            {"min_execution_ms",         stats.min_execution_ms},
            {"max_execution_ms",         stats.max_execution_ms},
            {"sample_count",             stats.sample_count},
            {"mean_estimated_ms",        stats.mean_estimated_ms},
            {"mean_absolute_error_ms",   stats.mean_absolute_error_ms},
            {"cost_ratio",               stats.cost_ratio},
            {"estimation_sample_count",  stats.estimation_sample_count}
        };
        algo_stats_json[costModelAlgoName(algo)] = entry;
    }

    json response = {
        {"total_samples",         report.total_samples},
        {"algorithms_calibrated", report.algorithms_calibrated},
        {"algorithm_stats",       algo_stats_json}
    };

    span.setAttribute("graph.total_samples",
        static_cast<int64_t>(report.total_samples));
    span.setAttribute("graph.algorithms_calibrated",
        static_cast<int64_t>(report.algorithms_calibrated));
    span.setStatus(true);
    return makeResponse(http::status::ok, response.dump(), req);
}

http::response<http::string_body> GraphApiHandler::handleCostModelExport(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleCostModelExport");
    span.setAttribute("http.method", "GET");
    span.setAttribute("http.path", "/api/v1/graph/cost-model");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    std::string model_json = optimizer_->exportCostModel();
    // Normalize legacy/empty exports to an object JSON so clients can round-trip
    // the payload without special-casing "null".
    auto parsed = json::parse(model_json, nullptr, false);
    if (parsed.is_discarded() || parsed.is_null() || !parsed.is_object()) {
        model_json = json::object().dump();
    }
    span.setStatus(true);
    return makeResponse(http::status::ok, model_json, req);
}

http::response<http::string_body> GraphApiHandler::handleCostModelImport(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleCostModelImport");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/api/v1/graph/cost-model");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }
    auto& optimizer = *optimizer_;

    auto parsed = json::parse(req.body(), nullptr, false);
    if (parsed.is_discarded()) {
        span.setStatus(false, "invalid cost model JSON");
        return makeErrorResponse(http::status::bad_request,
            "Invalid cost model JSON", req);
    }
    if (parsed.is_null()) {
        parsed = json::object();
    }
    if (!parsed.is_object()) {
        span.setStatus(false, "invalid cost model JSON type");
        return makeErrorResponse(http::status::bad_request,
            "Invalid cost model JSON", req);
    }

    if (!optimizer.importCostModel(parsed.dump())) {
        span.setStatus(false, "invalid cost model JSON");
        return makeErrorResponse(http::status::bad_request,
            "Invalid cost model JSON", req);
    }

    span.setStatus(true);
    json response = {{"imported", true}};
    return makeResponse(http::status::ok, response.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXPLAIN endpoint (Issue #1816): dry-run plan inspection for graph queries
// ─────────────────────────────────────────────────────────────────────────────

// Helper: build a QueryConstraints object from an optional JSON sub-object.
static themis::graph::GraphQueryOptimizer::QueryConstraints
parseQueryConstraints(const json& body, const std::string& key = "constraints") {
    themis::graph::GraphQueryOptimizer::QueryConstraints qc;
    if (!body.contains(key) || !body[key].is_object()) {
        return qc;
    }
    const auto& c = body[key];
    if (c.contains("max_depth") && c["max_depth"].is_number_integer()) {
        qc.max_depth = c["max_depth"].get<int>();
    }
    if (c.contains("max_results") && c["max_results"].is_number_unsigned()) {
        qc.max_results = c["max_results"].get<size_t>();
    }
    if (c.contains("edge_type") && c["edge_type"].is_string()) {
        qc.edge_type = c["edge_type"].get<std::string>();
    }
    if (c.contains("unique_vertices") && c["unique_vertices"].is_boolean()) {
        qc.unique_vertices = c["unique_vertices"].get<bool>();
    }
    if (c.contains("unique_edges") && c["unique_edges"].is_boolean()) {
        qc.unique_edges = c["unique_edges"].get<bool>();
    }
    if (c.contains("enable_parallel") && c["enable_parallel"].is_boolean()) {
        qc.enable_parallel = c["enable_parallel"].get<bool>();
    }
    if (c.contains("num_threads") && c["num_threads"].is_number_unsigned()) {
        qc.num_threads = c["num_threads"].get<uint32_t>();
    }
    if (c.contains("forbidden_vertices") && c["forbidden_vertices"].is_array()) {
        for (const auto& v : c["forbidden_vertices"]) {
            if (v.is_string()) {
              qc.forbidden_vertices.push_back(v.get<std::string>());
            }
        }
    }
    if (c.contains("required_vertices") && c["required_vertices"].is_array()) {
        for (const auto& v : c["required_vertices"]) {
            if (v.is_string()) {
              qc.required_vertices.push_back(v.get<std::string>());
            }
        }
    }
    if (c.contains("node_labels") && c["node_labels"].is_array()) {
        for (const auto& v : c["node_labels"]) {
            if (v.is_string()) {
              qc.node_labels.push_back(v.get<std::string>());
            }
        }
    }
    return qc;
}

// Helper: serialise an OptimizationPlan as a JSON object.
static json planToJson(
    const themis::graph::GraphQueryOptimizer::OptimizationPlan& plan,
    const std::string& explanation)
{
    using Algo = themis::graph::GraphQueryOptimizer::TraversalAlgorithm;
    using Pat  = themis::graph::GraphQueryOptimizer::QueryPattern;

    auto algo_name = [](Algo a) -> std::string {
        switch (a) {
            case Algo::BFS:           return "BFS";
            case Algo::DFS:           return "DFS";
            case Algo::BIDIRECTIONAL: return "BIDIRECTIONAL";
            case Algo::ASTAR:         return "ASTAR";
            case Algo::DIJKSTRA:      return "DIJKSTRA";
            default:                  return "UNKNOWN";
        }
    };

    auto pat_name = [](Pat p) -> std::string {
        switch (p) {
            case Pat::SHORTEST_PATH:       return "Shortest Path";
            case Pat::ALL_PATHS:           return "All Paths";
            case Pat::K_HOP_NEIGHBORS:     return "K-Hop Neighborhood";
            case Pat::PATTERN_MATCH:       return "Pattern Match";
            case Pat::REACHABILITY:        return "Reachability";
            case Pat::CONNECTED_COMPONENT: return "Connected Component";
            default:                       return "Unknown";
        }
    };

    json alt_arr = json::array();
    for (const auto& [alt_algo, alt_cost] : plan.alternatives) {
        alt_arr.push_back({{"algorithm", algo_name(alt_algo)}, {"estimated_cost", alt_cost}});
    }

    json shard_arr = json::array();
    for (const auto& sid : plan.shard_ids) {
        shard_arr.push_back(sid);
    }

    return json{
        {"algorithm",                algo_name(plan.algorithm)},
        {"pattern",                  pat_name(plan.pattern)},
        {"estimated_cost",           plan.estimated_cost},
        {"estimated_time_ms",        plan.estimated_time_ms},
        {"estimated_nodes_explored", plan.estimated_nodes_explored},
        {"use_index",                plan.use_index},
        {"use_cache",                plan.use_cache},
        {"early_termination",        plan.enable_early_termination},
        {"parallel_execution",       plan.enable_parallel},
        {"is_distributed",           plan.is_distributed},
        {"recommended_parallelism",  plan.recommended_parallelism},
        {"shard_ids",                shard_arr},
        {"alternatives",             alt_arr},
        {"explanation",              explanation}
    };
}

http::response<http::string_body> GraphApiHandler::handleQueryExplain(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleGraphQueryExplain");
    span.setAttribute("http.method", "POST");
    span.setAttribute("http.path", "/api/v1/graph/query/explain");

    if (!optimizer_) {
        span.setStatus(false, "optimizer not available");
        return makeErrorResponse(http::status::service_unavailable,
            "Graph optimizer not available", req);
    }

    json body_json;
    try {
        body_json = json::parse(req.body());
    } catch (const json::exception& e) {
        span.setStatus(false, "invalid JSON");
        return makeErrorResponse(http::status::bad_request,
            "Invalid JSON: " + std::string(e.what()), req);
    }

    if (!body_json.contains("query_type") || !body_json["query_type"].is_string()) {
        span.setStatus(false, "missing query_type");
        return makeErrorResponse(http::status::bad_request,
            "Missing required field 'query_type'", req);
    }

    const std::string query_type = body_json["query_type"].get<std::string>();
    span.setAttribute("graph.query_type", query_type);

    auto qc = parseQueryConstraints(body_json);

    try {
        if (query_type == "shortest_path" || query_type == "reachability") {
            if (!body_json.contains("start_vertex") || !body_json.contains("end_vertex")) {
                span.setStatus(false, "missing vertex fields");
                return makeErrorResponse(http::status::bad_request,
                    "Fields 'start_vertex' and 'end_vertex' are required for " + query_type, req);
            }
            const std::string sv = body_json["start_vertex"].get<std::string>();
            const std::string ev = body_json["end_vertex"].get<std::string>();
            span.setAttribute("graph.start_vertex", sv);
            span.setAttribute("graph.end_vertex", ev);

            auto result = (query_type == "shortest_path")
                ? optimizer_->optimizeShortestPath(sv, ev, qc)
                : optimizer_->optimizeReachability(sv, ev, qc);

            if (!result.has_value()) {
                span.setStatus(false, result.error().message());
                return makeErrorResponse(http::status::internal_server_error,
                    result.error().message(), req);
            }
            const auto& plan = result.value();
            span.setStatus(true);
            return makeResponse(http::status::ok,
                planToJson(plan, optimizer_->explainPlan(plan)).dump(), req);
        }

        if (query_type == "k_hop") {
            if (!body_json.contains("start_vertex")) {
                span.setStatus(false, "missing start_vertex");
                return makeErrorResponse(http::status::bad_request,
                    "Field 'start_vertex' is required for k_hop", req);
            }
            // max_depth may appear at top-level or inside constraints
            int k = 2;
            if (body_json.contains("max_depth") && body_json["max_depth"].is_number_integer()) {
                k = body_json["max_depth"].get<int>();
            } else if (qc.max_depth.has_value()) {
                k = *qc.max_depth;
            }
            const std::string sv = body_json["start_vertex"].get<std::string>();
            span.setAttribute("graph.start_vertex", sv);
            span.setAttribute("graph.k", static_cast<int64_t>(k));

            auto result = optimizer_->optimizeKHopNeighborhood(sv, k, qc);
            if (!result.has_value()) {
                span.setStatus(false, result.error().message());
                return makeErrorResponse(http::status::internal_server_error,
                    result.error().message(), req);
            }
            const auto& plan = result.value();
            span.setStatus(true);
            return makeResponse(http::status::ok,
                planToJson(plan, optimizer_->explainPlan(plan)).dump(), req);
        }

        if (query_type == "pattern_match") {
            if (!body_json.contains("pattern_vertices") || !body_json["pattern_vertices"].is_array()) {
                span.setStatus(false, "missing pattern_vertices");
                return makeErrorResponse(http::status::bad_request,
                    "Field 'pattern_vertices' (array) is required for pattern_match", req);
            }
            std::vector<std::string> pverts;
            for (const auto& pv : body_json["pattern_vertices"]) {
                if (pv.is_string()) {
                  pverts.push_back(pv.get<std::string>());
                }
            }
            std::vector<std::pair<std::string, std::string>> pedges;
            if (body_json.contains("pattern_edges") && body_json["pattern_edges"].is_array()) {
                for (const auto& pe : body_json["pattern_edges"]) {
                    if (pe.is_array() && pe.size() == 2 &&
                        pe[0].is_string() && pe[1].is_string())
                    {
                        pedges.emplace_back(pe[0].get<std::string>(), pe[1].get<std::string>());
                    }
                }
            }
            span.setAttribute("graph.pattern_vertices",
                static_cast<int64_t>(pverts.size()));

            auto result = optimizer_->optimizePatternMatch(pverts, pedges, qc);
            if (!result.has_value()) {
                span.setStatus(false, result.error().message());
                return makeErrorResponse(http::status::internal_server_error,
                    result.error().message(), req);
            }
            const auto& plan = result.value();
            span.setStatus(true);
            return makeResponse(http::status::ok,
                planToJson(plan, optimizer_->explainPlan(plan)).dump(), req);
        }

        if (query_type == "constrained_path") {
            if (!body_json.contains("start_vertex") || !body_json.contains("end_vertex")) {
                span.setStatus(false, "missing vertex fields");
                return makeErrorResponse(http::status::bad_request,
                    "Fields 'start_vertex' and 'end_vertex' are required for constrained_path", req);
            }
            const std::string sv = body_json["start_vertex"].get<std::string>();
            const std::string ev = body_json["end_vertex"].get<std::string>();
            span.setAttribute("graph.start_vertex", sv);
            span.setAttribute("graph.end_vertex", ev);

            if (!graph_index_) {
                span.setStatus(false, "graph index not available");
                return makeErrorResponse(http::status::service_unavailable,
                    "Graph index not available", req);
            }
            themis::graph::PathConstraints pc(graph_index_.get());
            if (body_json.contains("path_constraints") && body_json["path_constraints"].is_object()) {
                const auto& pco = body_json["path_constraints"];
                if (pco.contains("min_length") && pco["min_length"].is_number_integer()) {
                    pc.addMinLength(pco["min_length"].get<int>());
                }
                if (pco.contains("max_length") && pco["max_length"].is_number_integer()) {
                    pc.addMaxLength(pco["max_length"].get<int>());
                }
                if (pco.contains("forbidden_vertices") && pco["forbidden_vertices"].is_array()) {
                    for (const auto& fv : pco["forbidden_vertices"]) {
                        if (fv.is_string()) {
                          pc.addForbiddenNode(fv.get<std::string>());
                        }
                    }
                }
                if (pco.contains("required_vertices") && pco["required_vertices"].is_array()) {
                    for (const auto& rv : pco["required_vertices"]) {
                        if (rv.is_string()) {
                          pc.addRequiredNode(rv.get<std::string>());
                        }
                    }
                }
                if (pco.contains("unique_nodes") && pco["unique_nodes"].is_boolean() &&
                    pco["unique_nodes"].get<bool>()) {
                    pc.requireUniqueNodes();
                }
                if (pco.contains("acyclic") && pco["acyclic"].is_boolean() &&
                    pco["acyclic"].get<bool>()) {
                    pc.requireAcyclic();
                }
            }

            auto result = optimizer_->explainConstrainedPath(sv, ev, pc);
            if (!result.has_value()) {
                span.setStatus(false, result.error().message());
                return makeErrorResponse(http::status::internal_server_error,
                    result.error().message(), req);
            }
            const auto& plan = result.value();
            span.setStatus(true);
            return makeResponse(http::status::ok,
                planToJson(plan, optimizer_->explainPlan(plan)).dump(), req);
        }

        span.setStatus(false, "unknown query_type");
        return makeErrorResponse(http::status::bad_request,
            "Unknown query_type '" + query_type +
            "'. Supported: shortest_path, k_hop, reachability, pattern_match, constrained_path",
            req);

    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false);
        THEMIS_ERROR("Graph query explain error: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> GraphApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following AdminApiHandler pattern
    json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> GraphApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // Helper implementation following AdminApiHandler pattern
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis


