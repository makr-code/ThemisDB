/**
 * @file scraper_plugin_api_handler.h
 * @brief HTTP handler for the Scraper Plugin API.
 *
 * Exposes the `scraper` module's `IScraperJSRenderer` and
 * `IScraperMetadataWriter` interfaces as production HTTP endpoints under
 * `/scraper/`. This header is the production consumer route for the `scraper`
 * module — previously the module had only test consumers.
 *
 * ### Routes
 *  - `POST  /scraper/crawl`           — Submit a scrape job (URL + options).
 *  - `GET   /scraper/jobs`            — List submitted jobs (paginated).
 *  - `GET   /scraper/jobs/{id}/status`— Poll scrape job status.
 *  - `GET   /scraper/jobs/{id}/result`— Retrieve scrape result document.
 *  - `DELETE /scraper/jobs/{id}`      — Cancel a pending/running scrape job.
 *
 * ### Plugin availability
 * The scraper module is built as an opt-in plugin target via the CMake flag
 * `THEMIS_PLUGIN_SCRAPER`. When the flag is OFF, the scraper plugin is not
 * linked and this handler must not be instantiated. Use
 * `#ifdef THEMIS_PLUGIN_SCRAPER` guards at call sites.
 *
 * ### Authentication
 * All routes require a valid ****** validated by `AuthMiddleware`.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY (wiring to HttpServer and plugin loading pending)
 * @note Compile gate: `THEMIS_PLUGIN_SCRAPER`
 * @note Production consumer route for `src/scraper/` — see `src/scraper/ARCHITECTURE.md`
 */

#pragma once

#include "server/auth_middleware.h"
#include "scraper/scraper_js_renderer.h"
#include "scraper/scraper_metadata_writer.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstdint>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis {

class RocksDBWrapper;

namespace server {

class ScraperPluginApiHandler {
public:
    ScraperPluginApiHandler(
        std::shared_ptr<RocksDBWrapper>                          storage,
        std::shared_ptr<themis::AuthMiddleware>                  auth,
        std::shared_ptr<themis::scraper::IScraperJSRenderer>     renderer,
        std::shared_ptr<themis::scraper::IScraperMetadataWriter> writer);

    ~ScraperPluginApiHandler();

    // Non-copyable, non-movable.
    ScraperPluginApiHandler(const ScraperPluginApiHandler&)            = delete;
    ScraperPluginApiHandler& operator=(const ScraperPluginApiHandler&) = delete;

    /**
     * @brief Handle.
     * @param[in] req Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string&                      target);

private:
    struct ScraperJobRecord {
        std::string job_id;
        std::string url;
        std::string status;
        std::string created_at;
        std::string message;
        std::string content;
        std::string content_hash;
        std::vector<std::string> links;
    };

    /**
     * @brief To Iso8601 Now.
     * @return Return value.
     */
    static std::string toIso8601Now();

    /**
     * @brief Handle Crawl.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCrawl(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle List Jobs.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleListJobs(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle Job Status.
     * @param[in] req Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    http::response<http::string_body> handleJobStatus(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    /**
     * @brief Handle Job Result.
     * @param[in] req Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    http::response<http::string_body> handleJobResult(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    /**
     * @brief Handle Cancel Job.
     * @param[in] req Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    http::response<http::string_body> handleCancelJob(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);

    std::shared_ptr<RocksDBWrapper>                          storage_;
    std::shared_ptr<themis::AuthMiddleware>                  auth_;
    std::shared_ptr<themis::scraper::IScraperJSRenderer>     renderer_;
    std::shared_ptr<themis::scraper::IScraperMetadataWriter> writer_;
    std::mutex jobs_mutex_;
    std::unordered_map<std::string, ScraperJobRecord> jobs_;
    std::atomic<std::uint64_t> next_job_id_{1};
};

}  // namespace server
}  // namespace themis
