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

/*
 * ThemisDB | File: error_api_handler.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
