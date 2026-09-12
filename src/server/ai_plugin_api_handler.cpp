/**
 * @file ai_plugin_api_handler.cpp
 * @brief Implementation of the AI Plugin Generator HTTP API handler.
 *
 * Production consumer route for the `ai` module.
 * Routes under `/ai/plugins/` are dispatched to `themis::plugins::ai::AIPluginGenerator`.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY
 * @see include/server/ai_plugin_api_handler.h
 */

#include "server/ai_plugin_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <sstream>

namespace themis::server {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AiPluginApiHandler::AiPluginApiHandler(
    std::shared_ptr<RocksDBWrapper>                       storage,
    std::shared_ptr<themis::AuthMiddleware>               auth,
    const themis::plugins::ai::AIPluginGenerator::Config& generator_cfg)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
    , generator_(std::make_unique<themis::plugins::ai::AIPluginGenerator>(generator_cfg))
{
}

AiPluginApiHandler::~AiPluginApiHandler() = default;

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

http::response<http::string_body> AiPluginApiHandler::handle(
    const http::request<http::string_body>& req,
    const std::string& target)
{
    std::string path = target;
    const auto qpos  = path.find('?');
    if (qpos != std::string::npos) {
        path = path.substr(0, qpos);
    }
    // Strip optional /api prefix so routing only needs /ai/plugins/... paths.
    if (path.rfind("/api/ai/", 0) == 0) {
        path = path.substr(4);
    }

    const auto method = req.method();

    if (method == http::verb::post && path == "/ai/plugins/generate") {
        return handleGenerate(req);
    }
    if (method == http::verb::get && path == "/ai/plugins/list") {
        return handleList(req);
    }
    if (method == http::verb::get && path.rfind("/ai/plugins/", 0) == 0
        && path.size() > 12 && path.ends_with("/status")) {
        const std::string job_id = path.substr(12, path.size() - 12 - 7);
        return handleStatus(req, job_id);
    }
    if (method == http::verb::delete_ && path.rfind("/ai/plugins/", 0) == 0
        && path.size() > 12) {
        const std::string job_id = path.substr(12);
        return handleDelete(req, job_id);
    }

    http::response<http::string_body> resp{http::status::not_found, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = R"({"error":"AI Plugin API: route not found"})";
    resp.prepare_payload();
    return resp;
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

http::response<http::string_body> AiPluginApiHandler::handleGenerate(
    const http::request<http::string_body>& req)
{
    try {
        auto body = nlohmann::json::parse(req.body());

        const std::string plugin_name = body.value("plugin_name", "");
        const std::string description  = body.value("description", "");
        const std::string target_lang  = body.value("target_language", "cpp");

        if (plugin_name.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"plugin_name is required"})";
            resp.prepare_payload();
            return resp;
        }

        // Delegate generation to AIPluginGenerator.
        themis::plugins::ai::AIPluginGenerator::GenerationRequest gen_req;
        gen_req.plugin_name   = plugin_name;
        gen_req.description   = description;
        gen_req.target_language = target_lang;

        const auto result = generator_->generate(gen_req);

        nlohmann::json resp_body;
        resp_body["job_id"]     = result.job_id;
        resp_body["status"]     = result.status;
        resp_body["plugin_name"] = plugin_name;

        http::response<http::string_body> resp{http::status::accepted, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const nlohmann::json::exception& ex) {
        THEMIS_WARN("AiPluginApiHandler::handleGenerate JSON parse error: {}", ex.what());
        http::response<http::string_body> resp{http::status::bad_request, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"Invalid JSON body"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("AiPluginApiHandler::handleGenerate error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> AiPluginApiHandler::handleList(
    const http::request<http::string_body>& req)
{
    try {
        const auto plugins = generator_->listGeneratedPlugins();

        nlohmann::json resp_body = nlohmann::json::array();
        for (const auto& p : plugins) {
            resp_body.push_back({
                {"job_id",      p.job_id},
                {"plugin_name", p.plugin_name},
                {"status",      p.status},
                {"created_at",  p.created_at},
            });
        }

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("AiPluginApiHandler::handleList error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> AiPluginApiHandler::handleStatus(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    try {
        const auto status = generator_->getJobStatus(job_id);
        if (!status) {
            http::response<http::string_body> resp{http::status::not_found, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"job not found"})";
            resp.prepare_payload();
            return resp;
        }

        nlohmann::json resp_body{
            {"job_id",     job_id},
            {"status",     status->status},
            {"progress",   status->progress_pct},
            {"message",    status->message},
        };

        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("AiPluginApiHandler::handleStatus error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> AiPluginApiHandler::handleDelete(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    try {
        const bool deleted = generator_->cancelOrDeleteJob(job_id);
        const auto status  = deleted ? http::status::no_content : http::status::not_found;

        http::response<http::string_body> resp{status, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = deleted ? "" : R"({"error":"job not found"})";
        resp.prepare_payload();
        return resp;

    } catch (const std::exception& ex) {
        THEMIS_ERROR("AiPluginApiHandler::handleDelete error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

}  // namespace themis::server
