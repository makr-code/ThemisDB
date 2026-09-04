/**
 * @file policy_versioning_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/policy_versioning_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/input_validator.h"
#include "utils/logger.h"
#include "utils/tracing.h"

#include <chrono>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxPolicyVersioningIdentifierLength = 256;
constexpr size_t kMaxPolicyAuditFieldLength = 1024;

bool isValidIdentifier(std::string_view value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(std::string(value), kMaxPolicyVersioningIdentifierLength) &&
           validator.validatePathSegment(std::string(value));
}

bool isValidAuditField(std::string_view value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxPolicyAuditFieldLength) &&
           validator.validateHeaderValue(std::string(value));
}

} // namespace

PolicyVersioningApiHandler::PolicyVersioningApiHandler(
    std::shared_ptr<themis::governance::PolicyManagerWithVersioning> policy_manager_versioned,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : policy_manager_versioned_(std::move(policy_manager_versioned))
    , auth_(std::move(auth))
{
    if (!policy_manager_versioned_) {
        THEMIS_WARN([[maybe_unused]] "PolicyVersioningApiHandler created with null PolicyManagerWithVersioning");
    }
}

http::response<http::string_body> PolicyVersioningApiHandler::handleListVersions(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    auto span = Tracer::startSpan("handleListVersions");
    try {
        if (!isValidIdentifier(rule_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID", req);
        }

        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        auto& policy_manager_versioned = *policy_manager_versioned_;
        
        auto versions = policy_manager_versioned.getRuleVersions(rule_id);
        
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
        if (!isValidIdentifier(rule_id) || !isValidIdentifier(version)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID or version", req);
        }

        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        auto& policy_manager_versioned = *policy_manager_versioned_;
        
        auto version_data = policy_manager_versioned.getRuleVersion(rule_id, version);
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
    auto span = Tracer::startSpan([[maybe_unused]] "handleRollback");
    try {
        if (!isValidIdentifier(rule_id) || !isValidIdentifier(target_version)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID or target version", req);
        }

        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        auto& policy_manager_versioned = *policy_manager_versioned_;
        
        // Parse request body for user info
        nlohmann::json body;
        if (!req.body().empty()) {
            body = nlohmann::json::parse(req.body());
        }
        
        std::string user = "system";
        if (body.contains("user")) {
            user = body["user"].get<std::string>();
            if (!isValidAuditField(user)) {
                return makeErrorResponse(http::status::bad_request, "Invalid user field", req);
            }
        }
        
        // Perform rollback
        bool success = policy_manager_versioned.rollbackToVersion(
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
        if (!isValidIdentifier(rule_id) || !isValidIdentifier(version1) || !isValidIdentifier(version2)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID or version", req);
        }

        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_versioned_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyManagerWithVersioning not initialized", req);
        }
        auto& policy_manager_versioned = *policy_manager_versioned_;
        
        auto diff = policy_manager_versioned.compareVersions(rule_id, version1, version2);
        
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
        auto& policy_manager_versioned = *policy_manager_versioned_;
        
        // Extract query parameters
        std::string url(req.target());
        auto rule_id = getQueryParam(url, "rule_id");
        auto user = getQueryParam(url, "user");

        if (rule_id.has_value() && !isValidIdentifier(*rule_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule_id query parameter", req);
        }
        if (user.has_value() && !isValidAuditField(*user)) {
            return makeErrorResponse(http::status::bad_request, "Invalid user query parameter", req);
        }
        
        std::optional<std::int64_t> start_time;
        std::optional<std::int64_t> end_time;
        
        auto start_str = getQueryParam(url, "start_time");
        if (start_str.has_value()) {
            if (!isValidAuditField(*start_str)) {
                return makeErrorResponse(http::status::bad_request, "Invalid start_time query parameter", req);
            }
            try {
                start_time = std::stoll(*start_str);
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "policy_versioning_api_handler: unhandled exception caught");
                return makeErrorResponse(http::status::bad_request, "Invalid start_time query parameter", req);
            }
        }
        
        auto end_str = getQueryParam(url, "end_time");
        if (end_str.has_value()) {
            if (!isValidAuditField(*end_str)) {
                return makeErrorResponse(http::status::bad_request, "Invalid end_time query parameter", req);
            }
            try {
                end_time = std::stoll(*end_str);
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "policy_versioning_api_handler: unhandled exception caught");
                return makeErrorResponse(http::status::bad_request, "Invalid end_time query parameter", req);
            }
        }
        
        // Query audit log
        auto entries = policy_manager_versioned.queryAudit(rule_id, user, start_time, end_time);
        
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
        auto& policy_manager_versioned = *policy_manager_versioned_;

        auto conflicts = policy_manager_versioned.getActiveConflicts();

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
    auto& auth = *auth_;
    
    // Extract authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        THEMIS_WARN("Missing Authorization header for policy versioning endpoint");
        return false;
    }
    
    // Extract Bearer token
    std::string auth_str(auth_header.data(), auth_header.size());
    auto token = AuthMiddleware::extractBearerToken(auth_str);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for policy versioning endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth.authorize(*token, required_scope);
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
    std::string pair = {};
    
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

