#ifndef THEMIS_PITR_API_HANDLER_H
#define THEMIS_PITR_API_HANDLER_H

#include "storage/pitr_manager.h"
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>

#include <memory>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_HTTP_SERVER
#include <httplib.h>
#endif

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for Point-in-Time Recovery (PITR) operations
 * 
 * Provides HTTP endpoints for PITR operations:
 * - POST /api/v1/pitr/restore/sequence - Restore to sequence number
 * - POST /api/v1/pitr/restore/tag - Restore to named tag
 * - POST /api/v1/pitr/restore/timestamp - Restore to timestamp
 * - POST /api/v1/pitr/preview - Preview restore operation
 * - GET /api/v1/pitr/progress - Get current restore progress
 * @brief REST API handler for Point-in-Time Recovery operations
 * 
 * Provides HTTP endpoints for PITR functionality:
 * - POST /api/v1/restore/pitr - Restore to sequence/tag/timestamp
 * - POST /api/v1/restore/preview - Preview restore operation
 * - GET /api/v1/restore/progress - Get restore progress
 */
class PITRApiHandler {
public:
    /**
     * @brief Construct PITRApiHandler
     * @param pitr_manager Reference to PITRManager instance
     */
    explicit PITRApiHandler(PITRManager& pitr_manager);
    
    ~PITRApiHandler() = default;

    // Disable copy, allow move
    PITRApiHandler(const PITRApiHandler&) = delete;
    PITRApiHandler& operator=(const PITRApiHandler&) = delete;
    PITRApiHandler(PITRApiHandler&&) = default;
    PITRApiHandler& operator=(PITRApiHandler&&) = default;

    /**
     * @brief Handle POST /api/v1/pitr/restore/sequence
     * 
     * Request body:
     * {
     *   "target_sequence": 12345,
     *   "options": {
     *     "dry_run": false,
     *     "create_backup": true,
     *     "abort_on_first_error": true,
     *     "tables": [],
     *     "max_events_to_replay": 0,
     *     "backup_tag": "before_pitr_restore"
     *   }
     * }
     */
    void handleRestoreToSequence(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/pitr/restore/tag
     * 
     * Request body:
     * {
     *   "tag_name": "v1.0.0",
     *   "options": { ... }
     * }
     */
    void handleRestoreToTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/pitr/restore/timestamp
     * 
     * Request body:
     * {
     *   "timestamp_ms": 1234567890000,
     *   "options": { ... }
     * }
     */
    void handleRestoreToTimestamp(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/pitr/preview
     * 
     * Request body:
     * {
     *   "target_sequence": 12345,
     *   "options": {
     *     "tables": []
     *   }
     * }
     */
    void handlePreviewRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/pitr/progress
     * 
     * Returns current restore progress or 404 if no restore in progress.
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

private:
    PITRManager& pitr_manager_;

    /**
     * @brief Parse RestoreOptions from JSON
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& j) const;

    /**
     * @brief Convert RestoreProgress to JSON
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * @brief Convert RestorePreview to JSON
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * @brief Convert Status to JSON
     */
    json statusToJson(const PITRManager::Status& status) const;
#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Register routes with HTTP server
     * @param server HTTP server instance
     * 
     * Note: This method is for future use when HTTP server migration from Beast to cpp-httplib is complete.
     * Currently, routes are registered via the Route enum in HttpServer::routeRequest.
     */
    void registerRoutes(httplib::Server& server);
#endif

private:
    PITRManager& pitr_manager_;

    /**
     * @brief Handle POST /api/v1/restore/pitr
     * 
     * Request body:
     * {
     *   "target": {
     *     "type": "sequence" | "tag" | "timestamp",
     *     "value": <uint64_t> | <string> | <int64_t>
     *   },
     *   "options": {
     *     "dry_run": false,
     *     "create_backup": true,
     *     "abort_on_first_error": true,
     *     "tables": ["table1", "table2"],  // optional, empty = all
     *     "max_events_to_replay": 0,       // optional, 0 = unlimited
     *     "backup_tag": "before_pitr_restore"  // optional
     *   }
     * }
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/restore/preview
     * 
     * Request body: Same as handleRestore, but always sets dry_run=true
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/restore/progress
     * 
     * Query parameters: None
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Parse restore options from JSON
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& options_json);

    /**
     * @brief Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * @brief Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis

#endif // THEMIS_PITR_API_HANDLER_H
