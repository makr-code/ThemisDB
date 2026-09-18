/**
 * @file policy_versioning_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "governance/policy_manager_versioned.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

class PolicyVersioningApiHandler {
public:
    PolicyVersioningApiHandler(
        std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle List Versions.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    http::response<http::string_body> handleListVersions(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle Get Version.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetVersion(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& version
    );
    
    /**
     * @brief Handle Rollback.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] target_version Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleRollback(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /**
     * @brief Handle Compare Versions.
     * @param[in] req Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @param[in] version1 Input parameter.
     * @param[in] version2 Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCompareVersions(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& version1,
        const std::string& version2
    );
    
    /**
     * @brief Handle Query Audit.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryAudit(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle Get Conflicts.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetConflicts(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    /**
     * @brief Check Auth.
     * @param[in] req Input parameter.
     * @param[in] required_role Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
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
    ) const;
    
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
    ) const;
    
    /**
     * @brief Get Query Param.
     * @param[in] url Input parameter.
     * @param[in] param Input parameter.
     * @return Return value.
     */
    std::optional<std::string> getQueryParam(
        const std::string& url,
        const std::string& param
    ) const;
};

} // namespace server
} // namespace themis
