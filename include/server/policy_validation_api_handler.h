/**
 * @file policy_validation_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "governance/policy_validator.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief Handler for Policy Validation API
 * 
 * This handler manages policy validation endpoints:
 * - POST /policies/validate - Validate current ruleset
 * - POST /policies/validate/rule - Validate single rule
 * - GET /policies/validation/report - Get validation report
 * - GET /policies/metrics - Get effectiveness metrics
 */
class PolicyValidationApiHandler {
public:
    PolicyValidationApiHandler(
        std::shared_ptr<themis::governance::PolicyValidator> validator,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief TBD: Describe handleValidateRuleset.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleValidateRuleset(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief TBD: Describe handleValidateSingleRule.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleValidateSingleRule(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief TBD: Describe handleGetValidationReport.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetValidationReport(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief TBD: Describe handleGetMetrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetMetrics(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyValidator> validator_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    /**
     * @brief TBD: Describe checkAuth.
     * @param[in] req Input parameter.
     * @param[in] required_role Input parameter.
     * @return True on success.
     */
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
    /**
     * @brief TBD: Describe makeResponse.
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
     * @brief TBD: Describe makeErrorResponse.
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
};

} // namespace server
} // namespace themis
