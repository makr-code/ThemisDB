/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporting_api_handler.h                 ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     103                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
