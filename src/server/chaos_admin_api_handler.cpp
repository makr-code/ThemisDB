/**
 * @file chaos_admin_api_handler.cpp
 * @brief Implementation of the Chaos Engineering admin HTTP API handler.
 *
 * Production consumer route for the `chaos` module.
 * Routes under `/admin/chaos/` expose `ChaosScheduler` fault-injection controls.
 *
 * This file is compiled only when `THEMIS_CHAOS_ADMIN` is ON.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY
 * @note Compile gate: `THEMIS_CHAOS_ADMIN`
 * @see include/server/chaos_admin_api_handler.h
 */

#ifdef THEMIS_CHAOS_ADMIN

#include "server/chaos_admin_api_handler.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

namespace themis::server {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

ChaosAdminApiHandler::ChaosAdminApiHandler(
    std::shared_ptr<themis::AuthMiddleware>        auth,
    std::shared_ptr<themis::chaos::ChaosScheduler> sched)
    : auth_(std::move(auth))
    , scheduler_(std::move(sched))
{
}

ChaosAdminApiHandler::~ChaosAdminApiHandler() = default;

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

http::response<http::string_body> ChaosAdminApiHandler::handle(
    const http::request<http::string_body>& req,
    const std::string& target)
{
    std::string path = target;
    const auto qpos  = path.find('?');
    if (qpos != std::string::npos) {
        path = path.substr(0, qpos);
    }

    const auto method = req.method();

    if (method == http::verb::post && path == "/admin/chaos/inject") {
        return handleInject(req);
    }
    if (method == http::verb::post && path == "/admin/chaos/reset") {
        return handleReset(req);
    }
    if (method == http::verb::get && path == "/admin/chaos/status") {
        return handleStatus(req);
    }
    if (method == http::verb::get && path == "/admin/chaos/history") {
        return handleHistory(req);
    }

    http::response<http::string_body> resp{http::status::not_found, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = R"({"error":"Chaos Admin API: route not found"})";
    resp.prepare_payload();
    return resp;
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

http::response<http::string_body> ChaosAdminApiHandler::handleInject(
    const http::request<http::string_body>& req)
{
    try {
        auto body = nlohmann::json::parse(req.body());

        const std::string failure_class = body.value("failure_class", "");
        const std::string target_module = body.value("target_module", "");
        const int         duration_ms   = body.value("duration_ms", 5000);

        if (failure_class.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"failure_class is required"})";
            resp.prepare_payload();
            return resp;
        }

        themis::chaos::ChaosInjectionRequest inject_req;
        inject_req.failure_class = failure_class;
        inject_req.target_module = target_module;
        inject_req.duration_ms   = duration_ms;

        const auto result = scheduler_->inject(inject_req);

        nlohmann::json resp_body{
            {"injection_id", result.injection_id},
            {"status",       result.status},
            {"failure_class", failure_class},
            {"target_module", target_module},
            {"duration_ms",  duration_ms},
        };

        THEMIS_INFO("Chaos injection {} scheduled: class={} target={} duration={}ms",
                    result.injection_id, failure_class, target_module, duration_ms);

        http::response<http::string_body> resp{http::status::accepted, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const nlohmann::json::exception& ex) {
        THEMIS_WARN("ChaosAdminApiHandler::handleInject JSON parse error: {}", ex.what());
        http::response<http::string_body> resp{http::status::bad_request, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"Invalid JSON body"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("ChaosAdminApiHandler::handleInject error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ChaosAdminApiHandler::handleReset(
    const http::request<http::string_body>& req)
{
    try {
        scheduler_->reset();
        THEMIS_INFO("Chaos framework reset by admin API");

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"status":"reset","active_injections":0})";
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ChaosAdminApiHandler::handleReset error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ChaosAdminApiHandler::handleStatus(
    const http::request<http::string_body>& req)
{
    try {
        const auto status = scheduler_->getStatus();

        nlohmann::json active = nlohmann::json::array();
        for (const auto& inj : status.active_injections) {
            active.push_back({
                {"injection_id",  inj.injection_id},
                {"failure_class", inj.failure_class},
                {"target_module", inj.target_module},
                {"started_at",    inj.started_at},
                {"expires_at",    inj.expires_at},
            });
        }

        nlohmann::json resp_body{
            {"active_injections", active},
            {"total_injected",    status.total_injected},
            {"total_reset",       status.total_reset},
        };

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ChaosAdminApiHandler::handleStatus error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ChaosAdminApiHandler::handleHistory(
    const http::request<http::string_body>& req)
{
    try {
        const auto events = scheduler_->getHistory();

        nlohmann::json history = nlohmann::json::array();
        for (const auto& ev : events) {
            history.push_back({
                {"injection_id",  ev.injection_id},
                {"failure_class", ev.failure_class},
                {"target_module", ev.target_module},
                {"started_at",    ev.started_at},
                {"ended_at",      ev.ended_at},
                {"outcome",       ev.outcome},
            });
        }

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = history.dump();
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("ChaosAdminApiHandler::handleHistory error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

}  // namespace themis::server

#endif  // THEMIS_CHAOS_ADMIN
