/**
 * @file policy_versioning_api_handler.h
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
#include "governance/policy_manager_versioned.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief Handler for Policy Versioning API
 * 
 * This handler manages policy versioning endpoints:
 * - GET /policies/rules/:id/versions - List all versions of a rule
 * - GET /policies/rules/:id/versions/:version - Get specific version
 * - POST /policies/rules/:id/rollback/:version - Rollback to version
 * - GET /policies/rules/:id/diff/:v1/:v2 - Compare versions
 * - GET /policies/audit - Query audit trail
 * - GET /policies/conflicts - Real-time policy conflict report
 */
class PolicyVersioningApiHandler {
public:
    /**
     * @brief Construct a new Policy Versioning API Handler
     * 
     * @param policy_manager_versioned PolicyManagerWithVersioning instance
     * @param auth Authentication/authorization middleware
     */
    PolicyVersioningApiHandler(
        std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle GET /policies/rules/:id/versions - List all versions
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @return HTTP response with JSON array of versions
     */
    http::response<http::string_body> handleListVersions(
        const http::request<http::string_body>& req,
        const std::string& rule_id
    );
    
    /**
     * @brief Handle GET /policies/rules/:id/versions/:version - Get specific version
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @param version Version number
     * @return HTTP response with JSON version object
     */
    http::response<http::string_body> handleGetVersion(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& version
    );
    
    /**
     * @brief Handle POST /policies/rules/:id/rollback/:version - Rollback to version
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @param target_version Version to rollback to
     * @return HTTP response confirming rollback
     */
    http::response<http::string_body> handleRollback(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& target_version
    );
    
    /**
     * @brief Handle GET /policies/rules/:id/diff/:v1/:v2 - Compare versions
     * @param req HTTP request
     * @param rule_id Rule identifier
     * @param version1 First version
     * @param version2 Second version
     * @return HTTP response with version diff
     */
    http::response<http::string_body> handleCompareVersions(
        const http::request<http::string_body>& req,
        const std::string& rule_id,
        const std::string& version1,
        const std::string& version2
    );
    
    /**
     * @brief Handle GET /policies/audit - Query audit trail
     * @param req HTTP request (may include query parameters: rule_id, user, start_time, end_time)
     * @return HTTP response with JSON array of audit entries
     */
    http::response<http::string_body> handleQueryAudit(
        const http::request<http::string_body>& req
    );

    /**
     * @brief Handle GET /policies/conflicts - Real-time policy conflict report
     *
     * Returns a JSON object containing all currently active policy conflicts
     * across the entire rule set.  Conflicts are detected pairwise for every
     * enabled rule and classify contradictory or overlapping conditions with
     * their severity and concrete resolution suggestions.
     *
     * @param req HTTP request
     * @return HTTP response with JSON array of ConflictInfo objects
     */
    http::response<http::string_body> handleGetConflicts(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned_;
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
    
    /// Helper: Extract query parameter from URL
    std::optional<std::string> getQueryParam(
        const std::string& url,
        const std::string& param
    ) const;
};

} // namespace server
} // namespace themis
