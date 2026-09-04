/**
 * @file error_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/error_api_handler.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

void ErrorApiHandler::handleGetErrors(const Request& req, Response& res) {
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // Check if category filter is provided
    std::string category = {};
    if (req.query.contains("category")) {
    auto span = Tracer::startSpan("handleGetErrors");
        category = req.query["category"].get<std::string>();
    }
    
    json response = {};
    
    if (!category.empty()) {
        auto errors = registry.getErrorsByCategory(category);
        json errors_json = json::array();
        
        for (const auto& error : errors) {
            errors_json.push_back(error.toJSON());
        }
        
        response = {
            {"status", "success"},
            {"category", category},
            {"errors", errors_json},
            {"count", errors.size()}
        };
    } else {
        response = registry.toJSON();
        response["status"] = "success";
    }
    
    res.status_code = 200;
    res.setJSON(response);
}

void ErrorApiHandler::handleGetError(const Request& req, Response& res) {
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // Extract error code from params
    if (!req.params.contains("code")) {
    auto span = Tracer::startSpan("handleGetError");
        res.status_code = 400;
        res.setJSON({
            {"status", "error"},
            {"message", "Error code parameter is required"}
        });
        return;
    }
    
    try {
        int code = std::stoi(req.params["code"].get<std::string>());
        auto metadata = registry.getError(static_cast<errors::ErrorCode>(code));
        
        if (static_cast<int>(metadata.code) == static_cast<int>(errors::ErrorCode::ERR_UNKNOWN) && 
            code != static_cast<int>(errors::ErrorCode::ERR_UNKNOWN)) {
            // Error not found
            res.status_code = 404;
            res.setJSON({
                {"status", "not_found"},
                {"message", "Error code not registered"},
                {"code", code}
            });
        } else {
            res.status_code = 200;
            res.setJSON({
                {"status", "success"},
                {"error", metadata.toJSON()}
            });
        }
    } catch (const std::exception& e) {
        res.status_code = 400;
        res.setJSON({
            {"status", "error"},
            {"message", "Invalid error code format"},
            {"details", e.what()}
        });
    }
}

void ErrorApiHandler::handleGetCategories(const Request& /*req*/, Response& res) {
    auto span = Tracer::startSpan("handleGetCategories");
    auto& registry = errors::ErrorRegistry::getInstance();
    auto categories = registry.getAllCategories();
    
    json response = {
        {"status", "success"},
        {"categories", categories},
        {"count", categories.size()}
    };
    
    res.status_code = 200;
    res.setJSON(response);
}

void ErrorApiHandler::handleSearchErrors(const Request& req, Response& res) {
    auto span = Tracer::startSpan("handleSearchErrors");
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // Get search query
    std::string query = {};
    if (req.query.contains("q")) {
        query = req.query["q"].get<std::string>();
    }
    
    if (query.empty()) {
        res.status_code = 400;
        res.setJSON({
            {"status", "error"},
            {"message", "Search query parameter 'q' is required"}
        });
        return;
    }
    
    auto results = registry.searchErrors(query);
    json errors_json = json::array();
    
    for (const auto& error : results) {
        errors_json.push_back(error.toJSON());
    }
    
    json response = {
        {"status", "success"},
        {"query", query},
        {"errors", errors_json},
        {"count", results.size()}
    };
    
    res.status_code = 200;
    res.setJSON(response);
}

} // namespace server
} // namespace themis

