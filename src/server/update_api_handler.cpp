/**
 * @file update_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/update_api_handler.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include "utils/tracing.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)

namespace themis {
namespace server {

using json = nlohmann::json;

UpdateApiHandler::UpdateApiHandler(std::shared_ptr<utils::UpdateChecker> checker)
    : checker_(std::move(checker)) {
}

http::response<http::string_body> UpdateApiHandler::handleRequest(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan([[maybe_unused]] "handleRequest");
    std::string target = std::string(req.target());
    auto method = req.method();
    
    // Route to appropriate handler
    if (target == "/api/updates" && method == http::verb::get) {
        return handleGetStatus(req);
    } else if (target == "/api/updates/check" && method == http::verb::post) {
        return handleCheckNow(req);
    } else if (target == "/api/updates/config" && method == http::verb::get) {
        return handleGetConfig(req);
    } else if (target == "/api/updates/config" && method == http::verb::put) {
        return handleUpdateConfig(req);
    } else {
        return createErrorResponse(
            http::status::not_found,
            "Unknown update API endpoint",
            req
        );
    }
}

http::response<http::string_body> UpdateApiHandler::handleGetStatus(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetStatus");
    try {
        auto result = checker_->getLastResult();
        auto response_json = result.toJson();
        
        return createJsonResponse(http::status::ok, response_json, req);
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting update status: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            std::string("Failed to get update status: ") + e.what(),
            req
        );
    }
}

http::response<http::string_body> UpdateApiHandler::handleCheckNow(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleCheckNow");
    try {
        LOG_INFO("Manual update check triggered via API");
        
        // Perform check (blocking)
        auto result = checker_->checkNow();
        auto response_json = result.toJson();
        
        return createJsonResponse(http::status::ok, response_json, req);
    } catch (const std::exception& e) {
        LOG_ERROR("Error checking for updates: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            std::string("Failed to check for updates: ") + e.what(),
            req
        );
    }
}

http::response<http::string_body> UpdateApiHandler::handleGetConfig(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetConfig");
    try {
        auto config = checker_->getConfig();
        auto config_json = config.toJson();
        
        // Add runtime status
        config_json["is_running"] = checker_->isRunning();
        
        return createJsonResponse(http::status::ok, config_json, req);
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting update checker config: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            std::string("Failed to get config: ") + e.what(),
            req
        );
    }
}

http::response<http::string_body> UpdateApiHandler::handleUpdateConfig(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleUpdateConfig");
    try {
        // Parse request body
        json request_json;
        try {
            request_json = json::parse(req.body());
        } catch (const json::exception& e) {
            return createErrorResponse(
                http::status::bad_request,
                std::string("Invalid JSON: ") + e.what(),
                req
            );
        }
        
        // Parse new config
        auto new_config = utils::UpdateCheckerConfig::fromJson(request_json);
        
        // Update configuration
        checker_->updateConfig(new_config);
        
        LOG_INFO("Update checker configuration updated via API");
        
        // Return new config
        json response_json;
        response_json["success"] = true;
        response_json["message"] = "Configuration updated successfully";
        response_json["config"] = new_config.toJson();
        
        return createJsonResponse(http::status::ok, response_json, req);
    } catch (const std::exception& e) {
        LOG_ERROR("Error updating config: {}", e.what());
        return createErrorResponse(
            http::status::internal_server_error,
            std::string("Failed to update config: ") + e.what(),
            req
        );
    }
}

http::response<http::string_body> UpdateApiHandler::createJsonResponse(
    http::status status,
    const json& body,
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("createJsonResponse");
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.body() = body.dump(2);  // Pretty print with indent=2
    res.prepare_payload();
    res.keep_alive(req.keep_alive());
    return res;
}

http::response<http::string_body> UpdateApiHandler::createErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    json error_json;
    error_json["error"] = message;
    error_json["status"] = static_cast<int>(status);
    
    return createJsonResponse(status, error_json, req);
}

} // namespace server
} // namespace themis

