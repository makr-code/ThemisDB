/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_api_handler.cpp                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     435                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0aa583b3a  2026-02-21  Add crash recovery & robustness fixes    ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/graph_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "index/graph_index.h"
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
        size_t max_depth = body_json["max_depth"];
        
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
            if (from_opt) edge_from = std::move(*from_opt);
            if (to_opt)   edge_to   = std::move(*to_opt);
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
          "Fraction of graph queries that failed (0.0–1.0)",
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
    body += std::to_string(cumulative); body += '\n';
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
    if (target.size() <= prefix.size()) return "";
    if (target.substr(0, prefix.size()) != prefix) return "";
    
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
GraphApiHandler::parseChangeSet(const json& changes_array) {
    using CS = themis::graph::GraphQueryOptimizer::GraphChangeSet;
    CS cs;
    if (!changes_array.is_array()) return cs;

    for (const auto& item : changes_array) {
        if (!item.contains("type") || !item.contains("id")) continue;
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
        if (init_bfs) initial = init_bfs.value();

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

    try {
        json body_json = json::parse(req.body());

        if (!body_json.contains("changes") || !body_json["changes"].is_array()) {
            span.setStatus(false, "missing changes array");
            return makeErrorResponse(http::status::bad_request,
                "Missing or invalid 'changes' array", req);
        }

        auto cs = parseChangeSet(body_json["changes"]);
        const size_t reexecuted = optimizer_->onGraphChange(cs);

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
