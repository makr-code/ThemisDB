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
        {"error_rate",              m.errorRate()}
    };

    span.setAttribute("graph.total_queries",
        static_cast<int64_t>(m.total_queries.load(std::memory_order_relaxed)));
    span.setStatus(true);
    return makeResponse(http::status::ok, response.dump(), req);
}

std::string GraphApiHandler::extractPathParam(const std::string& target, const std::string& prefix) {
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
