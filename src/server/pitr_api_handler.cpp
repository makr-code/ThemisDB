#include "server/pitr_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace themis {
namespace server {

PITRApiHandler::PITRApiHandler(PITRManager& pitr_manager)
    : pitr_manager_(pitr_manager) {
}

void PITRApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/restore/pitr - Execute restore
void PITRApiHandler::handleRestoreToSequence(const httplib::Request& req, httplib::Response& res) {
#ifdef THEMIS_ENABLE_HTTP_SERVER
void PITRApiHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/restore/pitr - Restore operation
    server.Post("/api/v1/restore/pitr", [this](const httplib::Request& req, httplib::Response& res) {
        handleRestore(req, res);
    });

    // POST /api/v1/restore/preview - Preview restore
    server.Post("/api/v1/restore/preview", [this](const httplib::Request& req, httplib::Response& res) {
        handlePreview(req, res);
    });

    // GET /api/v1/restore/progress - Get restore progress
    server.Get("/api/v1/restore/progress", [this](const httplib::Request& req, httplib::Response& res) {
        handleProgress(req, res);
    // GET /api/v1/restore/progress - Get progress
    server.Get("/api/v1/restore/progress", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetProgress(req, res);
    });

    spdlog::info("PITR API routes registered");
}
#endif

void PITRApiHandler::handleRestore(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
        if (!body.contains("restore_type") || !body.contains("target")) {
            sendError(res, 400, "Missing required fields: restore_type and target");
            return;
        }
        
        std::string restore_type = body["restore_type"];
        std::string target = body["target"];
        
        // Parse restore options
        auto options = parseRestoreOptions(body);
        
        // Execute restore based on type
        PITRManager::Status status;
        
        if (restore_type == "sequence") {
            try {
                uint64_t target_sequence = std::stoull(target);
                status = pitr_manager_.restoreToSequence(target_sequence, options);
            } catch (const std::exception& e) {
                sendError(res, 400, "Invalid sequence number", e.what());
                return;
            }
        } else if (restore_type == "tag") {
            status = pitr_manager_.restoreToTag(target, options);
        } else if (restore_type == "timestamp") {
            try {
                int64_t timestamp_ms = std::stoll(target);
                status = pitr_manager_.restoreToTimestamp(timestamp_ms, options);
            } catch (const std::exception& e) {
                sendError(res, 400, "Invalid timestamp", e.what());
                return;
            }
        } else {
            sendError(res, 400, "Invalid restore_type", 
                     "Must be one of: sequence, tag, timestamp");
            return;
        }
        
        // Handle result
        if (!status.ok) {
            sendError(res, 500, "Restore failed", status.message);
        if (!body.contains("target_sequence")) {
            sendError(res, 400, "Missing required field: target_sequence");
            return;
        }
        
        uint64_t target_sequence = body["target_sequence"];
        
        // Parse options
        if (!body.contains("target")) {
            sendError(res, 400, "Missing required field: target");
            return;
        }
        
        json target = body["target"];
        if (!target.contains("type") || !target.contains("value")) {
            sendError(res, 400, "Target must contain 'type' and 'value'");
            return;
        }
        
        std::string type = target["type"];
        
        // Parse options (use defaults if not provided)
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
        
        // Execute restore based on type with explicit type validation
        PITRManager::Status status;
        
        if (type == "sequence") {
            if (!target["value"].is_number_unsigned()) {
                sendError(res, 400, "Value for sequence target must be a positive integer");
                return;
            }
            uint64_t sequence = target["value"];
            status = pitr_manager_.restoreToSequence(sequence, options);
        }
        else if (type == "tag") {
            if (!target["value"].is_string()) {
                sendError(res, 400, "Value for tag target must be a string");
                return;
            }
            std::string tag_name = target["value"];
            status = pitr_manager_.restoreToTag(tag_name, options);
        }
        else if (type == "timestamp") {
            if (!target["value"].is_number_integer()) {
                sendError(res, 400, "Value for timestamp target must be an integer (milliseconds since epoch)");
                return;
            }
            int64_t timestamp_ms = target["value"];
            status = pitr_manager_.restoreToTimestamp(timestamp_ms, options);
        }
        else {
            sendError(res, 400, fmt::format("Invalid target type: '{}'. Must be 'sequence', 'tag', or 'timestamp'", type));
            return;
        }
        
        // Build response
        json response;
        if (options.dry_run) {
            response["status"] = "dry_run_completed";
            response["message"] = "Dry-run completed successfully. No changes were applied.";
        } else if (status.progress.has_value()) {
            auto& progress = status.progress.value();
            if (progress.phase == PITRManager::RestoreProgress::Phase::COMPLETED) {
                response["status"] = "completed";
                response["message"] = fmt::format("Restore completed successfully. Processed {} events in {}ms",
                                                 progress.events_processed, progress.getElapsedMs());
            } else {
                response["status"] = "in_progress";
                response["message"] = "Restore operation in progress";
            }
            response["progress"] = progressToJson(progress);
        } else {
            response["status"] = "completed";
            response["message"] = "Restore completed successfully";
        }
        
        sendJson(res, response);
        
    } catch (const json::exception& e) {
        sendError(res, 400, "Invalid JSON", e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, "Internal error", e.what());
    }
}

        response["ok"] = status.ok;
        response["message"] = status.message;
        
        if (status.progress.has_value()) {
            auto& prog = status.progress.value();
            response["progress"] = {
                {"phase", static_cast<int>(prog.phase)},
                {"events_processed", prog.events_processed},
                {"total_events", prog.total_events},
                {"progress_percent", prog.getProgressPercent()},
                {"current_table", prog.current_table},
                {"elapsed_ms", prog.getElapsedMs()}
            };
            
            if (!prog.last_error.empty()) {
                response["progress"]["last_error"] = prog.last_error;
            }
        }
        
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

void PITRApiHandler::handleRestoreToTimestamp(const httplib::Request& req, httplib::Response& res) {
void PITRApiHandler::handlePreview(const httplib::Request& req, httplib::Response& res) {
    try {
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
                sendError(res, 400, "Invalid sequence number", e.what());
                return;
            }
        } else if (restore_type == "tag") {
            // Get sequence from tag
            target_sequence_opt = pitr_manager_.getSequenceForTag(target);
            if (!target_sequence_opt.has_value()) {
                sendError(res, 404, "Tag not found", "Tag '" + target + "' does not exist");
                return;
            }
        } else if (restore_type == "timestamp") {
            try {
                int64_t timestamp_ms = std::stoll(target);
                // Find sequence for timestamp
                target_sequence_opt = pitr_manager_.getSequenceForTimestamp(timestamp_ms);
                if (!target_sequence_opt.has_value()) {
                    sendError(res, 404, "No events found for timestamp", 
                             "No changefeed events found at or before the specified timestamp");
                    return;
                }
            } catch (const std::exception& e) {
                sendError(res, 400, "Invalid timestamp", e.what());
                return;
            }
        } else {
            sendError(res, 400, "Invalid restore_type", 
                     "Must be one of: sequence, tag, timestamp");
            return;
        }
        
        // Get preview
        auto preview = pitr_manager_.previewRestore(target_sequence_opt.value(), options);
        
        // Build response
        json response = previewToJson(preview);
        sendJson(res, response);
        
    } catch (const json::exception& e) {
        sendError(res, 400, "Invalid JSON", e.what());
    } catch (const std::exception& e) {
        sendError(res, 500, "Internal error", e.what());
    }
}

void PITRApiHandler::handleProgress(const httplib::Request& req, httplib::Response& res) {
    try {
        auto progress_opt = pitr_manager_.getProgress();
        
        json response;
        if (progress_opt.has_value()) {
            response["in_progress"] = true;
            response["progress"] = progressToJson(progress_opt.value());
        } else {
            response["in_progress"] = false;
            response["message"] = "No restore operation in progress";
        }
        
        sendJson(res, response);
        
    } catch (const std::exception& e) {
        sendError(res, 500, "Internal error", e.what());
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
            options.tables.push_back(table.get<std::string>());
        }
    }
    
    if (body.contains("max_events_to_replay")) {
        options.max_events_to_replay = body["max_events_to_replay"].get<uint64_t>();
    }
    
    if (body.contains("backup_tag")) {
        options.backup_tag = body["backup_tag"].get<std::string>();
        if (!body.contains("timestamp_ms")) {
            sendError(res, 400, "Missing required field: timestamp_ms");
            return;
        }
        
        int64_t timestamp_ms = body["timestamp_ms"];
        
        // Parse options
        if (!body.contains("target")) {
            sendError(res, 400, "Missing required field: target");
            return;
        }
        
        json target = body["target"];
        if (!target.contains("type") || !target.contains("value")) {
            sendError(res, 400, "Target must contain 'type' and 'value'");
            return;
        }
        
        std::string type = target["type"];
        
        // Parse options (always override dry_run to true for preview)
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
        
        options.dry_run = true;  // Force dry-run for preview
        
        // For preview, we use the main restore with dry_run=true
        // This allows preview to work with all target types (sequence, tag, timestamp)
        // All target types now return consistent RestorePreview-like structure
        PITRManager::Status status;
        
        if (type == "sequence") {
            if (!target["value"].is_number_unsigned()) {
                sendError(res, 400, "Value for sequence target must be a positive integer");
                return;
            }
            uint64_t sequence = target["value"];
            // Use previewRestore method which is more efficient for sequence-based preview
            auto preview = pitr_manager_.previewRestore(sequence, options);
            
            json response = {
                {"target_sequence", preview.target_sequence},
                {"current_sequence", preview.current_sequence},
                {"events_to_replay", preview.events_to_replay},
                {"affected_tables", preview.affected_tables},
                {"affected_keys", preview.affected_keys},
                {"estimated_duration_sec", preview.estimated_duration_sec},
                {"estimated_size_bytes", preview.estimated_size_bytes}
            };
            
            sendJson(res, response, 200);
        }
        else if (type == "tag") {
            if (!target["value"].is_string()) {
                sendError(res, 400, "Value for tag target must be a string");
                return;
            }
            std::string tag_name = target["value"];
            status = pitr_manager_.restoreToTag(tag_name, options);
            
            if (!status.ok) {
                sendError(res, 400, fmt::format("Preview failed: {}", status.message));
                return;
            }
            
            // Convert status to consistent preview-style response matching sequence format
            if (status.progress.has_value()) {
                auto& prog = status.progress.value();
                json response = {
                    {"target_sequence", 0},  // Not available in dry-run mode
                    {"current_sequence", 0},  // Not available in dry-run mode
                    {"events_to_replay", prog.total_events},
                    {"affected_tables", std::vector<std::string>()},  // Not available in dry-run mode
                    {"affected_keys", std::vector<std::string>()},    // Not available in dry-run mode
                    {"estimated_duration_sec", 0},  // Not calculated in dry-run mode
                    {"estimated_size_bytes", 0},    // Not calculated in dry-run mode
                    {"message", "Preview completed (dry-run mode)"}
                };
                sendJson(res, response, 200);
            } else {
                json response = {
                    {"target_sequence", 0},
                    {"current_sequence", 0},
                    {"events_to_replay", 0},
                    {"affected_tables", std::vector<std::string>()},
                    {"affected_keys", std::vector<std::string>()},
                    {"estimated_duration_sec", 0},
                    {"estimated_size_bytes", 0},
                    {"message", "Preview completed"}
                };
                sendJson(res, response, 200);
            }
        }
        else if (type == "timestamp") {
            if (!target["value"].is_number_integer()) {
                sendError(res, 400, "Value for timestamp target must be an integer (milliseconds since epoch)");
                return;
            }
            int64_t timestamp_ms = target["value"];
            status = pitr_manager_.restoreToTimestamp(timestamp_ms, options);
            
            if (!status.ok) {
                sendError(res, 400, fmt::format("Preview failed: {}", status.message));
                return;
            }
            
            // Convert status to consistent preview-style response matching sequence format
            if (status.progress.has_value()) {
                auto& prog = status.progress.value();
                json response = {
                    {"target_sequence", 0},  // Not available in dry-run mode
                    {"current_sequence", 0},  // Not available in dry-run mode
                    {"events_to_replay", prog.total_events},
                    {"affected_tables", std::vector<std::string>()},  // Not available in dry-run mode
                    {"affected_keys", std::vector<std::string>()},    // Not available in dry-run mode
                    {"estimated_duration_sec", 0},  // Not calculated in dry-run mode
                    {"estimated_size_bytes", 0},    // Not calculated in dry-run mode
                    {"message", "Preview completed (dry-run mode)"}
                };
                sendJson(res, response, 200);
            } else {
                json response = {
                    {"target_sequence", 0},
                    {"current_sequence", 0},
                    {"events_to_replay", 0},
                    {"affected_tables", std::vector<std::string>()},
                    {"affected_keys", std::vector<std::string>()},
                    {"estimated_duration_sec", 0},
                    {"estimated_size_bytes", 0},
                    {"message", "Preview completed"}
                };
                sendJson(res, response, 200);
            }
        }
        else {
            sendError(res, 400, fmt::format("Invalid target type: '{}'. Must be 'sequence', 'tag', or 'timestamp'", type));
            return;
        }
        
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
void PITRApiHandler::handleGetProgress(const httplib::Request& req, httplib::Response& res) {
    try {
        auto progress_opt = pitr_manager_.getProgress();
        
        if (!progress_opt.has_value()) {
            sendJson(res, {{"in_progress", false}}, 200);
            return;
        }
        
        auto& prog = progress_opt.value();
        
        json response = {
            {"in_progress", true},
            {"phase", static_cast<int>(prog.phase)},
            {"events_processed", prog.events_processed},
            {"total_events", prog.total_events},
            {"progress_percent", prog.getProgressPercent()},
            {"current_table", prog.current_table},
            {"elapsed_ms", prog.getElapsedMs()}
        };
        
        if (!prog.last_error.empty()) {
            response["last_error"] = prog.last_error;
        }
        
        sendJson(res, response, 200);
        
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

void PITRApiHandler::sendError(httplib::Response& res, int status_code, 
                               const std::string& message, const std::string& details) const {
    json error_response;
    error_response["error"] = message;
    if (!details.empty()) {
        error_response["details"] = details;
    }
    
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
        default:
            phase_str = "UNKNOWN";
            spdlog::error("Unknown restore progress phase: {}", static_cast<int>(progress.phase));
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
PITRManager::RestoreOptions PITRApiHandler::parseRestoreOptions(const json& options_json) {
    PITRManager::RestoreOptions options;
    
    if (options_json.contains("dry_run")) {
        options.dry_run = options_json["dry_run"];
    }
    
    if (options_json.contains("create_backup")) {
        options.create_backup = options_json["create_backup"];
    }
    
    if (options_json.contains("abort_on_first_error")) {
        options.abort_on_first_error = options_json["abort_on_first_error"];
    }
    
    if (options_json.contains("tables")) {
        options.tables = options_json["tables"].get<std::vector<std::string>>();
    }
    
    if (options_json.contains("max_events_to_replay")) {
        options.max_events_to_replay = options_json["max_events_to_replay"];
    }
    
    if (options_json.contains("backup_tag")) {
        options.backup_tag = options_json["backup_tag"];
    }
    
    return options;
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
    res.set_content(data.dump(), "application/json");
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis
