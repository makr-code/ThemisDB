/**
 * @file compliance_reporting_api_handler.h
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
#include "governance/compliance_reporter.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief Handler for Compliance Reporting API
 * 
 * This handler manages compliance reporting endpoints:
 * - GET /policies/reports/coverage - Coverage analysis
 * - GET /policies/reports/compliance - Compliance status
 * - GET /policies/reports/gaps - Gap analysis
 * - POST /policies/reports/generate - Generate custom report
 *     Supported types: "summary", "compliance", "risk", "time_window"
 *     For "time_window": body fields window_start_ms, window_end_ms (Unix ms),
 *     optional framework string, optional entries array of audit-log JSON objects.
 * - GET /policies/reports/:id/export - Export report
 */
class ComplianceReportingApiHandler {
public:
    ComplianceReportingApiHandler(
        std::shared_ptr<themis::governance::ComplianceReporter> reporter,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    http::response<http::string_body> handleCoverageAnalysis(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleComplianceReport(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleGapAnalysis(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleGenerateReport(
        const http::request<http::string_body>& req
    );
    
    http::response<http::string_body> handleExportReport(
        const http::request<http::string_body>& req,
        const std::string& report_id
    );
    
private:
    std::shared_ptr<themis::governance::ComplianceReporter> reporter_;
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
    
    std::optional<std::string> getQueryParam(const std::string& url, const std::string& param) const;
};

} // namespace server
} // namespace themis
