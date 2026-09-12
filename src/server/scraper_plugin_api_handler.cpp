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
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <sstream>

namespace themis::server {

namespace {

std::string hashContent(const std::string& content) {
    return std::to_string(std::hash<std::string>{}(content));
}

} // namespace

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

std::string ScraperPluginApiHandler::toIso8601Now() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

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
        auto body = nlohmann::json::parse(req.body());
        const std::string url = body.value("url", "");
        if (url.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"url is required"})";
            resp.prepare_payload();
            return resp;
        }

        themis::scraper::JsRenderRequest render_req;
        render_req.url = url;
        render_req.timeout_ms = body.value("timeout_ms", 30000);
        render_req.wait_selector = body.value("wait_selector", std::string{});
        if (body.contains("headers") && body["headers"].is_object()) {
            render_req.headers = body["headers"].get<std::map<std::string, std::string>>();
        }
        if (body.contains("extra_args") && body["extra_args"].is_array()) {
            render_req.extra_args = body["extra_args"].get<std::vector<std::string>>();
        }

        const auto result = renderer_->render(render_req);

        const std::string job_id = "scrape-" + std::to_string(next_job_id_.fetch_add(1));
        ScraperJobRecord job;
        job.job_id = job_id;
        job.url = url;
        job.created_at = toIso8601Now();

        if (result.success) {
            const std::string source_name = body.value("source_name", std::string{"api"});
            const std::string gov_source_id = body.value("gov_source_id", std::string{});
            themis::scraper::GapContext gap;
            gap.gap_id = body.value("gap_id", std::string{});
            gap.description = body.value("gap_description", std::string{});
            if (body.contains("gap_keywords") && body["gap_keywords"].is_array()) {
                gap.keywords = body["gap_keywords"].get<std::vector<std::string>>();
            }

            themis::scraper::EvaluationResult eval;
            eval.quality_score = 1.0;
            eval.gap_relevance = 1.0;
            eval.summary = "ingested via scraper API";

            const std::string title = body.value("title", url);
            const auto rel = themis::scraper::ScraperRecordBuilder::buildRelational(
                url,
                title,
                result.html,
                source_name,
                gov_source_id,
                eval,
                gap);
            const auto node = themis::scraper::ScraperRecordBuilder::buildNode(rel);
            const auto edges = themis::scraper::ScraperRecordBuilder::buildEdges(rel, eval);
            const auto vec = themis::scraper::ScraperRecordBuilder::buildVector(rel);

            const auto write_result = writer_->write(rel, node, edges, vec);
            if (!write_result.success) {
                job.status = "failed";
                job.message = write_result.error.empty() ? "metadata write failed" : write_result.error;
            } else {
                job.status = "completed";
                job.message = "ok";
            }

            job.content = result.html;
            job.content_hash = hashContent(result.html);
        } else {
            job.status = "failed";
            job.message = result.error.empty() ? "render failed" : result.error;
            job.content_hash.clear();
        }

        {
            std::lock_guard<std::mutex> lock(jobs_mutex_);
            jobs_[job_id] = job;
        }

        nlohmann::json resp_body{
            {"job_id", job_id},
            {"status", job.status},
            {"url",    url},
            {"message", job.message},
        };

        const auto status = job.status == "completed" ? http::status::accepted : http::status::bad_gateway;
        http::response<http::string_body> resp{status, req.version()};
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
    nlohmann::json arr = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        for (const auto& [id, job] : jobs_) {
            arr.push_back({
                {"job_id", id},
                {"url", job.url},
                {"status", job.status},
                {"created_at", job.created_at},
                {"message", job.message}
            });
        }
    }

    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = arr.dump();
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> ScraperPluginApiHandler::handleJobStatus(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    const auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        http::response<http::string_body> resp{http::status::not_found, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"job not found"})";
        resp.prepare_payload();
        return resp;
    }

    const ScraperJobRecord& job = it->second;
    nlohmann::json resp_body{
        {"job_id", job.job_id},
        {"status", job.status},
        {"created_at", job.created_at},
        {"message", job.message}
    };
    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = resp_body.dump();
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> ScraperPluginApiHandler::handleJobResult(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    const auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        http::response<http::string_body> resp{http::status::not_found, req.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"error":"job not found or result not ready"})";
        resp.prepare_payload();
        return resp;
    }

    const ScraperJobRecord& job = it->second;
    nlohmann::json resp_body{
        {"job_id", job.job_id},
        {"url", job.url},
        {"content", job.content},
        {"content_hash", job.content_hash},
        {"links", job.links},
        {"status", job.status}
    };
    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = resp_body.dump();
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> ScraperPluginApiHandler::handleCancelJob(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    bool deleted = false;
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        deleted = jobs_.erase(job_id) > 0;
    }

    http::response<http::string_body> resp{
        deleted ? http::status::no_content : http::status::not_found,
        req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = deleted ? "" : R"({"error":"job not found"})";
    resp.prepare_payload();
    return resp;
}

}  // namespace themis::server
