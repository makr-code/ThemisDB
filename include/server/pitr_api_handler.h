/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pitr_api_handler.h                                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:28:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "storage/pitr_manager.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * REST API handler for Point-in-Time Recovery (PITR) operations
 * 
 * Provides HTTP endpoints for database restore operations:
 * - POST /api/v1/restore/pitr - Execute restore to sequence/tag/timestamp
 * - POST /api/v1/restore/preview - Preview restore operation (dry-run)
 * - GET /api/v1/restore/progress - Get current restore progress
 * 
 * Thread-safety: NOT thread-safe. Only one restore operation at a time.
 */
class PITRApiHandler {
public:
    /**
     * Construct PITRApiHandler
     */
    explicit PITRApiHandler(PITRManager& pitr_manager);
    
    ~PITRApiHandler() = default;

    // Disable copy, allow move
    PITRApiHandler(const PITRApiHandler&) = delete;
    PITRApiHandler& operator=(const PITRApiHandler&) = delete;
    PITRApiHandler(PITRApiHandler&&) = default;
    PITRApiHandler& operator=(PITRApiHandler&&) = default;

    /**
     * Register routes with HTTP server
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * Handle POST /api/v1/restore/pitr
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle POST /api/v1/restore/preview
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle GET /api/v1/restore/progress
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

private:
    PITRManager& pitr_manager_;

    /**
     * Parse restore options from JSON request
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& options_json) const;

    /**
     * Helper: Convert progress to JSON
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * Helper: Convert preview to JSON
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * Helper: Convert status to JSON
     */
    json statusToJson(const PITRManager::Status& status) const;

    /**
     * Helper: Convert phase enum to string
     */
    std::string phaseToString(PITRManager::RestoreProgress::Phase phase) const;

    /**
     * Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis
