/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_template_api_handler.cpp                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:14:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     294                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/policy_template_api_handler.h"
#include "server/auth_scope_mapper.h"
#include "utils/logger.h"
#include "utils/tracing.h"

#include <chrono>

namespace themis {
namespace server {

PolicyTemplateApiHandler::PolicyTemplateApiHandler(
    std::shared_ptr<themis::governance::PolicyTemplateManager> template_manager,
    std::shared_ptr<themis::governance::PolicyManager> policy_manager,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : template_manager_(std::move(template_manager))
    , policy_manager_(std::move(policy_manager))
    , auth_(std::move(auth))
{
    if (!template_manager_) {
        THEMIS_WARN("PolicyTemplateApiHandler created with null PolicyTemplateManager");
    }
    if (!policy_manager_) {
        THEMIS_WARN("PolicyTemplateApiHandler created with null PolicyManager");
    }
}

http::response<http::string_body> PolicyTemplateApiHandler::handleListTemplates(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleListTemplates");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!template_manager_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyTemplateManager not initialized", req);
        }
        
        auto templates = template_manager_->listTemplates();
        
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& tmpl : templates) {
            json_array.push_back(tmpl->toJson());
        }
        
        nlohmann::json response = {
            {"templates", json_array},
            {"count", templates.size()}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error listing policy templates: {}", e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyTemplateApiHandler::handleGetTemplate(
    const http::request<http::string_body>& req,
    const std::string& template_id
) {
    auto span = Tracer::startSpan("handleGetTemplate");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!template_manager_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyTemplateManager not initialized", req);
        }
        
        auto tmpl = template_manager_->getTemplate(template_id);
        if (!tmpl.has_value()) {
            return makeErrorResponse(http::status::not_found, 
                "Template not found: " + template_id, req);
        }
        
        return makeResponse(http::status::ok, (*tmpl)->toJson().dump(2), req);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error getting template {}: {}", template_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyTemplateApiHandler::handleInstantiateTemplate(
    const http::request<http::string_body>& req,
    const std::string& template_id
) {
    auto span = Tracer::startSpan("handleInstantiateTemplate");
    try {
        if (!checkAuth(req, "admin")) {
            return makeErrorResponse(http::status::unauthorized, 
                "Unauthorized - admin role required", req);
        }
        
        if (!template_manager_ || !policy_manager_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "Template or Policy Manager not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Extract parameters and rule ID
        if (!body.contains("rule_id")) {
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: rule_id", req);
        }
        
        if (!body.contains("parameters")) {
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: parameters", req);
        }
        
        std::string rule_id = body["rule_id"].get<std::string>();
        nlohmann::json params = body["parameters"];
        
        // Instantiate template
        auto rule = template_manager_->instantiateTemplate(template_id, params, rule_id);
        
        // Add to policy manager
        policy_manager_->addRule(rule);
        
        nlohmann::json response = {
            {"success", true},
            {"rule", rule.toJson()},
            {"message", "Rule created successfully from template " + template_id}
        };
        
        return makeResponse(http::status::created, response.dump(2), req);
        
    } catch (const std::invalid_argument& e) {
        THEMIS_ERROR("Invalid template parameters: {}", e.what());
        return makeErrorResponse(http::status::bad_request, e.what(), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error instantiating template {}: {}", template_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyTemplateApiHandler::handlePreviewTemplate(
    const http::request<http::string_body>& req,
    const std::string& template_id
) {
    auto span = Tracer::startSpan("handlePreviewTemplate");
    try {
        if (!checkAuth(req, "operator")) {
            return makeErrorResponse(http::status::unauthorized, "Unauthorized", req);
        }
        
        if (!template_manager_) {
            return makeErrorResponse(http::status::service_unavailable, 
                "PolicyTemplateManager not initialized", req);
        }
        
        // Parse request body
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Extract parameters and rule ID
        if (!body.contains("rule_id")) {
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: rule_id", req);
        }
        
        if (!body.contains("parameters")) {
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: parameters", req);
        }
        
        std::string rule_id = body["rule_id"].get<std::string>();
        nlohmann::json params = body["parameters"];
        
        // Preview template (doesn't persist)
        auto rule = template_manager_->previewTemplate(template_id, params, rule_id);
        
        nlohmann::json response = {
            {"preview", true},
            {"rule", rule.toJson()},
            {"message", "Preview of rule from template " + template_id}
        };
        
        return makeResponse(http::status::ok, response.dump(2), req);
        
    } catch (const std::invalid_argument& e) {
        THEMIS_ERROR("Invalid template parameters: {}", e.what());
        return makeErrorResponse(http::status::bad_request, e.what(), req);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Error previewing template {}: {}", template_id, e.what());
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

bool PolicyTemplateApiHandler::checkAuth(
    const http::request<http::string_body>& req,
    const std::string& required_role
) const {
    // Backward compatibility: If no auth configured or disabled, allow access but log a warning
    // Production deployments should always enable authentication
    if (!auth_ || !auth_->isEnabled()) {
        THEMIS_WARN("AuthMiddleware not configured or disabled - allowing unauthenticated access to policy template endpoint (dev/test mode only)");
        return true;
    }
    
    // Extract authorization header
    auto auth_it = req.find(http::field::authorization);
    if (auth_it == req.end()) {
        THEMIS_WARN("Missing Authorization header for policy template endpoint");
        return false;
    }
    
    // Extract Bearer token
    std::string auth_header(auth_it->value());
    auto token = AuthMiddleware::extractBearerToken(auth_header);
    
    if (!token) {
        THEMIS_WARN("Invalid Authorization header format for policy template endpoint");
        return false;
    }
    
    // Map role to scope for authorization using shared helper
    std::string required_scope = auth_scope_mapper::mapPolicyRoleToScope(required_role);
    
    // Validate token and check required scope
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        THEMIS_WARN("Authorization failed for policy template endpoint - user: {}, required scope: {}, reason: {}",
            auth_result.user_id.empty() ? "unknown" : auth_result.user_id,
            required_scope,
            auth_result.reason.empty() ? "insufficient_scope" : auth_result.reason);
        return false;
    }
    
    THEMIS_DEBUG("Authorization successful for policy template endpoint - user: {}, scope: {}",
        auth_result.user_id, required_scope);
    return true;
}

http::response<http::string_body> PolicyTemplateApiHandler::makeResponse(
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

http::response<http::string_body> PolicyTemplateApiHandler::makeErrorResponse(
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
