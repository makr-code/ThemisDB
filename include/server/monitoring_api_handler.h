#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class SecondaryIndexManager;

namespace sharding {
class PrometheusMetrics;
class SLOMonitor;
}

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class ShardingMetricsHandler;

} // namespace server
} // namespace themis

namespace themis {
class AuthMiddleware;
class SchemaManager;
} // namespace themis

namespace themis {
namespace server {

/**
 * @brief Handler for Monitoring and System Information
 * 
 * This handler manages all monitoring and system information endpoints:
 * - GET / or GET /health - Health check endpoint
 * - GET /version - Get version information
 * - GET /stats - Get runtime statistics
 * - GET /api/capabilities - Get server capabilities
 * - GET /metrics - Get Prometheus-compatible metrics
 * - GET /config or POST /config - Get/update server configuration
 * 
 * Features:
 * - Health monitoring
 * - Version information
 * - Runtime statistics
 * - Prometheus metrics export
 * - Dynamic configuration
 * 
 * Extracted from http_server.cpp (~300 lines) to improve maintainability.
 */
class MonitoringApiHandler {
public:
    /**
     * @brief Construct a new Monitoring API Handler
     * 
     * @param storage Storage backend
     * @param auth Authentication/authorization middleware  
     * @param request_count Request counter (shared atomic)
     * @param error_count Error counter (shared atomic)
     * @param start_time Server start time (shared)
     * @param secondary_index Secondary index manager (for stats)
     * @param schema_manager Schema manager (for capabilities, optional)
     * @param sharding_metrics Sharding metrics handler (for metrics/slo endpoints, optional)
     */
    MonitoringApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<AuthMiddleware> auth,
        std::atomic<uint64_t>* request_count,
        std::atomic<uint64_t>* error_count,
        const std::chrono::steady_clock::time_point* start_time,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        ::themis::SchemaManager* schema_manager = nullptr,
        std::shared_ptr<ShardingMetricsHandler> sharding_metrics = nullptr,
        const std::atomic<bool>* is_running = nullptr,
        const std::atomic<uint64_t>* active_requests = nullptr,
        const std::atomic<uint64_t>* active_connections = nullptr
    );

    /**
     * @brief Handle GET / or GET /health request
     * @param req HTTP request
     * @return HTTP response with health status
     */
    http::response<http::string_body> handleHealthCheck(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /health/live request (liveness probe)
     * Returns 200 if server is running, 503 otherwise.
     * @param req HTTP request
     * @return HTTP response with liveness status
     */
    http::response<http::string_body> handleLiveness(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /health/ready request (readiness probe)
     * Returns 200 if storage is accessible and server is ready to serve traffic.
     * Returns 503 if not ready (e.g. storage unavailable, still starting up).
     * Reports per-layer status (storage, connections, memory).
     * @param req HTTP request
     * @return HTTP response with readiness status
     */
    http::response<http::string_body> handleReadiness(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/openapi.json request
     * Returns an OpenAPI 3.0 specification describing the ThemisDB REST API.
     * @param req HTTP request
     * @return HTTP response with OpenAPI 3.0 JSON document
     */
    http::response<http::string_body> handleOpenApi(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /version request
     * @param req HTTP request
     * @return HTTP response with version information
     */
    http::response<http::string_body> handleVersion(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /stats request
     * @param req HTTP request
     * @return HTTP response with runtime statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /api/capabilities request
     * @param req HTTP request
     * @return HTTP response with server capabilities
     */
    http::response<http::string_body> handleCapabilities(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /metrics request (Prometheus format)
     * @param req HTTP request
     * @return HTTP response with metrics in Prometheus format
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /api/plugins/metrics request
     * @param req HTTP request
     * @return HTTP response with plugin metrics in JSON format
     */
    http::response<http::string_body> handlePluginMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /metrics/sharding request (Sharding metrics in Prometheus format)
     * @param req HTTP request
     * @return HTTP response with sharding metrics
     */
    http::response<http::string_body> handleShardingMetrics(const http::request<http::string_body>& req);
    
    /**
     * @brief Handle GET /slo or GET /api/slo request (SLO status in JSON)
     * @param req HTTP request
     * @return HTTP response with SLO compliance and error budgets
     */
    http::response<http::string_body> handleSLOStatus(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<AuthMiddleware> auth_;
    std::atomic<uint64_t>* request_count_;
    std::atomic<uint64_t>* error_count_;
    const std::chrono::steady_clock::time_point* start_time_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    ::themis::SchemaManager* schema_manager_;
    std::shared_ptr<ShardingMetricsHandler> sharding_metrics_;
    const std::atomic<bool>* is_running_{nullptr};
    const std::atomic<uint64_t>* active_requests_{nullptr};
    const std::atomic<uint64_t>* active_connections_{nullptr};

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
