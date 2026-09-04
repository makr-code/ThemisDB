/**
 * @file policy_manager_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/policy_manager_api_handler.h"
#include "server/auth_middleware.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/input_validator.h"

#include <chrono>

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxPolicyBodySize = 1'000'000;
constexpr size_t kMaxRuleIdLength = 256;
constexpr size_t kMaxPolicyFieldLength = 4096;

bool isValidPolicyBody(std::string_view body) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(body), kMaxPolicyBodySize);
}

bool isValidRuleId(std::string_view rule_id) {
    themis::utils::InputValidator validator;
    return !rule_id.empty() &&
           validator.validateStringLength(std::string(rule_id), kMaxRuleIdLength) &&
           validator.validatePathSegment(std::string(rule_id));
}

bool isValidPolicyField(std::string_view value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxPolicyFieldLength) &&
           validator.validateHeaderValue(std::string(value));
}

} // namespace

PolicyManagerApiHandler::PolicyManagerApiHandler(
    std::shared_ptr<themis::governance::PolicyManager> policy_manager,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : policy_manager_(std::move(policy_manager))
    , auth_(std::move(auth))
{
    if (!policy_manager_) {
        THEMIS_WARN([[maybe_unused]] "PolicyManagerApiHandler created with null PolicyManager");
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleListRules(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleListRules");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        auto rules = policy_manager_->listRules();
        nlohmann::json json_array = nlohmann::json::array();
        
        for (const auto& rule : rules) {
            json_array.push_back(rule.toJson());
        }
        
        nlohmann::json response = {
            {"rules", json_array},
            {"count",static_cast<int>(rules.size())}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error listing policy rules: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleGetRule(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    auto span = Tracer::startSpan("handleGetRule");
    try {
        if (!isValidRuleId(rule_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID", req);
        }

        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        auto rule = policy_manager_->getRule(rule_id);
        if (!rule.has_value()) {
            return makeErrorResponse(http::status::not_found, "Rule not found: " + rule_id, req);
        }
        
        return makeResponse(http::status::ok, rule->toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting policy rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleCreateRule(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleCreateRule");
    try {
        if (!isValidPolicyBody(req.body())) {
            return makeErrorResponse(http::status::bad_request, "Request body exceeds maximum allowed size", req);
        }

        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized - admin role required", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        auto& policy_manager = *policy_manager_;
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Convert JSON to PolicyRule
        auto rule = themis::governance::PolicyRule::fromJson(body);
        
        // Validate rule ID is provided
        if (rule.id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Rule ID is required", req);
        }
        if (!isValidRuleId(rule.id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID", req);
        }
        
        // Check if rule already exists
        if (policy_manager.getRule(rule.id).has_value()) {
            return makeErrorResponse(http::status::conflict, "Rule already exists: " + rule.id, req);
        }
        
        // Set creation metadata
        rule.created_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        rule.updated_at = rule.created_at;
        
        // Add rule
        policy_manager.addRule(rule);
        
        THEMIS_INFO("Created policy rule: {}", rule.id);
        
        nlohmann::json response = {
            {"rule", rule.toJson()},
            {"message", "Rule created successfully"}
        };
        
        return makeResponse(http::status::created, response.dump(2), req);
        
    } catch (const nlohmann::json::exception& e) {
        THEMIS_ERROR("JSON parse error creating rule: {}", e.what());
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error creating policy rule: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleUpdateRule(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    auto span = Tracer::startSpan("handleUpdateRule");
    try {
        if (!isValidRuleId(rule_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID", req);
        }
        if (!isValidPolicyBody(req.body())) {
            return makeErrorResponse(http::status::bad_request, "Request body exceeds maximum allowed size", req);
        }

        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized - admin role required", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        // Check if rule exists
        auto existing = policy_manager_->getRule(rule_id);
        if (!existing.has_value()) {
            return makeErrorResponse(http::status::not_found, "Rule not found: " + rule_id, req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Convert JSON to PolicyRule
        auto updated_rule = themis::governance::PolicyRule::fromJson(body);
        
        // Ensure ID matches
        updated_rule.id = rule_id;
        
        // Preserve creation time, update modification time
        updated_rule.created_at = existing->created_at;
        updated_rule.updated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        
        // Update rule (remove old, add new)
        policy_manager_->removeRule(rule_id);
        policy_manager_->addRule(updated_rule);
        
        THEMIS_INFO("Updated policy rule: {}", rule_id);
        
        nlohmann::json response = {
            {"rule", updated_rule.toJson()},
            {"message", "Rule updated successfully"}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const nlohmann::json::exception& e) {
        THEMIS_ERROR("JSON parse error updating rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error updating policy rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleDeleteRule(
    const http::request<http::string_body>& req,
    const std::string& rule_id
) {
    auto span = Tracer::startSpan("handleDeleteRule");
    try {
        if (!isValidRuleId(rule_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid rule ID", req);
        }

        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized - admin role required", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        // Check if rule exists
        if (!policy_manager_->getRule(rule_id).has_value()) {
            return makeErrorResponse(http::status::not_found, "Rule not found: " + rule_id, req);
        }
        
        // Remove rule
        policy_manager_->removeRule(rule_id);
        
        THEMIS_INFO("Deleted policy rule: {}", rule_id);
        
        nlohmann::json response = {
            {"message", "Rule deleted successfully"},
            {"rule_id", rule_id}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error deleting policy rule {}: {}", rule_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleEvaluatePolicy(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEvaluatePolicy");
    try {
        if (!isValidPolicyBody(req.body())) {
            return makeErrorResponse(http::status::bad_request, "Request body exceeds maximum allowed size", req);
        }

        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Extract parameters
        if (!body.contains("resource") || !body.contains("action")) {
            return makeErrorResponse(http::status::bad_request, "Missing required fields: resource, action", req);
        }
        
        std::string resource = body["resource"].get<std::string>();
        std::string action = body["action"].get<std::string>();
        std::vector<std::string> user_roles;

        if (!isValidPolicyField(resource) || !isValidPolicyField(action)) {
            return makeErrorResponse(http::status::bad_request, "Invalid resource or action field", req);
        }
        
        if (body.contains("user_roles")) {
            user_roles = body["user_roles"].get<std::vector<std::string>>();
            for (const auto& role : user_roles) {
                if (!isValidPolicyField(role)) {
                    return makeErrorResponse(http::status::bad_request, "Invalid user_roles entry", req);
                }
            }
        }
        
        // Evaluate policy
        auto decision = policy_manager_->evaluatePolicy(resource, action, user_roles);
        
        // Build response
        nlohmann::json response = {
            {"resource", resource},
            {"action", action},
            {"user_roles", user_roles},
            {"decision", {
                {"allowed", decision.allowed},
                {"require_encryption", decision.require_encryption},
                {"require_signature", decision.require_signature},
                {"allow_export", decision.allow_export},
                {"allow_cache", decision.allow_cache},
                {"retention_days", decision.retention_days},
                {"redaction_level", decision.redaction_level},
                {"audit_access", decision.audit_access},
                {"audit_changes", decision.audit_changes},
                {"applied_rules", decision.applied_rules}
            }}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const nlohmann::json::exception& e) {
        THEMIS_ERROR("JSON parse error evaluating policy: {}", e.what());
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error evaluating policy: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::handleGetStats(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetStats");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PolicyManager not initialized", req);
        }
        
        auto stats = policy_manager_->getStats();
        
        nlohmann::json response = {
            {"total_rules", stats.total_rules},
            {"enabled_rules", stats.enabled_rules},
            {"disabled_rules", stats.disabled_rules},
            {"rules_by_classification", stats.rules_by_classification}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting policy stats: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyManagerApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    nlohmann::json error_json = {
        {"error", message},
        {"status", static_cast<int>(status)}
    };
    
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.body() = error_json.dump();
    res.prepare_payload();
    res.keep_alive(req.keep_alive());
    
    return res;
}

http::response<http::string_body> PolicyManagerApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.body() = body;
    res.prepare_payload();
    res.keep_alive(req.keep_alive());
    
    return res;
}

bool PolicyManagerApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) {
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to policy endpoint (dev/test mode only)");
        return true;
    }
    auto& auth = *auth_;
    
    // Extract authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        THEMIS_WARN("Missing Authorization header for policy endpoint");
        return false;
    }
    
    // Extract Bearer token
    auto token = AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(),static_cast<int>(auth_header.size()))
    );
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for policy endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth.authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for policy endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for policy endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
}

} // namespace server
} // namespace themis
