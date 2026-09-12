/**
 * @file ai_plugin_api_handler.h
 * @brief HTTP handler for the AI Plugin Generator API.
 *
 * Exposes `themis::plugins::ai::AIPluginGenerator` as production HTTP endpoints
 * under `/ai/plugins/`. This header is the production consumer route for the
 * `ai` module — previously the module had only test consumers.
 *
 * ### Routes
 *  - `POST   /ai/plugins/generate`   — Generate a new plugin artefact.
 *  - `GET    /ai/plugins/list`        — List available / generated plugins.
 *  - `GET    /ai/plugins/{id}/status` — Poll generation status for a job.
 *  - `DELETE /ai/plugins/{id}`        — Cancel or delete a plugin job.
 *
 * ### Authentication
 * All routes require a valid ****** validated by `AuthMiddleware`.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY (wiring to HttpServer pending)
 * @note Production consumer route for `src/ai/` — see `src/ai/ARCHITECTURE.md`
 */

#pragma once

#include "server/auth_middleware.h"
#include "ai/ai_plugin_generator.h"

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
 * @brief HTTP handler for AI Plugin Generator endpoints.
 *
 * Wraps `themis::plugins::ai::AIPluginGenerator` and exposes it via REST.
 * Follows the same handler contract as `EthicsApiHandler`, `ExportersApiHandler`, etc.
 *
 * ### Thread safety
 * All public methods are thread-safe. The underlying `AIPluginGenerator`
 * is accessed through its own internal locking.
 */
class AiPluginApiHandler {
public:
    /**
     * @brief Construct the AI Plugin API handler.
     *
     * @param storage       RocksDB storage backend (used for plugin metadata persistence).
     * @param auth          Authentication/authorisation middleware.
     * @param generator_cfg Configuration forwarded to the underlying `AIPluginGenerator`.
     */
    AiPluginApiHandler(
        std::shared_ptr<RocksDBWrapper>                       storage,
        std::shared_ptr<themis::AuthMiddleware>               auth,
        const themis::plugins::ai::AIPluginGenerator::Config& generator_cfg = {});

    ~AiPluginApiHandler();

    // Non-copyable, non-movable.
    AiPluginApiHandler(const AiPluginApiHandler&)            = delete;
    AiPluginApiHandler& operator=(const AiPluginApiHandler&) = delete;

    /**
     * @brief Dispatch an incoming request to the appropriate sub-handler.
     *
     * @param req    Parsed HTTP request.
     * @param target URL target path (may differ from `req.target()` after prefix stripping).
     * @return       HTTP response.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string&                      target);

private:
    /// @name Route handlers
    /// @{
    http::response<http::string_body> handleGenerate(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    /// @}

    std::shared_ptr<RocksDBWrapper>                   storage_;
    std::shared_ptr<themis::AuthMiddleware>            auth_;
    std::unique_ptr<themis::plugins::ai::AIPluginGenerator> generator_;
};

}  // namespace server
}  // namespace themis
