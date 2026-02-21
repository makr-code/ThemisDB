/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pitr_api_handler.h                                 ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_PITR_API_HANDLER_H
#define THEMIS_PITR_API_HANDLER_H

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

#endif // THEMIS_PITR_API_HANDLER_H
