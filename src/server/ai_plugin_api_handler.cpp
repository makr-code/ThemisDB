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
#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis::server {

namespace {

std::string toLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

themis::plugins::PluginType parsePluginType(const nlohmann::json& body) {
    if (body.contains("plugin_type") && body["plugin_type"].is_number_integer()) {
        const int type_value = body["plugin_type"].get<int>();
        if (type_value >= static_cast<int>(themis::plugins::PluginType::COMPUTE_BACKEND)
            && type_value <= static_cast<int>(themis::plugins::PluginType::CUSTOM)) {
            return static_cast<themis::plugins::PluginType>(type_value);
        }
    }

    if (body.contains("plugin_type") && body["plugin_type"].is_string()) {
        const std::string type_name = toLowerAscii(body["plugin_type"].get<std::string>());
        if (type_name == "compute_backend") {
            return themis::plugins::PluginType::COMPUTE_BACKEND;
        }
        if (type_name == "blob_storage") {
            return themis::plugins::PluginType::BLOB_STORAGE;
        }
        if (type_name == "importer") {
            return themis::plugins::PluginType::IMPORTER;
        }
        if (type_name == "exporter") {
            return themis::plugins::PluginType::EXPORTER;
        }
        if (type_name == "hsm_provider") {
            return themis::plugins::PluginType::HSM_PROVIDER;
        }
        if (type_name == "embedding") {
            return themis::plugins::PluginType::EMBEDDING;
        }
        if (type_name == "llm_backend") {
            return themis::plugins::PluginType::LLM_BACKEND;
        }
        if (type_name == "audio_processing") {
            return themis::plugins::PluginType::AUDIO_PROCESSING;
        }
        if (type_name == "image_generation") {
            return themis::plugins::PluginType::IMAGE_GENERATION;
        }
        if (type_name == "agentic_tool") {
            return themis::plugins::PluginType::AGENTIC_TOOL;
        }
        if (type_name == "ingestion_step") {
            return themis::plugins::PluginType::INGESTION_STEP;
        }
        if (type_name == "resource_limit_policy") {
            return themis::plugins::PluginType::RESOURCE_LIMIT_POLICY;
        }
    }

    return themis::plugins::PluginType::CUSTOM;
}

} // namespace

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

std::string AiPluginApiHandler::toIso8601Now() {
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
        const std::string description = body.value("description", plugin_name);

        if (plugin_name.empty()) {
            http::response<http::string_body> resp{http::status::bad_request, req.version()};
            resp.set(http::field::content_type, "application/json");
            resp.body() = R"({"error":"plugin_name is required"})";
            resp.prepare_payload();
            return resp;
        }

        themis::plugins::ai::PluginGenerationPrompt prompt;
        prompt.description = description;
        prompt.type = parsePluginType(body);
        if (body.contains("required_capabilities") && body["required_capabilities"].is_array()) {
            prompt.required_capabilities = body["required_capabilities"].get<std::vector<std::string>>();
        }
        if (body.contains("dependencies") && body["dependencies"].is_array()) {
            prompt.dependencies = body["dependencies"].get<std::vector<std::string>>();
        }

        const std::string job_id = "aip-" + std::to_string(next_job_id_.fetch_add(1));
        PluginJobRecord record;
        record.job_id = job_id;
        record.plugin_name = plugin_name;
        record.created_at = toIso8601Now();

        const auto result = generator_->generatePlugin(prompt);
        if (result.has_value()) {
            record.status = "completed";
            record.generated = result.value();
        } else {
            record.status = "failed";
            record.error_message = result.error().message();
        }

        {
            std::lock_guard<std::mutex> lock(jobs_mutex_);
            jobs_[job_id] = std::move(record);
        }

        nlohmann::json resp_body;
        resp_body["job_id"] = job_id;
        resp_body["status"] = result.has_value() ? "completed" : "failed";
        resp_body["plugin_name"] = plugin_name;
        if (result.has_value()) {
            resp_body["manifest_name"] = result.value().manifest.name;
            resp_body["manifest_version"] = result.value().manifest.version;
            resp_body["passed_security_checks"] = result.value().passed_security_checks;
        } else {
            resp_body["error"] = result.error().message();
        }

        const auto status = result.has_value() ? http::status::accepted : http::status::bad_gateway;
        http::response<http::string_body> resp{status, req.version()};
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
    nlohmann::json resp_body = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        for (const auto& [id, job] : jobs_) {
            nlohmann::json item{
                {"job_id", id},
                {"plugin_name", job.plugin_name},
                {"status", job.status},
                {"created_at", job.created_at}
            };
            if (!job.error_message.empty()) {
                item["error"] = job.error_message;
            }
            resp_body.push_back(std::move(item));
        }
    }

    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = resp_body.dump();
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> AiPluginApiHandler::handleStatus(
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

    const PluginJobRecord& job = it->second;
    nlohmann::json resp_body{
        {"job_id", job.job_id},
        {"status", job.status},
        {"created_at", job.created_at},
        {"plugin_name", job.plugin_name}
    };
    if (!job.error_message.empty()) {
        resp_body["error"] = job.error_message;
    }

    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = resp_body.dump();
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> AiPluginApiHandler::handleDelete(
    const http::request<http::string_body>& req,
    const std::string& job_id)
{
    bool deleted = false;
    {
        std::lock_guard<std::mutex> lock(jobs_mutex_);
        deleted = jobs_.erase(job_id) > 0;
    }

    const auto status = deleted ? http::status::no_content : http::status::not_found;
    http::response<http::string_body> resp{status, req.version()};
    resp.set(http::field::content_type, "application/json");
    resp.body() = deleted ? "" : R"({"error":"job not found"})";
    resp.prepare_payload();
    return resp;
}

}  // namespace themis::server
