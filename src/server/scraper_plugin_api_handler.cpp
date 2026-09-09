/**
 * @file scraper_plugin_api_handler.cpp
 * @brief Implementation of the Scraper Plugin HTTP API handler.
 *
 * Production consumer route for the `scraper` module.
 * Routes under `/scraper/` dispatch crawl jobs to `IScraperJSRenderer`
 * and persist metadata via `IScraperMetadataWriter`.
 *
 * This handler is gated by the `THEMIS_PLUGIN_SCRAPER` CMake flag.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY
 * @note Compile gate: `THEMIS_PLUGIN_SCRAPER`
 * @see include/server/scraper_plugin_api_handler.h
 */

#include "server/scraper_plugin_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <sstream>

namespace themis::server {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

ScraperPluginApiHandler::ScraperPluginApiHandler(
    std::shared_ptr<RocksDBWrapper>                          storage,
    std::shared_ptr<themis::AuthMiddleware>                  auth,
    std::shared_ptr<themis::scraper::IScraperJSRenderer>     renderer,
    std::shared_ptr<themis::scraper::IScraperMetadataWriter> writer)
    : storage_(std::move(storage))
    , auth_(std::move(auth))
    , renderer_(std::move(renderer))
    , writer_(std::move(writer))
{
}

ScraperPluginApiHandler::~ScraperPluginApiHandler() = default;

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

http::response<http::string_body> ScraperPluginApiHandler::handle(
    const http::request<http::string_body>& req,
    const std::string& target)
{
    std::string path = target;
    const auto qpos  = path.find('?');
    if (qpos != std::string::npos) {
        path = path.substr(0, qpos);
    }
    // Strip optional /api prefix.
    if (path.rfind("/api/scraper", 0) == 0) {
        path = path.substr(4);
    }

    const auto method = req.method();

    if (method == http::verb::post && path == "/scraper/crawl") {
        return handleCrawl(req);
    }
    if (method == http::verb::get && path == "/scraper/jobs") {
        return handleListJobs(req);
    }
    // /scraper/jobs/{id}/status
    if (method == http::verb::get
        && path.rfind("/scraper/jobs/", 0) == 0
        && path.ends_with("/status")) {
        const std::string job_id = path.substr(14, path.size() - 14 - 7);
        return handleJobStatus(req, job_id);
    }
    // /scraper/jobs/{id}/result
    if (method == http::verb::get
        && path.rfind("/scraper/jobs/", 0) == 0
        && path.ends_with("/result")) {
        const std::string job_id = path.substr(14, path.size() - 14 - 7);
        return handleJobResult(req, job_id);
    }
    // DELETE /scraper/jobs/{id}
    if (method == http::verb::delete_
        && path.rfind("/scraper/jobs/", 0) == 0
        && path.size() > 14) {
        const std::string job_id = path.substr(14);
        return handleCancelJob(req, job_id);
    }

    http::response<http::string_body> resp{http::status::not_found, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = R"({"error":"Scraper API: route not found"})";
    resp.prepare_payload();
    return resp;
}

// ---------------------------------------------------------------------------
// Route handlers
// ---------------------------------------------------------------------------

http::response<http::string_body> ScraperPluginApiHandler::handleCrawl(
    const http::request<http::string_body>& req)
{
    try {
        auto body       = nlohmann::json::parse(req.body());
        const std::string url = body.value("url", "");
        if (url.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"url is required"})";
            resp.prepare_payload();
            return resp;
        }

        themis::scraper::RenderRequest render_req;
        render_req.url            = url;
        render_req.wait_for_js    = body.value("wait_for_js", true);
        render_req.timeout_ms     = body.value("timeout_ms", 30000);
        render_req.extract_links  = body.value("extract_links", false);

        const auto result = renderer_->render(render_req);

        // Persist metadata via writer.
        themis::scraper::ScraperMetadata meta;
        meta.url          = url;
        meta.job_id       = result.job_id;
        meta.status       = result.status;
        meta.content_hash = result.content_hash;
        writer_->write(meta);

        nlohmann::json resp_body{
            {"job_id", result.job_id},
            {"status", result.status},
            {"url",    url},
        };

        http::response<http::string_body> resp{http::status::accepted, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;

    } catch (const nlohmann::json::exception& ex) {
        THEMIS_WARN("ScraperPluginApiHandler::handleCrawl JSON parse error: {}", ex.what());
        http::response<http::string_body> resp{http::status::bad_request, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"Invalid JSON body"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        THEMIS_ERROR("ScraperPluginApiHandler::handleCrawl error: {}", ex.what());
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ScraperPluginApiHandler::handleListJobs(
    const http::request<http::string_body>& req)
{
    try {
        const auto jobs = writer_->listJobs();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& j : jobs) {
            arr.push_back({
                {"job_id",       j.job_id},
                {"url",          j.url},
                {"status",       j.status},
                {"created_at",   j.created_at},
            });
        }
        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = arr.dump();
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ScraperPluginApiHandler::handleJobStatus(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    try {
        const auto status = writer_->getJobStatus(job_id);
        if (!status) {
            http::response<http::string_body> resp{http::status::not_found, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"job not found"})";
            resp.prepare_payload();
            return resp;
        }
        nlohmann::json resp_body{
            {"job_id", job_id},
            {"status", status->status},
            {"progress_pct", status->progress_pct},
        };
        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ScraperPluginApiHandler::handleJobResult(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    try {
        const auto result = writer_->getJobResult(job_id);
        if (!result) {
            http::response<http::string_body> resp{http::status::not_found, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"job not found or result not ready"})";
            resp.prepare_payload();
            return resp;
        }
        nlohmann::json resp_body{
            {"job_id",       job_id},
            {"url",          result->url},
            {"content",      result->content},
            {"content_hash", result->content_hash},
            {"links",        result->links},
        };
        http::response<http::string_body> resp{http::status::ok, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = resp_body.dump();
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

http::response<http::string_body> ScraperPluginApiHandler::handleCancelJob(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    try {
        const bool cancelled = renderer_->cancelJob(job_id);
        http::response<http::string_body> resp{
            cancelled ? http::status::no_content : http::status::not_found,
            req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = cancelled ? "" : R"({"error":"job not found"})";
        resp.prepare_payload();
        return resp;
    } catch (const std::exception& ex) {
        http::response<http::string_body> resp{http::status::internal_server_error, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = std::string(R"({"error":)") + nlohmann::json(ex.what()).dump() + "}";
        resp.prepare_payload();
        return resp;
    }
}

}  // namespace themis::server
