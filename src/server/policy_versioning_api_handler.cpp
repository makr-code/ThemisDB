#include "server/policy_versioning_api_handler.h"
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

bool PolicyVersioningApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    if (!auth_) {
        THEMIS_WARN("AuthMiddleware not configured - denying access");
        return false;
    }
    
    // Extract token from Authorization header
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        return false;
    }
    
    std::string auth_header(auth_it->value());
    if (auth_header.find("Bearer ") != 0) {
        return false;
    }
    
    std::string token = auth_header.substr(7);
    
    // Validate token and check role using auth middleware
    // In a real implementation, this would call auth_->validateToken(token, required_role)
    // For now, we perform basic validation
    if (token.empty()) {
        return false;
    }
    
    // TODO: Replace with actual auth_->validateToken(token, required_role) when available
    // For production use, integrate with the actual AuthMiddleware implementation
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
