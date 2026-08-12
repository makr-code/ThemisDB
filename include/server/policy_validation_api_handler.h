/**
 * @file policy_validation_api_handler.h
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
    
    http::response<http::string_body> handleValidateRuleset(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleValidateSingleRule(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleGetValidationReport(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleGetMetrics(
        const http::request<http::string_body>& req
    );
    
private:
    std::shared_ptr<themis::governance::PolicyValidator> validator_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    
    bool checkAuth(const http::request<http::string_body>& req, const std::string& required_role) const;
    
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const http::request<http::string_body>& req
    ) const;
    
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req
    ) const;
};

} // namespace server
} // namespace themis
