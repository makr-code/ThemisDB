/**
 * @file compliance_reporting_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ComplianceReportingApiHandler {
public:
    ComplianceReportingApiHandler(
        std::shared_ptr<themis::governance::ComplianceReporter> reporter,
        std::shared_ptr<themis::AuthMiddleware> auth
    );
    
    /**
     * @brief Handle Coverage Analysis.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCoverageAnalysis(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Compliance Report.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleComplianceReport(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Gap Analysis.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGapAnalysis(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Generate Report.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGenerateReport(
        const http::request<http::string_body>& req
    );
    
    /**
     * @brief Handle Export Report.
     * @param[in] req Input parameter.
     * @param[in] report_id Identifier of the report.
     * @return Return value.
     */
    http::response<http::string_body> handleExportReport(
        const http::request<http::string_body>& req,
        const std::string& report_id
    );
    
private:
    std::shared_ptr<themis::governance::ComplianceReporter> reporter_;
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
    std::optional<std::string> getQueryParam(const std::string& url, const std::string& param) const;
};

} // namespace server
} // namespace themis
