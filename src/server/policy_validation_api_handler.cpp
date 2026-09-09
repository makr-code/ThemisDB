/**
 * @file policy_validation_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/policy_validation_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

PolicyValidationApiHandler::PolicyValidationApiHandler(
    std::shared_ptr<themis::governance::PolicyValidator> validator,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : validator_(std::move(validator))
    , auth_(std::move(auth))
{
    if (!validator_) {
        THEMIS_WARN("PolicyValidationApiHandler created with null PolicyValidator");
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleValidateRuleset(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleValidateRuleset");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        auto& validator = *validator_;
        auto report = validator.validateRuleset();
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error validating ruleset: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleValidateSingleRule(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleValidateSingleRule");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        auto& validator = *validator_;
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        auto rule = themis::governance::PolicyRule::fromJson(body);
        auto issues = validator.validateSingleRule(rule);
        
        nlohmann::json response = {
            {"rule_id", rule.id},
            {"valid", issues.empty()},
            {"issues", issues}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error validating single rule: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleGetValidationReport(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetValidationReport");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        auto& validator = *validator_;
        auto report = validator.validateRuleset();
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting validation report: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleGetMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGetMetrics");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        auto& validator = *validator_;
        auto metrics = validator.calculateEffectiveness();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& metric : metrics) {
            json_array.push_back(metric.toJson());
        }
        
        nlohmann::json response = {
            {"metrics", json_array},
            {"count",static_cast<int>(metrics.size())}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting metrics: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

bool PolicyValidationApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to policy validation endpoint (dev/test mode only)");
        return true;
    }
    auto& auth = *auth_;
    
    // Extract authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        THEMIS_WARN("Missing Authorization header for policy validation endpoint");
        return false;
    }
    
    // Extract Bearer token
    auto token = AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(),static_cast<int>(auth_header.size()))
    );
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for policy validation endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth.authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for policy validation endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for policy validation endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
}

http::response<http::string_body> PolicyValidationApiHandler::makeResponse(
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

http::response<http::string_body> PolicyValidationApiHandler::makeErrorResponse(
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

} // namespace server
} // namespace themis
