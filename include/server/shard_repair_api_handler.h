/**
 * @file shard_repair_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace sharding {
class ShardRepairEngine;
}

namespace server {

/**
 * @brief ShardRepairApiHandler - Shard consistency and repair operations.
 * 
 * HTTP API handler for shard consistency and repair operations.
 * Implements endpoint-specific routing, request validation, business logic,
 * and response formatting.
 * 
 * ### HTTP Endpoints
 * Supported operations depend on the specific handler implementation.
 * See handler methods for endpoint mappings and request/response schemas.
 * 
 * ### Thread Safety
 * Handler instance and all methods are thread-safe for concurrent requests.
 * Internal state modifications use appropriate synchronization primitives.
 * 
 * ### Error Handling
 * All endpoints follow consistent error response formatting:
 * - 400: Bad Request (invalid input)
 * - 401: Unauthorized (missing/invalid authentication)
 * - 403: Forbidden (insufficient permissions)
 * - 404: Not Found (resource doesn't exist)
 * - 500: Internal Server Error (unexpected failure)
 * 
 * @note Integrates with rate limiting, auth middleware, and validation pipeline
 * @note Request bodies are validated against JSON schemas before processing
 * @note All operations are auditable and logged
 */

class ShardRepairApiHandler {
public:
    ShardRepairApiHandler(
        std::shared_ptr<sharding::ShardRepairEngine> repair_engine,
        std::shared_ptr<themis::AuthMiddleware> auth);

    void setRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> repair_engine);

    http::response<http::string_body> handleHealth(const http::request<http::string_body>& req);
    http::response<http::string_body> handleTriggerRepair(const http::request<http::string_body>& req);
    http::response<http::string_body> handleTriggerFullScan(const http::request<http::string_body>& req);
    http::response<http::string_body> handleJobStatus(const http::request<http::string_body>& req);
    http::response<http::string_body> handleDashboard(const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    bool checkAuth(const http::request<http::string_body>& req,
                   const std::string& required_scope,
                   http::response<http::string_body>& out) const;

    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const std::string& content_type,
        const http::request<http::string_body>& req) const;
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis