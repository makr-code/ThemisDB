#ifndef THEMIS_PITR_API_HANDLER_H
#define THEMIS_PITR_API_HANDLER_H

#include "storage/pitr_manager.h"
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for Point-in-Time Recovery (PITR) operations
 * 
 * Provides HTTP endpoints for database restore operations:
 * - POST /api/v1/restore/pitr - Execute restore to sequence/tag/timestamp
 * - POST /api/v1/restore/preview - Preview restore operation (dry-run)
 * - GET /api/v1/restore/progress - Get current restore progress
 * 
 * Thread-safety:
 * - NOT thread-safe: Only one restore operation at a time
 * - Concurrent reads during restore are blocked
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
     * @brief Register routes with HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);

private:
    PITRManager& pitr_manager_;

    /**
     * @brief Handle POST /api/v1/restore/pitr
     * 
     * Execute point-in-time restore operation.
     * 
     * Request body:
     * {
     *   "restore_type": "sequence|tag|timestamp",  // Required
     *   "target": "12345|v1.0.0|1705045200000",    // Required: depends on restore_type
     *   "dry_run": false,                           // Optional: preview only (default: false)
     *   "create_backup": true,                      // Optional: auto-backup before restore (default: true)
     *   "abort_on_first_error": true,              // Optional: stop on first error (default: true)
     *   "tables": ["users", "products"],           // Optional: selective restore (default: all tables)
     *   "max_events_to_replay": 0,                 // Optional: limit events (default: 0 = unlimited)
     *   "backup_tag": "before_pitr_restore"        // Optional: backup tag name (default: "before_pitr_restore")
     * }
     * 
     * Response (success):
     * {
     *   "status": "completed|in_progress|dry_run_completed",
     *   "message": "...",
     *   "progress": {
     *     "phase": "...",
     *     "events_processed": 100,
     *     "total_events": 150,
     *     "progress_percent": 66.67,
     *     "elapsed_ms": 1234,
     *     "current_table": "users"
     *   }
     * }
     * 
     * Response (error):
     * {
     *   "error": "...",
     *   "details": "..."
     * }
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/restore/preview
     * 
     * Preview restore operation without applying changes.
     * 
     * Request body (same as handleRestore, but dry_run is forced to true):
     * {
     *   "restore_type": "sequence|tag|timestamp",
     *   "target": "...",
     *   "tables": [...]  // Optional: filter preview
     * }
     * 
     * Response:
     * {
     *   "target_sequence": 12345,
     *   "current_sequence": 12500,
     *   "events_to_replay": 155,
     *   "affected_tables": ["users", "products"],
     *   "affected_keys": ["users:1", "users:2", ...],  // Sample (first 100)
     *   "estimated_duration_sec": 1,
     *   "estimated_size_bytes": 51200
     * }
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/restore/progress
     * 
     * Get current restore operation progress.
     * 
     * Response (restore in progress):
     * {
     *   "in_progress": true,
     *   "progress": {
     *     "phase": "replaying_events",
     *     "events_processed": 100,
     *     "total_events": 150,
     *     "progress_percent": 66.67,
     *     "elapsed_ms": 1234,
     *     "current_table": "users",
     *     "last_error": ""
     *   }
     * }
     * 
     * Response (no restore in progress):
     * {
     *   "in_progress": false,
     *   "message": "No restore operation in progress"
     * }
     */
    void handleProgress(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Parse restore options from JSON request
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& body) const;

    /**
     * @brief Convert RestoreProgress to JSON
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * @brief Convert RestorePreview to JSON
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * @brief Convert Phase enum to string
     */
    std::string phaseToString(PITRManager::RestoreProgress::Phase phase) const;

    /**
     * @brief Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message,
                   const std::string& details = "") const;

    /**
     * @brief Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis

#endif // THEMIS_PITR_API_HANDLER_H
