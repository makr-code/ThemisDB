#include "server/pitr_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace themis {
namespace server {

PITRApiHandler::PITRApiHandler(PITRManager& pitr_manager)
    : pitr_manager_(pitr_manager) {
}

void PITRApiHandler::handleRestoreToSequence(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("target_sequence")) {
            sendError(res, 400, "Missing required field: target_sequence");
            return;
        }
        
        uint64_t target_sequence = body["target_sequence"];
        
        // Parse options
        PITRManager::RestoreOptions options;
        if (body.contains("options")) {
            options = parseRestoreOptions(body["options"]);
        }
        
        // Execute restore
        auto status = pitr_manager_.restoreToSequence(target_sequence, options);
        
        if (!status.ok) {
            sendError(res, 500, status.message);
            return;
        }
        
        // Return status with progress
        sendJson(res, statusToJson(status), 200);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handleRestoreToTag(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("tag_name")) {
            sendError(res, 400, "Missing required field: tag_name");
            return;
        }
        
        std::string tag_name = body["tag_name"];
        
        // Parse options
        PITRManager::RestoreOptions options;
        if (body.contains("options")) {
            options = parseRestoreOptions(body["options"]);
        }
        
        // Execute restore
        auto status = pitr_manager_.restoreToTag(tag_name, options);
        
        if (!status.ok) {
            sendError(res, 500, status.message);
            return;
        }
        
        // Return status with progress
        sendJson(res, statusToJson(status), 200);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handleRestoreToTimestamp(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("timestamp_ms")) {
            sendError(res, 400, "Missing required field: timestamp_ms");
            return;
        }
        
        int64_t timestamp_ms = body["timestamp_ms"];
        
        // Parse options
        PITRManager::RestoreOptions options;
        if (body.contains("options")) {
            options = parseRestoreOptions(body["options"]);
        }
        
        // Execute restore
        auto status = pitr_manager_.restoreToTimestamp(timestamp_ms, options);
        
        if (!status.ok) {
            sendError(res, 500, status.message);
            return;
        }
        
        // Return status with progress
        sendJson(res, statusToJson(status), 200);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handlePreviewRestore(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("target_sequence")) {
            sendError(res, 400, "Missing required field: target_sequence");
            return;
        }
        
        uint64_t target_sequence = body["target_sequence"];
        
        // Parse options
        PITRManager::RestoreOptions options;
        if (body.contains("options")) {
            options = parseRestoreOptions(body["options"]);
        }
        
        // Get preview
        auto preview = pitr_manager_.previewRestore(target_sequence, options);
        
        // Return preview
        sendJson(res, previewToJson(preview), 200);
        
    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

void PITRApiHandler::handleGetProgress(const httplib::Request& req, httplib::Response& res) {
    try {
        auto progress = pitr_manager_.getProgress();
        
        if (!progress.has_value()) {
            sendError(res, 404, "No restore operation in progress");
            return;
        }
        
        // Return progress
        sendJson(res, progressToJson(*progress), 200);
        
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

PITRManager::RestoreOptions PITRApiHandler::parseRestoreOptions(const json& j) const {
    PITRManager::RestoreOptions options;
    
    if (j.contains("dry_run")) {
        options.dry_run = j["dry_run"];
    }
    if (j.contains("create_backup")) {
        options.create_backup = j["create_backup"];
    }
    if (j.contains("abort_on_first_error")) {
        options.abort_on_first_error = j["abort_on_first_error"];
    }
    if (j.contains("tables")) {
        options.tables = j["tables"].get<std::vector<std::string>>();
    }
    if (j.contains("max_events_to_replay")) {
        options.max_events_to_replay = j["max_events_to_replay"];
    }
    if (j.contains("backup_tag")) {
        options.backup_tag = j["backup_tag"];
    }
    
    return options;
}

json PITRApiHandler::progressToJson(const PITRManager::RestoreProgress& progress) const {
    json j;
    
    // Convert phase enum to string
    std::string phase_str;
    switch (progress.phase) {
        case PITRManager::RestoreProgress::Phase::NOT_STARTED:
            phase_str = "NOT_STARTED";
            break;
        case PITRManager::RestoreProgress::Phase::CREATING_BACKUP:
            phase_str = "CREATING_BACKUP";
            break;
        case PITRManager::RestoreProgress::Phase::VALIDATING:
            phase_str = "VALIDATING";
            break;
        case PITRManager::RestoreProgress::Phase::REPLAYING_EVENTS:
            phase_str = "REPLAYING_EVENTS";
            break;
        case PITRManager::RestoreProgress::Phase::COMMITTING:
            phase_str = "COMMITTING";
            break;
        case PITRManager::RestoreProgress::Phase::COMPLETED:
            phase_str = "COMPLETED";
            break;
        case PITRManager::RestoreProgress::Phase::FAILED:
            phase_str = "FAILED";
            break;
        case PITRManager::RestoreProgress::Phase::ROLLED_BACK:
            phase_str = "ROLLED_BACK";
            break;
    }
    
    j["phase"] = phase_str;
    j["events_processed"] = progress.events_processed;
    j["total_events"] = progress.total_events;
    j["current_table"] = progress.current_table;
    j["last_error"] = progress.last_error;
    j["start_time_ms"] = progress.start_time_ms;
    j["end_time_ms"] = progress.end_time_ms;
    j["progress_percent"] = progress.getProgressPercent();
    j["elapsed_ms"] = progress.getElapsedMs();
    
    return j;
}

json PITRApiHandler::previewToJson(const PITRManager::RestorePreview& preview) const {
    json j;
    j["target_sequence"] = preview.target_sequence;
    j["current_sequence"] = preview.current_sequence;
    j["events_to_replay"] = preview.events_to_replay;
    j["affected_tables"] = preview.affected_tables;
    j["affected_keys"] = preview.affected_keys;
    j["estimated_duration_sec"] = preview.estimated_duration_sec;
    j["estimated_size_bytes"] = preview.estimated_size_bytes;
    return j;
}

json PITRApiHandler::statusToJson(const PITRManager::Status& status) const {
    json j;
    j["ok"] = status.ok;
    j["message"] = status.message;
    
    if (status.progress.has_value()) {
        j["progress"] = progressToJson(*status.progress);
    }
    
    return j;
}

void PITRApiHandler::sendError(httplib::Response& res, int status_code, const std::string& message) const {
    json error;
    error["error"] = message;
    error["status"] = status_code;
    
    res.status = status_code;
    res.set_content(error.dump(), "application/json");
}

void PITRApiHandler::sendJson(httplib::Response& res, const json& data, int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(), "application/json");
}

} // namespace server
} // namespace themis
