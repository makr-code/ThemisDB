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

class PolicyValidationApiHandler {
public:
    PolicyValidationApiHandler(
        std::shared_ptr<themis::governance::PolicyValidator> validator,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle Validate Ruleset.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleValidateRuleset(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Validate Single Rule.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleValidateSingleRule(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Validation Report.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGetValidationReport(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Get Metrics.
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
};

} // namespace server
} // namespace themis
