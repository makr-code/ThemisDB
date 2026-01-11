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
