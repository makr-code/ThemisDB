/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_api_handler.cpp                             ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2a0e15fab  2026-03-11  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/policy_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/ranger_adapter.h"
#include "server/auth_middleware.h"
#include "server/policy_engine.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

PolicyApiHandler::PolicyApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    RangerClient* ranger_client,
    PolicyEngine* policy_engine,
    std::shared_ptr<AuthMiddleware> auth,
    const std::string& service_name
)
    : storage_(std::move(storage))
    , ranger_client_(ranger_client)
    , policy_engine_(policy_engine)
    , auth_(std::move(auth))
    , service_name_(service_name)
{
}

http::response<http::string_body> PolicyApiHandler::handleImportRanger(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleImportRanger");
    // Implementation moved from http_server.cpp handlePoliciesImportRanger()
    if (!ranger_client_) {
        return makeErrorResponse(http::status::service_unavailable, "Ranger client not configured", req);
    }
    try {
        std::string err;
        auto jsonOpt = ranger_client_->fetchPolicies(&err);
        if (!jsonOpt) {
            return makeErrorResponse(http::status::bad_gateway, std::string("Ranger fetch failed: ") + err, req);
        }
        auto internal = RangerClient::convertFromRanger(*jsonOpt);
        if (internal.empty()) {
            return makeErrorResponse(http::status::bad_request, "No policies converted from Ranger response", req);
        }
        if (!policy_engine_) {
            return makeErrorResponse(http::status::service_unavailable, "Policy engine not initialized", req);
        }
        policy_engine_->setPolicies(internal);
        // Persist to local file
        std::string save_err;
        bool saved = policy_engine_->saveToFile("config/policies.json", &save_err);
        nlohmann::json resp = {
            {"imported", internal.size()},
            {"saved", saved}
        };
        if (!saved) resp["save_error"] = save_err;
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyApiHandler::handleExportRanger(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleExportRanger");
    // Implementation moved from http_server.cpp handlePoliciesExportRanger()
    try {
        if (!policy_engine_) {
            return makeErrorResponse(http::status::service_unavailable, "Policy engine not initialized", req);
        }
        auto list = policy_engine_->listPolicies();
        auto out = RangerClient::convertToRanger(list, service_name_);
        return makeResponse(http::status::ok, out.dump(2), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> PolicyApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // Helper implementation following AdminApiHandler pattern
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> PolicyApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // Helper implementation following AdminApiHandler pattern
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
