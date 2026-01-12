#pragma once

#include <string>
#include <memory>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

// Forward declarations
namespace themis {

class SnapshotManager;

/**
 * @brief REST API Handler for Named Snapshots
 * 
 * Endpoints:
 * - POST   /api/v1/snapshots/tags      - Create a new snapshot tag
 * - GET    /api/v1/snapshots/tags      - List all snapshot tags
 * - GET    /api/v1/snapshots/tags/:name - Get a specific snapshot tag
 * - DELETE /api/v1/snapshots/tags/:name - Delete a snapshot tag
 * - GET    /api/v1/snapshots/stats     - Get snapshot statistics
 */
class SnapshotApiHandler {
public:
    explicit SnapshotApiHandler(SnapshotManager& snapshot_mgr);
    
    ~SnapshotApiHandler() = default;

    // Prevent copying, allow moving
    SnapshotApiHandler(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler& operator=(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler(SnapshotApiHandler&&) noexcept = default;
    SnapshotApiHandler& operator=(SnapshotApiHandler&&) noexcept = default;

    /**
     * @brief Handle POST /api/v1/snapshots/tags
     * Create a new snapshot tag
     * 
     * Request body (JSON):
     * {
     *   "tag_name": "before_migration_2026_q1",
     *   "description": "Snapshot before Q1 2026 migration",
     *   "created_by": "admin"  // optional
     * }
     * 
     * Response (201 Created):
     * {
     *   "tag_name": "before_migration_2026_q1",
     *   "sequence_number": 12345,
     *   "timestamp_ms": 1736629200000,
     *   "description": "Snapshot before Q1 2026 migration",
     *   "created_by": "admin"
     * }
     */
    void handleCreateTag(const boost::beast::http::request<boost::beast::http::string_body>& req, 
                        boost::beast::http::response<boost::beast::http::string_body>& res);

    /**
     * @brief Handle GET /api/v1/snapshots/tags
     * List all snapshot tags
     * 
     * Response (200 OK):
     * {
     *   "tags": [
     *     {
     *       "tag_name": "before_migration_2026_q1",
     *       "sequence_number": 12345,
     *       "timestamp_ms": 1736629200000,
     *       "description": "...",
     *       "created_by": "admin"
     *     },
     *     ...
     *   ],
     *   "total": 5
     * }
     */
    void handleListTags(const boost::beast::http::request<boost::beast::http::string_body>& req, 
                       boost::beast::http::response<boost::beast::http::string_body>& res);

    /**
     * @brief Handle GET /api/v1/snapshots/tags/:name
     * Get a specific snapshot tag
     * 
     * Response (200 OK):
     * {
     *   "tag_name": "before_migration_2026_q1",
     *   "sequence_number": 12345,
     *   "timestamp_ms": 1736629200000,
     *   "description": "...",
     *   "created_by": "admin"
     * }
     * 
     * Response (404 Not Found):
     * {
     *   "error": "Tag not found"
     * }
     */
    void handleGetTag(const boost::beast::http::request<boost::beast::http::string_body>& req, 
                     boost::beast::http::response<boost::beast::http::string_body>& res);

    /**
     * @brief Handle DELETE /api/v1/snapshots/tags/:name
     * Delete a snapshot tag
     * 
     * Response (200 OK):
     * {
     *   "message": "Tag deleted successfully"
     * }
     * 
     * Response (404 Not Found):
     * {
     *   "error": "Tag not found"
     * }
     */
    void handleDeleteTag(const boost::beast::http::request<boost::beast::http::string_body>& req, 
                        boost::beast::http::response<boost::beast::http::string_body>& res);

    /**
     * @brief Handle GET /api/v1/snapshots/stats
     * Get snapshot statistics
     * 
     * Response (200 OK):
     * {
     *   "total_tags": 5,
     *   "oldest_sequence": 100,
     *   "newest_sequence": 12345,
     *   "oldest_timestamp_ms": 1704067200000,
     *   "newest_timestamp_ms": 1736629200000
     * }
     */
    void handleGetStats(const boost::beast::http::request<boost::beast::http::string_body>& req, 
                       boost::beast::http::response<boost::beast::http::string_body>& res);

private:
    SnapshotManager& snapshot_mgr_;

    // Helper methods
    void sendJsonResponse(boost::beast::http::response<boost::beast::http::string_body>& res, 
                         int status, const nlohmann::json& json);
    void sendErrorResponse(boost::beast::http::response<boost::beast::http::string_body>& res, 
                          int status, const std::string& error);
};

} // namespace themis
