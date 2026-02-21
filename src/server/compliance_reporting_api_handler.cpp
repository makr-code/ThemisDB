/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporting_api_handler.cpp               ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     338                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/compliance_reporting_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"

#include <sstream>

namespace themis {
namespace server {

ComplianceReportingApiHandler::ComplianceReportingApiHandler(
    std::shared_ptr<themis::governance::ComplianceReporter> reporter,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : reporter_(std::move(reporter))
    , auth_(std::move(auth))
{
    if (!reporter_) {
        THEMIS_WARN("ComplianceReportingApiHandler created with null ComplianceReporter");
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleCoverageAnalysis(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        
        // Parse resources from request body if provided
        std::vector<std::string> resources;
        if (!req.body().empty()) {
            try {
                nlohmann::json body = nlohmann::json::parse(req.body());
                if (body.contains("resources") && body["resources"].is_array()) {
                    resources = body["resources"].get<std::vector<std::string>>();
                }
            } catch (...) {
                // If parsing fails, analyze with empty resource list
            }
        }
        
        auto analysis = reporter_->analyzeCoverage(resources);
        
        return makeResponse(http::status::ok, analysis.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error analyzing coverage: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleComplianceReport(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        
        // Get framework from query parameter
        std::string url(req.target());
        auto framework = getQueryParam(url, "framework");
        
        auto report = reporter_->generateComplianceReport(
            framework.value_or("")
        );
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error generating compliance report: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleGapAnalysis(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        
        auto gaps = reporter_->detectGaps();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& gap : gaps) {
            json_array.push_back(gap.toJson());
        }
        
        nlohmann::json response = {
            {"gaps", json_array},
            {"count", gaps.size()}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error detecting gaps: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleGenerateReport(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string report_type = "summary";
        if (body.contains("type")) {
            report_type = body["type"].get<std::string>();
        }
        
        themis::governance::ComplianceReport report;
        
        if (report_type == "summary") {
            report = reporter_->generateSummaryReport();
        } else if (report_type == "compliance") {
            std::string framework = body.value("framework", "");
            report = reporter_->generateComplianceReport(framework);
        } else if (report_type == "risk") {
            report = reporter_->generateRiskAssessmentReport();
        } else {
            return makeErrorResponse(http::status::bad_request, 
                "Invalid report type: " + report_type, req);
        }
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error generating report: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleExportReport(
    const http::request<http::string_body>& req,
    const std::string& report_id
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        
        // Get format from query parameter
        std::string url(req.target());
        auto format = getQueryParam(url, "format");
        std::string export_format = format.value_or("json");
        
        // For this simplified implementation, generate a summary report
        // In a full implementation, you would retrieve the cached report by ID
        auto report = reporter_->generateSummaryReport();
        
        std::string exported = reporter_->exportReport(report, export_format);
        
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        
        if (export_format == "csv") {
            res.set(http::field::content_type, "text/csv");
            res.set(http::field::content_disposition, 
                "attachment; filename=\"compliance_report.csv\"");
        } else {
            res.set(http::field::content_type, "application/json");
        }
        
        res.keep_alive(req.keep_alive());
        res.body() = exported;
        res.prepare_payload();
        
        return res;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error exporting report {}: {}", report_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

bool ComplianceReportingApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to compliance reporting endpoint (dev/test mode only)");
        return true;
    }
    
    // Extract authorization header
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        THEMIS_WARN("Missing Authorization header for compliance reporting endpoint");
        return false;
    }
    
    // Extract Bearer token
    const auto auth_value = std::string(auth_it->value());
    auto token = AuthMiddleware::extractBearerToken(auth_value);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for compliance reporting endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    // Compliance reporting uses audit scopes
    std::string required_scope = auth_scope_mapper::mapAuditRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for compliance reporting endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for compliance reporting endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
}

http::response<http::string_body> ComplianceReportingApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) const {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ComplianceReportingApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) const {
    nlohmann::json error = {
        {"error", message},
        {"status", static_cast<int>(status)}
    };
    return makeResponse(status, error.dump(2), req);
}

std::optional<std::string> ComplianceReportingApiHandler::getQueryParam(
    const std::string& url,
    const std::string& param
) const {
    size_t query_pos = url.find('?');
    if (query_pos == std::string::npos) {
        return std::nullopt;
    }
    
    std::string query_string = url.substr(query_pos + 1);
    std::istringstream iss(query_string);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            
            if (key == param) {
                return value;
            }
        }
    }
    
    return std::nullopt;
}

} // namespace server
} // namespace themis
