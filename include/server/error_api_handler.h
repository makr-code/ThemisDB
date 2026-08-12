/**
 * @file error_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>

namespace themis {
namespace server {

// Forward declarations to avoid circular dependencies
struct Request {
    std::string method;
    std::string path;
    nlohmann::json params;
    nlohmann::json query;
    nlohmann::json body;
};

struct Response {
    int status_code = 200;
    nlohmann::json body;
    std::string content_type = "application/json";
    
    void setJSON(const nlohmann::json& j) {
        body = j;
        content_type = "application/json";
    }
};

// Simple route handler type
using RouteHandler = std::function<void(const Request&, Response&)>;

/**
 * @brief ErrorApiHandler - Error response and diagnostic API operations.
 * 
 * HTTP API handler for error response and diagnostic API operations.
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

class ErrorApiHandler {
public:
    ErrorApiHandler() = default;
    
    // Handler methods
    void handleGetErrors(const Request& req, Response& res);
    void handleGetError(const Request& req, Response& res);
    void handleGetCategories(const Request& req, Response& res);
    void handleSearchErrors(const Request& req, Response& res);
};

} // namespace server
} // namespace themis
