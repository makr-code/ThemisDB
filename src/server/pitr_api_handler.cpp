/**
 * @file pitr_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/pitr_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include "utils/tracing.h"
#include "utils/input_validator.h"
#include "utils/logger.h"

namespace themis {
namespace server {

PITRApiHandler::PITRApiHandler(PITRManager& pitr_manager)
    : pitr_manager_(pitr_manager) {
}

void PITRApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/restore/pitr - Execute restore
    server.Post("/api/v1/restore/pitr", [this](const httplib::Request& req, httplib::Response& res) {
        handleRestore(req, res);
    });

    // POST /api/v1/restore/preview - Preview restore
    server.Post("/api/v1/restore/preview", [this](const httplib::Request& req, httplib::Response& res) {
        handlePreview(req, res);
    });

    // GET /api/v1/restore/progress - Get progress
    server.Get("/api/v1/restore/progress", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetProgress(req, res);
    });

    spdlog::info("PITR API routes registered");
}

void PITRApiHandler::handleRestore(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleRestore");
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("restore_type") || !body.contains("target")) {
            sendError(res, 400, "Missing required fields: restore_type and target");
            return;
        }
        
        std::string restore_type = body["restore_type"];
        std::string target = body["target"];
        
        // Parse restore options (use defaults if not provided)
        PITRManager::RestoreOptions options;
        if (body.contains("options")) {
            options = parseRestoreOptions(body["options"]);
        }
        
        // Execute restore based on type
        PITRManager::Status status;
        
        if (restore_type == "sequence") {
            try {
                uint64_t target_sequence = std::stoull(target);
                status = pitr_manager_.restoreToSequence(target_sequence, options);
            } catch (const std::exception& e) {
                sendError(res, 400, fmt::format("Invalid sequence number: {}", e.what()));
                return;
            }
        } else if (restore_type == "tag") {
            status = pitr_manager_.restoreToTag(target, options);
        } else if (restore_type == "timestamp") {
            try {
                int64_t timestamp_ms = std::stoll(target);
                status = pitr_manager_.restoreToTimestamp(timestamp_ms, options);
            } catch (const std::exception& e) {
                sendError(res, 400, fmt::format("Invalid timestamp: {}", e.what()));
                return;
            }
        } else {
            sendError(res, 400, "Invalid restore_type. Must be one of: sequence, tag, timestamp");
            return;
        }
        
        // Build response JSON
        json response = json{
            {"ok", status.ok},
            {"message", status.message}
        };
        
        if (status.ok) {
            sendJson(res, response, 200);
        } else {
            sendJson(res, response, 500);
        }
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handlePreview(const httplib::Request& req, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handlePreview");
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("restore_type") || !body.contains("target")) {
            sendError(res, 400, "Missing required fields: restore_type and target");
            return;
        }
        
        std::string restore_type = body["restore_type"];
        std::string target = body["target"];
        
        // Parse restore options (for table filter)
        auto options = parseRestoreOptions(body);
        
        // Get target sequence based on restore type
        std::optional<uint64_t> target_sequence_opt;
        
        if (restore_type == "sequence") {
            try {
                target_sequence_opt = std::stoull(target);
            } catch (const std::exception& e) {
                sendError(res, 400, fmt::format("Invalid sequence number: {}", e.what()));
                return;
            }
        } else if (restore_type == "tag") {
            // Get sequence from tag
            target_sequence_opt = pitr_manager_.getSequenceForTag(target);
            if (!target_sequence_opt.has_value()) {
                sendError(res, 404, fmt::format("Tag '{}' not found", target));
                return;
            }
        } else if (restore_type == "timestamp") {
            try {
                int64_t timestamp_ms = std::stoll(target);
                // Find sequence for timestamp
                target_sequence_opt = pitr_manager_.getSequenceForTimestamp(timestamp_ms);
                if (!target_sequence_opt.has_value()) {
                    sendError(res, 404, "No changefeed events found at or before the specified timestamp");
                    return;
                }
            } catch (const std::exception& e) {
                sendError(res, 400, fmt::format("Invalid timestamp: {}", e.what()));
                return;
            }
        } else {
            sendError(res, 400, "Invalid restore_type: must be sequence, tag, or timestamp");
            return;
        }
        
        // Get preview
        auto preview = pitr_manager_.previewRestore(target_sequence_opt.value(), options);
        
        // Build and send response
        sendJson(res, previewToJson(preview), 200);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handleGetProgress(const httplib::Request& /*req*/, httplib::Response& res) {
    try {
    auto span = Tracer::startSpan("handleGetProgress");
        auto progress_opt = pitr_manager_.getProgress();
        
        if (!progress_opt.has_value()) {
            sendJson(res, {{"in_progress", false}}, 200);
            return;
        }
        
        json response;
        response["in_progress"] = true;
        response["progress"] = progressToJson(progress_opt.value());
        
        sendJson(res, response, 200);
        
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

PITRManager::RestoreOptions PITRApiHandler::parseRestoreOptions(const json& body) const {
    PITRManager::RestoreOptions options;
    
    if (body.contains("dry_run")) {
        options.dry_run = body["dry_run"].get<bool>();
    }
    
    if (body.contains("create_backup")) {
        options.create_backup = body["create_backup"].get<bool>();
    }
    
    if (body.contains("abort_on_first_error")) {
        options.abort_on_first_error = body["abort_on_first_error"].get<bool>();
    }
    
    if (body.contains("tables") && body["tables"].is_array()) {
        for (const auto& table : body["tables"]) {
            std::string table_name = table.get<std::string>();
            
            // QW-46 Guard: Fail-closed collection name validation
            {
                utils::InputValidator validator;
                if (!validator.validateStringLength(table_name, 256) || !validator.validatePathSegment(table_name)) {
                    THEMIS_ERROR("QW-46 Guard: Invalid table name in PITR restore options");
                    continue;  // Skip invalid table names
                }
            }
            
            options.tables.push_back(table_name);
        }
    }
    
    if (body.contains("max_events_to_replay")) {
        options.max_events_to_replay = body["max_events_to_replay"].get<uint64_t>();
    }
    
    if (body.contains("backup_tag")) {
        options.backup_tag = body["backup_tag"].get<std::string>();
    }
    
    return options;
}

json PITRApiHandler::progressToJson(const PITRManager::RestoreProgress& progress) const {
    json result;
    result["phase"] = phaseToString(progress.phase);
    result["events_processed"] = progress.events_processed;
    result["total_events"] = progress.total_events;
    result["progress_percent"] = progress.getProgressPercent();
    result["elapsed_ms"] = progress.getElapsedMs();
    result["current_table"] = progress.current_table;
    result["last_error"] = progress.last_error;
    result["start_time_ms"] = progress.start_time_ms;
    result["end_time_ms"] = progress.end_time_ms;
    return result;
}

json PITRApiHandler::previewToJson(const PITRManager::RestorePreview& preview) const {
    json result;
    result["target_sequence"] = preview.target_sequence;
    result["current_sequence"] = preview.current_sequence;
    result["events_to_replay"] = preview.events_to_replay;
    result["affected_tables"] = preview.affected_tables;
    result["affected_keys"] = preview.affected_keys;
    result["estimated_duration_sec"] = preview.estimated_duration_sec;
    result["estimated_size_bytes"] = preview.estimated_size_bytes;
    return result;
}

json PITRApiHandler::statusToJson(const PITRManager::Status& status) const {
    json result;
    result["ok"] = status.ok;
    result["message"] = status.message;
    
    if (status.progress.has_value()) {
        result["progress"] = progressToJson(status.progress.value());
    }
    
    return result;
}

std::string PITRApiHandler::phaseToString(PITRManager::RestoreProgress::Phase phase) const {
    switch (phase) {
        case PITRManager::RestoreProgress::Phase::NOT_STARTED:
            return "not_started";
        case PITRManager::RestoreProgress::Phase::CREATING_BACKUP:
            return "creating_backup";
        case PITRManager::RestoreProgress::Phase::VALIDATING:
            return "validating";
        case PITRManager::RestoreProgress::Phase::REPLAYING_EVENTS:
            return "replaying_events";
        case PITRManager::RestoreProgress::Phase::COMMITTING:
            return "committing";
        case PITRManager::RestoreProgress::Phase::COMPLETED:
            return "completed";
        case PITRManager::RestoreProgress::Phase::FAILED:
            return "failed";
        case PITRManager::RestoreProgress::Phase::ROLLED_BACK:
            return "rolled_back";
        default:
            return "unknown";
    }
}

void PITRApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) const {
    json error_response = {
        {"error", message},
        {"status", status_code}
    };
    res.status = status_code;
    res.set_content(error_response.dump(2), "application/json");
}

void PITRApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis
