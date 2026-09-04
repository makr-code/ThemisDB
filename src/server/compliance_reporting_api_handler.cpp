/**
 * @file compliance_reporting_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/compliance_reporting_api_handler.h"
#include <stdexcept>
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"

#include <climits>
#include <sstream>
#include "utils/tracing.h"

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
        THEMIS_WARN([[maybe_unused]] "ComplianceReportingApiHandler created with null ComplianceReporter");
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleCoverageAnalysis(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleCoverageAnalysis");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        auto& reporter = *reporter_;
        // Parse resources from request body if provided
        std::vector<std::string> resources = {};

        if (!req.body().empty()) {
            try {
                nlohmann::json body = nlohmann::json::parse(req.body());
                if (body.contains("resources") && body["resources"].is_array()) {
                    resources = body["resources"].get<std::vector<std::string>>();
                }
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "compliance_reporting_api_handler: unhandled exception caught");
                // If parsing fails, analyze with empty resource list
            }
        }
        
        auto analysis = reporter.analyzeCoverage(resources);
        
        return makeResponse(http::status::ok, analysis.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error analyzing coverage: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleComplianceReport(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleComplianceReport");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        auto& reporter = *reporter_;
        // Get framework from query parameter
        std::string url(req.target());
        auto framework = getQueryParam(url, "framework");
        
        auto report = reporter.generateComplianceReport(
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
    auto span = Tracer::startSpan("handleGapAnalysis");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        auto& reporter = *reporter_;
        auto gaps = reporter.detectGaps();
        
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
    auto span = Tracer::startSpan("handleGenerateReport");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        auto& reporter = *reporter_;
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        std::string report_type = "summary";
        if (body.contains("type")) {
            report_type = body["type"].get<std::string>();
        }
        
        if (report_type == "summary") {
            auto report = reporter.generateSummaryReport();
            return makeResponse(http::status::ok, report.toJson().dump(2), req);
        } else if (report_type == "compliance") {
            std::string framework = body.value("framework", "");
            auto report = reporter.generateComplianceReport(framework);
            return makeResponse(http::status::ok, report.toJson().dump(2), req);
        } else if (report_type == "risk") {
            auto report = reporter.generateRiskAssessmentReport();
            return makeResponse(http::status::ok, report.toJson().dump(2), req);
        } else if (report_type == "time_window") {
            // Parse time window bounds (Unix milliseconds).
            // Defaults: start=0 (epoch), end=INT64_MAX (no upper bound = all time).
            int64_t window_start_ms = 0;
            int64_t window_end_ms   = INT64_MAX;
            if (body.contains("window_start_ms") && body["window_start_ms"].is_number_integer())
                window_start_ms = body["window_start_ms"].get<int64_t>();
            else if (body.contains("window_start_ms") && !body["window_start_ms"].is_number_integer())
                return makeErrorResponse(http::status::bad_request,
                    "window_start_ms must be an integer (Unix milliseconds)", req);
            if (body.contains("window_end_ms") && body["window_end_ms"].is_number_integer())
                window_end_ms = body["window_end_ms"].get<int64_t>();
            else if (body.contains("window_end_ms") && !body["window_end_ms"].is_number_integer())
                return makeErrorResponse(http::status::bad_request,
                    "window_end_ms must be an integer (Unix milliseconds)", req);

            if (window_start_ms < 0 || window_end_ms < 0) {
                return makeErrorResponse(http::status::bad_request,
                    "window_start_ms and window_end_ms must be non-negative", req);
            }
            if (window_start_ms > window_end_ms) {
                return makeErrorResponse(http::status::bad_request,
                    "window_start_ms must be <= window_end_ms", req);
            }

            std::string framework = body.value("framework", "");

            // Optional: caller may supply pre-parsed evaluation entries inline.
            std::vector<themis::governance::RuleEvaluationEntry> entries = {};

            if (body.contains("entries") && body["entries"].is_array()) {
                try {
                    for (const auto& item : body["entries"]) {
                        entries.push_back(
                            themis::governance::RuleEvaluationEntry::fromJson(item));
                    }
                } catch (const std::exception& parse_err) {
                    THEMIS_WARN("time_window report: failed to parse entries array: {}", parse_err.what());
                    return makeErrorResponse(http::status::bad_request,
                        std::string("Invalid entry in entries array: ") + parse_err.what(), req);
                }
            }

            auto report = reporter.generateTimeWindowReport(
                entries, window_start_ms, window_end_ms, framework);
            return makeResponse(http::status::ok, report.toJson().dump(2), req);
        } else {
            return makeErrorResponse(http::status::bad_request, 
                "Invalid report type: " + report_type, req);
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error generating report: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ComplianceReportingApiHandler::handleExportReport(
    const http::request<http::string_body>& req,
    const std::string& report_id
) {
    auto span = Tracer::startSpan("handleExportReport");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!reporter_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "ComplianceReporter not initialized", req);
        }
        auto& reporter = *reporter_;
        // Get format from query parameter
        std::string url(req.target());
        auto format = getQueryParam(url, "format");
        std::string export_format = format.value_or("json");
        
        // For this simplified implementation, generate a summary report
        // In a full implementation, you would retrieve the cached report by ID
        auto report = reporter.generateSummaryReport();
        
        std::string exported = reporter.exportReport(report, export_format);
        
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "ThemisDB");
        
        if (export_format == "csv") {
            res.set(http::field::content_type, "text/csv");
            res.set(http::field::content_disposition, 
                "attachment; filename=\"compliance_report.csv\"");
        } else if (export_format == "html") {
            res.set(http::field::content_type, "text/html");
            res.set(http::field::content_disposition,
                "attachment; filename=\"compliance_report.html\"");
        } else if (export_format == "pdf") {
            res.set(http::field::content_type, "application/pdf");
            res.set(http::field::content_disposition,
                "attachment; filename=\"compliance_report.pdf\"");
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
    auto& auth = *auth_;
    
    // Extract authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        THEMIS_WARN("Missing Authorization header for compliance reporting endpoint");
        return false;
    }
    
    // Extract Bearer token
    const auto auth_value = std::string(auth_header.data(), auth_header.size());
    auto token = AuthMiddleware::extractBearerToken(auth_value);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for compliance reporting endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    // Compliance reporting uses audit scopes
    std::string required_scope = auth_scope_mapper::mapAuditRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth.authorize(*token, required_scope);
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

