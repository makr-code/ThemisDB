/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_versioning_api_handler.cpp                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:14:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     392                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/policy_versioning_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"
#include "utils/tracing.h"

#include <chrono>
#include <sstream>

namespace themis {
namespace server {

PolicyVersioningApiHandler::PolicyVersioningApiHandler(
    std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : policy_manager_versioned_(std::move(policy_manager_versioned))
    , auth_(std::move(auth))
{
    if (!policy_manager_versioned_) {
        THEMIS_WARN("PolicyVersioningApiHandler created with null PolicyManagerWithVersioning");
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleListVersions(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    auto span = Tracer::startSpan("handleListVersions");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        
        auto versions = policy_manager_versioned_->getRuleVersions(rule_id);
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& version : versions) {
            json_array.push_back(version.toJson());
        }
        
        nlohmann::json response = {
            {"rule_id", rule_id},
            {"versions", json_array},
            {"count", versions.size()}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error listing versions for rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleGetVersion(
    const http::request<http::string_body>& req,
    const std::string& rule_id,
    const std::string& version
) {
    auto span = Tracer::startSpan("handleGetVersion");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        
        auto version_data = policy_manager_versioned_->getRuleVersion(rule_id, version);
        if (!version_data.has_value()) {
            return makeErrorResponse(http::status::not_found, 
                "Version " + version + " of rule " + rule_id + " not found", req);
        }
        
        return makeResponse(http::status::ok, version_data->toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting version {} of rule {}: {}", version, rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleRollback(
    const http::request<http::string_body>& req,
    const std::string& rule_id,
    const std::string& target_version
) {
    auto span = Tracer::startSpan("handleRollback");
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        
        // Parse request body for user info
        nlohmann::json body;
        if (!req.body().empty()) {
            body = nlohmann::json::parse(req.body());
        }
        
        std::string user = "system";
        if (body.contains("user")) {
            user = body["user"].get<std::string>();
        }
        
        // Perform rollback
        bool success = policy_manager_versioned_->rollbackToVersion(
            rule_id, 
            target_version, 
            user
        );
        
        if (!success) {
            return makeErrorResponse(http::status::bad_request, 
                "Rollback failed - version may not exist", req);
        }
        
        nlohmann::json response = {
            {"success", true},
            {"rule_id", rule_id},
            {"target_version", target_version},
            {"message", "Successfully rolled back to version " + target_version}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error rolling back rule {} to version {}: {}", 
            rule_id, target_version, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleCompareVersions(
    const http::request<http::string_body>& req,
    const std::string& rule_id,
    const std::string& version1,
    const std::string& version2
) {
    auto span = Tracer::startSpan("handleCompareVersions");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        
        auto diff = policy_manager_versioned_->compareVersions(rule_id, version1, version2);
        
        return makeResponse(http::status::ok, diff.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error comparing versions {} and {} of rule {}: {}", 
            version1, version2, rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleQueryAudit(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleQueryAudit");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        
        // Extract query parameters
        std::string url(req.target());
        auto rule_id = getQueryParam(url, "rule_id");
        auto user = getQueryParam(url, "user");
        
        std::optional<std::int64_t> start_time;
        std::optional<std::int64_t> end_time;
        
        auto start_str = getQueryParam(url, "start_time");
        if (start_str.has_value()) {
            start_time = std::stoll(*start_str);
        }
        
        auto end_str = getQueryParam(url, "end_time");
        if (end_str.has_value()) {
            end_time = std::stoll(*end_str);
        }
        
        // Query audit log
        auto entries = policy_manager_versioned_->queryAudit(rule_id, user, start_time, end_time);
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& entry : entries) {
            json_array.push_back(entry.toJson());
        }
        
        nlohmann::json response = {
            {"entries", json_array},
            {"count", entries.size()}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error querying audit log: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleGetConflicts(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetConflicts");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }

        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable,
                "PolicyManagerWithVersioning not initialized", req);
        }

        auto conflicts = policy_manager_versioned_->getActiveConflicts();

        nlohmann::json conflicts_arr = nlohmann::json::array();
        bool has_critical = false;
        for (const auto& c : conflicts) {
            conflicts_arr.push_back(c.toJson());
            if (!has_critical && (c.severity == "critical" || c.severity == "high")) {
                has_critical = true;
            }
        }

        nlohmann::json response = {
            {"conflicts", conflicts_arr},
            {"conflict_count", conflicts.size()},
            {"has_critical_conflicts", has_critical}
        };

        return makeResponse(http::status::ok, response.dump(2), req);

    } catch (const std::exception& e) {
        THEMIS_ERROR("Error retrieving active policy conflicts: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

bool PolicyVersioningApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to policy versioning endpoint (dev/test mode only)");
        return true;
    }
    
    // Extract authorization header
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        THEMIS_WARN("Missing Authorization header for policy versioning endpoint");
        return false;
    }
    
    // Extract Bearer token
    std::string auth_header(auth_it->value());
    auto token = AuthMiddleware::extractBearerToken(auth_header);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for policy versioning endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for policy versioning endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for policy versioning endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
}

http::response<http::string_body> PolicyVersioningApiHandler::makeResponse(
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

http::response<http::string_body> PolicyVersioningApiHandler::makeErrorResponse(
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

std::optional<std::string> PolicyVersioningApiHandler::getQueryParam(
    const std::string& url,
    const std::string& param
) const {
    // Find query string
    size_t query_pos = url.find('?');
    if (query_pos == std::string::npos) {
        return std::nullopt;
    }
    
    std::string query_string = url.substr(query_pos + 1);
    
    // Parse query parameters
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
