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
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis {

class RocksDBWrapper;

namespace server {

/**
 * @brief HTTP handler for scraper plugin endpoints.
 *
 * Bridges incoming HTTP requests to the `scraper` module's renderer and
 * metadata-writer interfaces. Scrape jobs are dispatched asynchronously;
 * callers poll `/scraper/jobs/{id}/status` for completion.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 */
class ScraperPluginApiHandler {
public:
    /**
     * @brief Construct the scraper plugin API handler.
     *
     * @param storage  RocksDB storage backend (for persisting job state and results).
     * @param auth     Authentication/authorisation middleware.
     * @param renderer JS renderer implementation (subprocess or in-memory).
     * @param writer   Metadata writer implementation.
     */
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
     * @brief Dispatch a scraper API request.
     *
     * @param req    Parsed HTTP request.
     * @param target URL target path.
     * @return       HTTP response.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string&                      target);

private:
    /// @name Route handlers
    /// @{
    http::response<http::string_body> handleCrawl(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleListJobs(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleJobStatus(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    http::response<http::string_body> handleJobResult(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    http::response<http::string_body> handleCancelJob(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    /// @}

    std::shared_ptr<RocksDBWrapper>                          storage_;
    std::shared_ptr<themis::AuthMiddleware>                  auth_;
    std::shared_ptr<themis::scraper::IScraperJSRenderer>     renderer_;
    std::shared_ptr<themis::scraper::IScraperMetadataWriter> writer_;
};

}  // namespace server
}  // namespace themis
