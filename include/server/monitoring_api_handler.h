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

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

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
     */
    MonitoringApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<AuthMiddleware> auth,
        std::atomic<uint64_t>* request_count,
        std::atomic<uint64_t>* error_count,
        const std::chrono::steady_clock::time_point* start_time,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        ::themis::SchemaManager* schema_manager = nullptr
    );

    /**
     * @brief Handle GET / or GET /health request
     * @param req HTTP request
     * @return HTTP response with health status
     */
    http::response<http::string_body> handleHealthCheck(const http::request<http::string_body>& req);

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

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<AuthMiddleware> auth_;
    std::atomic<uint64_t>* request_count_;
    std::atomic<uint64_t>* error_count_;
    const std::chrono::steady_clock::time_point* start_time_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    ::themis::SchemaManager* schema_manager_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
