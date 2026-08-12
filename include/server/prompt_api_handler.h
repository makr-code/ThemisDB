/**
 * @file prompt_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace prompt_engineering {
class PromptManager;
}

namespace server {

/**
 * @brief Handler for Prompt Template Operations
 * 
 * This handler manages all prompt template-related endpoints:
 * - POST /prompt_template - Create a new prompt template
 * - GET /prompt_template - List all prompt templates
 * - GET /prompt_template/:id - Get a specific prompt template
 * - PUT /prompt_template/:id - Update a prompt template
 * 
 * Features:
 * - Prompt template management for LLM operations
 * - Template versioning
 * - Variable substitution support
 * - Template metadata and tagging
 * 
 * Extracted from http_server.cpp (~250 lines) to improve maintainability.
 */
class PromptApiHandler {
public:
    /**
     * @brief Construct a new Prompt API Handler
     * 
     * @param storage Storage backend
     * @param prompt_manager Prompt template manager
     * @param auth Authentication/authorization middleware
     */
    PromptApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<prompt_engineering::PromptManager> prompt_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /prompt_template request
     * @param req HTTP request with template data
     * @return HTTP response with creation status and template ID
     */
    http::response<http::string_body> handlePost(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /prompt_template request (list all)
     * @param req HTTP request
     * @return HTTP response with list of templates
     */
    http::response<http::string_body> handleList(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /prompt_template/:id request
     * @param req HTTP request
     * @return HTTP response with template data
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /prompt_template/:id request
     * @param req HTTP request with updated template data
     * @return HTTP response with update status
     */
    http::response<http::string_body> handlePut(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<prompt_engineering::PromptManager> prompt_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
