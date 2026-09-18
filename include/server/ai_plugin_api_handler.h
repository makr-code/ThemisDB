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
#include <mutex>
#include <optional>
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

class AiPluginApiHandler {
public:
    AiPluginApiHandler(
        std::shared_ptr<RocksDBWrapper>                       storage,
        std::shared_ptr<themis::AuthMiddleware>               auth,
        const themis::plugins::ai::AIPluginGenerator::Config& generator_cfg = {});

    ~AiPluginApiHandler();

    // Non-copyable, non-movable.
    AiPluginApiHandler(const AiPluginApiHandler&)            = delete;
    AiPluginApiHandler& operator=(const AiPluginApiHandler&) = delete;

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
    struct PluginJobRecord {
        std::string job_id;
        std::string plugin_name;
        std::string status;
        std::string created_at;
        std::string error_message;
        std::optional<themis::plugins::ai::GeneratedPlugin> generated;
    };

    /**
     * @brief To Iso8601 Now.
     * @return Return value.
     */
    static std::string toIso8601Now();

    /**
     * @brief Handle Generate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGenerate(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle List.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle Status.
     * @param[in] req Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);
    /**
     * @brief Handle Delete.
     * @param[in] req Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    http::response<http::string_body> handleDelete(
        const http::request<http::string_body>& req,
        const std::string&                      job_id);

    std::shared_ptr<RocksDBWrapper>                   storage_;
    std::shared_ptr<themis::AuthMiddleware>            auth_;
    std::unique_ptr<themis::plugins::ai::AIPluginGenerator> generator_;
    std::mutex jobs_mutex_;
    std::unordered_map<std::string, PluginJobRecord> jobs_;
    std::atomic<std::uint64_t> next_job_id_{1};
};

}  // namespace server
}  // namespace themis
