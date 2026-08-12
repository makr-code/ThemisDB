/**
 * @file graph_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"
#include "graph/graph_query_optimizer.h"

#include <memory>
#include <string>
#include <unordered_map>
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
     *   "plan_cache_evictions": 5,
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

    /**
     * @brief Handle POST /graph/query/incremental request
     *
     * Registers a BFS query for incremental re-execution when the graph changes.
     * The query is executed immediately to capture the initial result set.
     *
     * Request body:
     * ```json
     * {
     *   "start_vertex": "A",
     *   "max_depth": 3,
     *   "edge_type": "knows",     // optional
     *   "max_results": 100        // optional
     * }
     * ```
     *
     * Response:
     * ```json
     * {
     *   "handle": 1,
     *   "initial_results": ["B", "C", "D"]
     * }
     * ```
     *
     * @param req HTTP request with registration parameters
     * @return HTTP 201 response with handle and initial results
     */
    http::response<http::string_body> handleIncrementalQueryRegister(const http::request<http::string_body>& req);

    /**
     * @brief Handle DELETE /graph/query/incremental/:handle request
     *
     * Unregisters a previously registered incremental query identified by
     * its numeric handle.
     *
     * @param req HTTP DELETE request with handle in the path
     * @return HTTP 200 on success, 404 if the handle is not found
     */
    http::response<http::string_body> handleIncrementalQueryUnregister(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /graph/changes request
     *
     * Notifies the graph query optimizer that the graph has changed and
     * triggers re-execution of all affected registered incremental queries.
     *
     * Request body:
     * ```json
     * {
     *   "changes": [
     *     {"type": "edge_added",    "id": "e1", "from": "A", "to": "B"},
     *     {"type": "edge_removed",  "id": "e2", "from": "B", "to": "C"},
     *     {"type": "vertex_added",  "id": "D"},
     *     {"type": "vertex_removed","id": "E"}
     *   ]
     * }
     * ```
     *
     * Response:
     * ```json
     * { "queries_reexecuted": 2, "changes_applied": 4 }
     * ```
     *
     * @param req HTTP request with change set
     * @return HTTP 200 response with reexecution count
     */
    http::response<http::string_body> handleGraphChanges(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/graph/cost-model/calibrate request
     *
     * Triggers a batch recalibration of per-algorithm cost models from the
     * full execution history recorded since the optimizer was started.
     * Re-seeds each algorithm's EMA cost with the historical mean when at
     * least MIN_CALIBRATION_SAMPLES observations are available.
     *
     * Response:
     * ```json
     * {
     *   "total_samples": 42,
     *   "algorithms_calibrated": 2,
     *   "algorithm_stats": {
     *     "BFS": {
     *       "mean_execution_ms": 5.2,
     *       "stddev_execution_ms": 1.3,
     *       "min_execution_ms": 3.1,
     *       "max_execution_ms": 8.5,
     *       "sample_count": 15,
     *       "mean_estimated_ms": 6.0,
     *       "mean_absolute_error_ms": 0.8,
     *       "cost_ratio": 1.15,
     *       "estimation_sample_count": 15
     *     }
     *   }
     * }
     * ```
     *
     * @param req HTTP POST request (body ignored)
     * @return HTTP 200 response with calibration report JSON
     */
    http::response<http::string_body> handleCostModelCalibrate(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/v1/graph/cost-model request
     *
     * Exports the current per-algorithm learned cost model as a JSON object.
     * The exported JSON can be persisted and imported into a new optimizer
     * instance via POST /api/v1/graph/cost-model.
     *
     * Response:
     * ```json
     * {
     *   "BFS":           {"ema_cost_ms": 8.1, "exec_count": 42, "confidence": 0.42},
     *   "DFS":           {"ema_cost_ms": 6.5, "exec_count": 10, "confidence": 0.10},
     *   "DIJKSTRA":      {"ema_cost_ms": 12.3,"exec_count": 5,  "confidence": 0.05},
     *   "ASTAR":         {"ema_cost_ms": 9.7, "exec_count": 3,  "confidence": 0.03},
     *   "BIDIRECTIONAL": {"ema_cost_ms": 7.2, "exec_count": 8,  "confidence": 0.08}
     * }
     * ```
     *
     * @param req HTTP GET request
     * @return HTTP 200 response with cost model JSON
     */
    http::response<http::string_body> handleCostModelExport(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/graph/cost-model request
     *
     * Imports a previously exported cost model JSON to seed the optimizer
     * with pre-learned data.  Unknown algorithm keys are silently ignored;
     * malformed JSON returns HTTP 400.
     *
     * Request body: JSON object as returned by GET /api/v1/graph/cost-model
     *
     * Response on success:
     * ```json
     * { "imported": true }
     * ```
     *
     * @param req HTTP POST request with cost model JSON body
     * @return HTTP 200 on success, 400 on invalid JSON
     */
    http::response<http::string_body> handleCostModelImport(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /api/v1/graph/query/explain request (Issue: #1816)
     *
     * Performs a dry-run optimization for a graph query and returns the
     * selected algorithm, cost estimates, and a human-readable explanation
     * without actually executing the traversal.
     *
     * Supported query types: `shortest_path`, `k_hop`, `reachability`,
     * `pattern_match`, `constrained_path`.
     *
     * Request body:
     * ```json
     * {
     *   "query_type": "shortest_path",
     *   "start_vertex": "A",
     *   "end_vertex": "D",
     *   "constraints": {
     *     "max_depth": 5,
     *     "edge_type": "knows",
     *     "unique_vertices": false,
     *     "unique_edges": false,
     *     "enable_parallel": false
     *   }
     * }
     * ```
     *
     * For `k_hop` and `reachability`, use `"max_depth"` in the top-level
     * body (or inside `"constraints"`).  For `pattern_match`, supply
     * `"pattern_vertices"` (array of strings) and `"pattern_edges"`
     * (array of `[from, to]` pairs).  For `constrained_path`, the optional
     * `"path_constraints"` object supports `min_length`, `max_length`,
     * `forbidden_vertices`, `required_vertices`, `unique_nodes`, and
     * `acyclic` fields.
     *
     * Response:
     * ```json
     * {
     *   "algorithm": "BFS",
     *   "pattern": "Shortest Path",
     *   "estimated_cost": 42.5,
     *   "estimated_time_ms": 8.3,
     *   "estimated_nodes_explored": 150,
     *   "use_index": true,
     *   "use_cache": false,
     *   "early_termination": true,
     *   "parallel_execution": false,
     *   "is_distributed": false,
     *   "alternatives": [{"algorithm": "DFS", "estimated_cost": 65.2}],
     *   "explanation": "Query Pattern: Shortest Path\n..."
     * }
     * ```
     *
     * @param req HTTP POST request with query specification
     * @return HTTP 200 with plan JSON, 400 on invalid input, 503 if optimizer unavailable
     */
    http::response<http::string_body> handleQueryExplain(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    /// Optimizer instance used for query execution and metrics collection.
    /// Owned by this handler; lazily used for metrics export.
    std::unique_ptr<themis::graph::GraphQueryOptimizer> optimizer_;

    /// Server-side cache of the latest IncrementalQueryResult per registered
    /// query handle.  Updated by the callback installed in
    /// handleIncrementalQueryRegister() whenever onGraphChange() fires.
    /// Accessed only from the HTTP request thread (single-threaded handler),
    /// so no mutex is required.
    std::unordered_map<
        themis::graph::GraphQueryOptimizer::IncrementalQueryHandle,
        themis::graph::GraphQueryOptimizer::IncrementalQueryResult
    > incremental_results_;

    /// Parse a GraphChangeSet from a JSON array of change objects.
    /// Returns an empty change set on parse error.
    static themis::graph::GraphQueryOptimizer::GraphChangeSet
    parseChangeSet(const nlohmann::json& changes_array);

    // Helper methods
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
