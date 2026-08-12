/**
 * @file policy_manager_api_handler.h
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
 * @brief Handler for PolicyManager CRUD Operations
 * 
 * This handler manages PolicyManager RBAC rule endpoints:
 * - GET /policies/rules - List all policy rules
 * - GET /policies/rules/:id - Get a specific rule
 * - POST /policies/rules - Create a new rule
 * - PUT /policies/rules/:id - Update an existing rule
 * - DELETE /policies/rules/:id - Delete a rule
 * - POST /policies/evaluate - Evaluate policy for resource/action/roles
 * - GET /policies/stats - Get policy statistics
 * 
 * Features:
 * - Full CRUD for PolicyManager rules
 * - Policy evaluation API
 * - Statistics and diagnostics
 * - Authentication/authorization via AuthMiddleware
 */
class PolicyManagerApiHandler {
public:
    /**
     * @brief Construct a new PolicyManager API Handler
     * 
     * @param policy_manager PolicyManager instance for RBAC rules
     * @param auth Authentication/authorization middleware
     */
    PolicyManagerApiHandler(
        std::shared_ptr<themis::governance::PolicyManager> policy_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle GET /policies/rules - List all rules
     * @param req HTTP request
     * @return HTTP response with JSON array of rules
     */
    http::response<http::string_body> handleListRules(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle GET /policies/rules/:id - Get specific rule
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @return HTTP response with JSON rule object
     */
    http::response<http::string_body> handleGetRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle POST /policies/rules - Create new rule
     * @param req HTTP request with JSON rule in body
     * @return HTTP response with created rule
     */
    http::response<http::string_body> handleCreateRule(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle PUT /policies/rules/:id - Update existing rule
     * @param req HTTP request with JSON rule updates in body
     * @param rule_id Rule identifier
     * @return HTTP response with updated rule
     */
    http::response<http::string_body> handleUpdateRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle DELETE /policies/rules/:id - Delete rule
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @return HTTP response confirming deletion
     */
    http::response<http::string_body> handleDeleteRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle POST /policies/evaluate - Evaluate policy
     * 
     * Request body should contain:
     * {
     *   "resource": "data/users",
     *   "action": "read",
     *   "user_roles": ["operator", "analyst"]
     * }
     * 
     * @param req HTTP request with evaluation parameters
     * @return HTTP response with policy decision
     */
    http::response<http::string_body> handleEvaluatePolicy(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle GET /policies/stats - Get policy statistics
     * @param req HTTP request
     * @return HTTP response with policy stats
     */
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyManager> policy_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    // Helper methods
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );
    
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role = "admin");
};

} // namespace server
} // namespace themis
