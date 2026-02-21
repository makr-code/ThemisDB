/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_validation_api_handler.h                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     88                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • b5959447b  2026-02-06  Implement GAP-004 Phase 5: Enterprise Policy Features wit... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
