/**
 * @file error_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
    
    /**
     * @brief Set JSON.
     * @param[in] j Input parameter.
     * @details Implements setJSON without additional internal calls.
     */
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
    /**
     * @brief Handle Get Errors.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetErrors(const Request& req, Response& res);
    /**
     * @brief Handle Get Error.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetError(const Request& req, Response& res);
    /**
     * @brief Handle Get Categories.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetCategories(const Request& req, Response& res);
    /**
     * @brief Handle Search Errors.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleSearchErrors(const Request& req, Response& res);
};

} // namespace server
} // namespace themis
