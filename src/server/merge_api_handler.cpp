/**
 * @file merge_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/merge_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <stdexcept>
#include "utils/tracing.h"

#ifdef THEMIS_ENABLE_HTTP_SERVER

namespace themis {
namespace server {

using json = nlohmann::json;

MergeApiHandler::MergeApiHandler(
    transaction::MergeEngine& merge_engine,
    transaction::SnapshotManager& snapshot_manager)
    : merge_engine_(merge_engine),
      snapshot_manager_(snapshot_manager) {
}

void MergeApiHandler::registerRoutes([[maybe_unused]] httplib::Server& server) {
    // POST /api/v1/merge - Perform three-way merge
    server.Post("/api/v1/merge", [this](const httplib::Request& req, httplib::Response& res) {
        handleMerge(req, res);
    });

    // POST /api/v1/merge/preview - Preview merge without applying
    server.Post("/api/v1/merge/preview", [this](const httplib::Request& req, httplib::Response& res) {
        handleMergePreview(req, res);
    });

    // POST /api/v1/merge/by-tag - Merge using snapshot tags
    server.Post("/api/v1/merge/by-tag", [this](const httplib::Request& req, httplib::Response& res) {
        handleMergeByTag(req, res);
    });

    // GET /api/v1/merge/can-fast-forward - Check if fast-forward is possible
    server.Get("/api/v1/merge/can-fast-forward", [this](const httplib::Request& req, httplib::Response& res) {
        handleCanFastForward(req, res);
    });

    spdlog::info("Merge API routes registered");
}

void MergeApiHandler::handleMerge(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleMerge");
        // Parse request body
        json request_body = json::parse(req.body);
        
        if (!request_body.contains("base_sequence") ||
            !request_body.contains("source_sequence") ||
            !request_body.contains("target_sequence")) {
            sendError(res, 400, "Missing required fields: base_sequence, source_sequence, target_sequence");
            return;
        }

        uint64_t base_sequence = request_body["base_sequence"];
        uint64_t source_sequence = request_body["source_sequence"];
        uint64_t target_sequence = request_body["target_sequence"];

        auto options = parseMergeOptions(request_body);

        spdlog::info("Performing merge: base={}, source={}, target={}", 
                     base_sequence, source_sequence, target_sequence);

        // Perform merge
        auto result = merge_engine_.merge(base_sequence, source_sequence, target_sequence, options);

        // Return result
        sendJson(res, result.toJson());

    } catch (const json::parse_error& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        spdlog::error("Error handling merge request: {}", e.what());
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void MergeApiHandler::handleMergePreview(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleMergePreview");
        json request_body = json::parse(req.body);
        
        if (!request_body.contains("base_sequence") ||
            !request_body.contains("source_sequence") ||
            !request_body.contains("target_sequence")) {
            sendError(res, 400, "Missing required fields: base_sequence, source_sequence, target_sequence");
            return;
        }

        uint64_t base_sequence = request_body["base_sequence"];
        uint64_t source_sequence = request_body["source_sequence"];
        uint64_t target_sequence = request_body["target_sequence"];

        spdlog::info("Previewing merge: base={}, source={}, target={}", 
                     base_sequence, source_sequence, target_sequence);

        auto result = merge_engine_.previewMerge(base_sequence, source_sequence, target_sequence);

        sendJson(res, result.toJson());

    } catch (const json::parse_error& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        spdlog::error("Error handling merge preview request: {}", e.what());
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void MergeApiHandler::handleMergeByTag(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleMergeByTag");
        json request_body = json::parse(req.body);
        
        if (!request_body.contains("base_tag") ||
            !request_body.contains("source_tag") ||
            !request_body.contains("target_tag")) {
            sendError(res, 400, "Missing required fields: base_tag, source_tag, target_tag");
            return;
        }

        std::string base_tag = request_body["base_tag"];
        std::string source_tag = request_body["source_tag"];
        std::string target_tag = request_body["target_tag"];

        auto options = parseMergeOptions(request_body);

        spdlog::info("Merging by tags: base={}, source={}, target={}", 
                     base_tag, source_tag, target_tag);

        auto result = merge_engine_.mergeByTag(base_tag, source_tag, target_tag, options);

        sendJson(res, result.toJson());

    } catch (const json::parse_error& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        spdlog::error("Error handling merge by tag request: {}", e.what());
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void MergeApiHandler::handleCanFastForward(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleCanFastForward");
        if (!req.has_param("base_sequence") ||
            !req.has_param("source_sequence") ||
            !req.has_param("target_sequence")) {
            sendError(res, 400, "Missing required parameters: base_sequence, source_sequence, target_sequence");
            return;
        }

        uint64_t base_sequence = std::stoull(req.get_param_value("base_sequence"));
        uint64_t source_sequence = std::stoull(req.get_param_value("source_sequence"));
        uint64_t target_sequence = std::stoull(req.get_param_value("target_sequence"));

        bool can_ff = merge_engine_.canFastForward(base_sequence, source_sequence, target_sequence);
        
        json response;
        response["can_fast_forward"] = can_ff;
        response["base_sequence"] = base_sequence;
        response["source_sequence"] = source_sequence;
        response["target_sequence"] = target_sequence;
        
        sendJson(res, response);

    } catch (const std::invalid_argument& e) {
        sendError(res, 400, fmt::format("Invalid parameter: {}", e.what()));
    } catch (const std::exception& e) {
        spdlog::error("Error checking fast-forward: {}", e.what());
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

transaction::MergeEngine::MergeOptions MergeApiHandler::parseMergeOptions([[maybe_unused]] const json& body) const {
    transaction::MergeEngine::MergeOptions options;
    
    // Parse strategy
    if (body.contains("strategy")) {
        std::string strategy = body["strategy"];
        if (strategy == "ours") {
            options.strategy = transaction::MergeEngine::MergeStrategy::OURS;
        } else if (strategy == "theirs") {
            options.strategy = transaction::MergeEngine::MergeStrategy::THEIRS;
        } else if (strategy == "fast_forward") {
            options.strategy = transaction::MergeEngine::MergeStrategy::FAST_FORWARD;
        } else if (strategy == "manual") {
            options.strategy = transaction::MergeEngine::MergeStrategy::MANUAL;
        }
    }

    // Parse other options
    if (body.contains("fail_on_conflict")) {
        options.fail_on_conflict = body["fail_on_conflict"];
    }

    // Parse manual resolutions
    if (body.contains("manual_resolutions")) {
        for (const auto& res_json : body["manual_resolutions"]) {
            transaction::MergeEngine::ConflictResolution resolution;
            resolution.key = res_json["key"];
            if (res_json.contains("resolved_value")) {
                resolution.resolved_value = res_json["resolved_value"].get<std::string>();
            }
            options.manual_resolutions.push_back(resolution);
        }
    }

    return options;
}

void MergeApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) const {
    json error_response;
    error_response["success"] = false;
    error_response["error"] = message;
    
    res.status = status_code;
    res.set_content(error_response.dump(), "application/json");
}

void MergeApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(), "application/json");
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP_SERVER
