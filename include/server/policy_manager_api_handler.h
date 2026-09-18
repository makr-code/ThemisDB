/**
 * @file policy_manager_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PolicyManagerApiHandler {
public:
    PolicyManagerApiHandler(
        std::shared_ptr<themis::governance::PolicyManager> policy_manager,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle List Rules.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListRules(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Rule.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleGetRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle Create Rule.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCreateRule(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Update Rule.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleUpdateRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle Delete Rule.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleDeleteRule(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle Evaluate Policy.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEvaluatePolicy(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetStats(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyManager> policy_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    // Helper methods
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    );
    
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role = "admin");
};

} // namespace server
} // namespace themis
