/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_api_handler.h                                ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     171                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
#include "server/auth_middleware.h"
#include "graph/graph_query_optimizer.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class GraphIndexManager;

namespace server {

/**
 * @brief Handler for Graph Operations
 * 
 * This handler manages all graph-related endpoints:
 * - POST /graph/traverse - Execute graph traversal query
 * - POST /graph/edge - Create a graph edge
 * - DELETE /graph/edge/:id - Delete a graph edge
 * - GET /api/v1/graph/metrics - Aggregate query metrics (observability)
 * 
 * Features:
 * - Graph traversal algorithms (BFS, DFS, shortest path)
 * - Edge creation and deletion
 * - Property graph support
 * - Path finding and pattern matching
 * - Observability metrics export for Prometheus/OTel
 * 
 * Extracted from http_server.cpp (~150 lines) to improve maintainability.
 */
class GraphApiHandler {
public:
    /**
     * @brief Construct a new Graph API Handler
     * 
     * @param storage Storage backend
     * @param graph_index Graph index manager
     * @param auth Authentication/authorization middleware
     */
    GraphApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /graph/traverse request
     * @param req HTTP request with traversal specification
     * @return HTTP response with traversal results
     */
    http::response<http::string_body> handleTraverse(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /graph/edge request
     * @param req HTTP request with edge data
     * @return HTTP response with creation status
     */
    http::response<http::string_body> handleEdgeCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /graph/edge/:id request
     * @param req HTTP request
     * @return HTTP response with deletion status
     */
    http::response<http::string_body> handleEdgeDelete(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/graph/metrics request
     *
     * Returns a JSON snapshot of the cumulative query-level observability
     * metrics collected by the internal GraphQueryOptimizer:
     *
     * ```json
     * {
     *   "total_queries": 42,
     *   "failed_queries": 1,
     *   "timed_out_queries": 0,
     *   "total_execution_time_ms": 350.5,
     *   "max_execution_time_ms": 12.3,
     *   "avg_execution_time_ms": 8.35,
     *   "total_nodes_explored": 1234,
     *   "total_edges_traversed": 5678,
     *   "plan_cache_hits": 30,
     *   "plan_cache_misses": 12,
     *   "error_rate": 0.0238
     * }
     * ```
     *
     * @param req HTTP GET request
     * @return HTTP 200 response with JSON metrics body
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/graph/metrics/prometheus request
     *
     * Returns metrics in Prometheus text exposition format (text/plain; version=0.0.4)
     * so that a Prometheus server can scrape this endpoint directly.
     *
     * Example output:
     * ```
     * # HELP themis_graph_queries_total Total graph traversal executions
     * # TYPE themis_graph_queries_total counter
     * themis_graph_queries_total 42
     * ...
     * # HELP themis_graph_latency_ms Latency histogram of graph query execution
     * # TYPE themis_graph_latency_ms histogram
     * themis_graph_latency_ms_bucket{le="1"} 5
     * ...
     * themis_graph_latency_ms_bucket{le="+Inf"} 42
     * themis_graph_latency_ms_sum 350
     * themis_graph_latency_ms_count 42
     * ```
     *
     * @param req HTTP GET request
     * @return HTTP 200 response with Prometheus text body
     */
    http::response<http::string_body> handleMetricsPrometheus(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    /// Optimizer instance used for query execution and metrics collection.
    /// Owned by this handler; lazily used for metrics export.
    std::unique_ptr<themis::graph::GraphQueryOptimizer> optimizer_;

    // Helper methods
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
