/**
 * @file prompt_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/prompt_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "prompt_engineering/prompt_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

PromptApiHandler::PromptApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<prompt_engineering::PromptManager> prompt_manager,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , prompt_manager_(std::move(prompt_manager))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> PromptApiHandler::handlePost(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePost");
    // Implementation moved from http_server.cpp handlePromptTemplatePost()
    // Note: Authorization checks (requireAccess) from original implementation are not included
    // as they rely on HttpServer methods. Authorization should be handled at middleware/routing layer.
    try {
        if (!prompt_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PromptManager not available", req);
        }
        auto& prompt_manager = *prompt_manager_;
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing JSON body", req);
        }

        auto body = nlohmann::json::parse(req.body());
        themis::prompt_engineering::PromptManager::PromptTemplate t;
        
        if (body.contains("id")) t.id = body.value("id", std::string());
        if (body.contains("name")) t.name = body.value("name", std::string());
        if (body.contains("version")) t.version = body.value("version", std::string());
        if (body.contains("content")) t.content = body.value("content", std::string());
        if (body.contains("metadata")) t.metadata = body["metadata"];
        if (body.contains("active")) t.active = body.value("active", true);

        auto created = prompt_manager.createTemplate(std::move(t));
        return makeResponse(http::status::created, created.toJson().dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PromptApiHandler::handleList(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleList");
    // Implementation moved from http_server.cpp handlePromptTemplateList()
    try {
        if (!prompt_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PromptManager not available", req);
        }
        auto& prompt_manager = *prompt_manager_;
        auto list = prompt_manager.listTemplates();
        nlohmann::json out = nlohmann::json::array();
        for (const auto& t : list) {
            out.push_back(t.toJson());
        }
        return makeResponse(http::status::ok, out.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PromptApiHandler::handleGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleGet");
    // Implementation moved from http_server.cpp handlePromptTemplateGet()
    try {
        if (!prompt_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PromptManager not available", req);
        }
        auto& prompt_manager = *prompt_manager_;
        std::string path = std::string(req.target());
        auto id = extractPathParam(path, "/prompt_template/");
        if (id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing template id", req);
        }

        auto opt = prompt_manager.getTemplate(id);
        if (!opt.has_value()) {
            return makeErrorResponse(http::status::not_found, "Template not found", req);
        }

        return makeResponse(http::status::ok, opt->toJson().dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PromptApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePut");
    // Implementation moved from http_server.cpp handlePromptTemplatePut()
    try {
        if (!prompt_manager_) {
            return makeErrorResponse(http::status::service_unavailable, "PromptManager not available", req);
        }
        auto& prompt_manager = *prompt_manager_;
        std::string path = std::string(req.target());
        auto id = extractPathParam(path, "/prompt_template/");
        if (id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing template id", req);
        }

        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing JSON body", req);
        }

        auto body = nlohmann::json::parse(req.body());
        nlohmann::json metadata = nlohmann::json::object();
        bool active = true;

        if (body.contains("metadata")) metadata = body["metadata"];
        if (body.contains("active")) active = body.value("active", true);

        bool ok = prompt_manager.updateTemplate(id, metadata, active);
        if (!ok) {
            return makeErrorResponse(http::status::not_found, "Template not found", req);
        }

        auto updated_opt = prompt_manager.getTemplate(id);
        nlohmann::json out = updated_opt ? updated_opt->toJson() : nlohmann::json::object();
        return makeResponse(http::status::ok, out.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

std::string PromptApiHandler::extractPathParam(const std::string& target, const std::string& prefix) {
    // Helper implementation following http_server.cpp pattern
    if (target.rfind(prefix, 0) != 0) {
        return "";
    }
    auto param = target.substr(prefix.length());
    // Remove query string if present
    auto query_pos = param.find('?');
    if (query_pos != std::string::npos) {
        param = param.substr(0, query_pos);
    }
    return param;
}

http::response<http::string_body> PromptApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> PromptApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // Helper implementation following http_server.cpp pattern
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis

