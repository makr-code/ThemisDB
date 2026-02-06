#include "server/pitr_api_handler.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace themis {
namespace server {

PITRApiHandler::PITRApiHandler(PITRManager& pitr_manager)
    : pitr_manager_(pitr_manager) {
}

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

void PITRApiHandler::handlePreview(const httplib::Request& req, httplib::Response& res) {
    try {
        // Parse request body
        json body = json::parse(req.body);
        
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
                    {"target_sequence", 0},  // Unknown for tag-based preview
                    {"current_sequence", 0},  // Unknown for tag-based preview
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
                    {"target_sequence", 0},  // Unknown for timestamp-based preview
                    {"current_sequence", 0},  // Unknown for timestamp-based preview
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
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis
