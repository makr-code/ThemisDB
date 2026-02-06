#include "server/policy_validation_api_handler.h"
#include "utils/logger.h"

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
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        
        auto report = validator_->validateRuleset();
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error validating ruleset: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleValidateSingleRule(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        auto rule = themis::governance::PolicyRule::fromJson(body);
        auto issues = validator_->validateSingleRule(rule);
        
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
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        
        auto report = validator_->validateRuleset();
        
        return makeResponse(http::status::ok, report.toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting validation report: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyValidationApiHandler::handleGetMetrics(
    const http::request<http::string_body>& req
) {
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!validator_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyValidator not initialized", req);
        }
        
        auto metrics = validator_->calculateEffectiveness();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& metric : metrics) {
            json_array.push_back(metric.toJson());
        }
        
        nlohmann::json response = {
            {"metrics", json_array},
            {"count", metrics.size()}
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
    if (!auth_) {
        THEMIS_WARN("AuthMiddleware not configured - denying request for role '{}'", required_role);
        return false;
    }
    
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        return false;
    }
    
    const auto auth_value = auth_it->value().to_string();
    if (auth_value.empty()) {
        return false;
    }
    
    // Further validation (e.g., token verification and role checks)
    // should be performed by the configured AuthMiddleware
    // TODO: Replace with actual auth_->authorize(req, required_role) when available
    // For production use, integrate with the actual AuthMiddleware implementation
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
