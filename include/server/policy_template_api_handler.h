/**
 * @file policy_template_api_handler.h
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
#include "governance/policy_template.h"
#include "governance/policy_manager.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief Handler for Policy Template API
 * 
 * This handler manages policy template endpoints:
 * - GET /policies/templates - List available templates
 * - GET /policies/templates/:id - Get template details
 * - POST /policies/templates/:id/instantiate - Create rule from template
 * - POST /policies/templates/:id/preview - Preview without creating
 */
class PolicyTemplateApiHandler {
public:
    /**
     * @brief Construct a new Policy Template API Handler
     * 
     * @param template_manager PolicyTemplateManager instance
     * @param policy_manager PolicyManager for rule creation
     * @param auth Authentication/authorization middleware
     */
    PolicyTemplateApiHandler(
        std::shared_ptr<themis::governance::PolicyTemplateManager> template_manager,
        std::shared_ptr<themis::governance::PolicyManager> policy_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle GET /policies/templates - List all templates
     * @param req HTTP request
     * @return HTTP response with JSON array of templates
     */
    http::response<http::string_body> handleListTemplates(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle GET /policies/templates/:id - Get template details
     * @param req HTTP request
     * @param template_id Template identifier
     * @return HTTP response with JSON template object
     */
    http::response<http::string_body> handleGetTemplate(
        const http::request<http::string_body>& req,
        const std::string& template_id
    );
    
    /**
     * @brief Handle POST /policies/templates/:id/instantiate - Create rule from template
     * @param req HTTP request with JSON parameters in body
     * @param template_id Template identifier
     * @return HTTP response with created rule
     */
    http::response<http::string_body> handleInstantiateTemplate(
        const http::request<http::string_body>& req,
        const std::string& template_id
    );
    
    /**
     * @brief Handle POST /policies/templates/:id/preview - Preview rule creation
     * @param req HTTP request with JSON parameters in body
     * @param template_id Template identifier
     * @return HTTP response with preview of rule
     */
    http::response<http::string_body> handlePreviewTemplate(
        const http::request<http::string_body>& req,
        const std::string& template_id
    );
    
private:
    std::shared_ptr<themis::governance::PolicyTemplateManager> template_manager_;
    std::shared_ptr<themis::governance::PolicyManager> policy_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    /// Helper: Check authentication and authorization
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
    /// Helper: Make success response
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    ) const;
    
    /// Helper: Make error response
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;
};

} // namespace server
} // namespace themis
