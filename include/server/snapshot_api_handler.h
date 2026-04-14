/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            snapshot_api_handler.h                             ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     122                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "transaction/snapshot_manager.h"
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for Snapshot operations
 * 
 * Provides HTTP endpoints for managing named snapshots/tags:
 * - POST /api/v1/snapshots/tags - Create new tag
 * - GET /api/v1/snapshots/tags - List all tags
 * - GET /api/v1/snapshots/tags/:name - Get specific tag
 * - DELETE /api/v1/snapshots/tags/:name - Delete tag
 * - GET /api/v1/snapshots/stats - Get snapshot statistics
 */
class SnapshotApiHandler {
public:
    /**
     * @brief Construct SnapshotApiHandler
     * @param snapshot_manager Reference to SnapshotManager instance
     */
    explicit SnapshotApiHandler(transaction::SnapshotManager& snapshot_manager);
    
    ~SnapshotApiHandler() = default;

    // Disable copy, allow move
    SnapshotApiHandler(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler& operator=(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler(SnapshotApiHandler&&) = default;
    SnapshotApiHandler& operator=(SnapshotApiHandler&&) = default;

    /**
     * @brief Register routes with HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * @brief Handle POST /api/v1/snapshots/tags
     * 
     * Request body:
     * {
     *   "tag_name": "v1.0.0",
     *   "description": "Release 1.0",
     *   "created_by": "admin"  // optional
     * }
     */
    void handleCreateTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/snapshots/tags
     * 
     * Query parameters:
     * - limit: Maximum number of tags to return (default: 0 = all)
     * - sort_by: Sort field (timestamp, sequence, name) (default: timestamp)
     * - ascending: Sort direction (true/false) (default: false)
     */
    void handleListTags(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/snapshots/tags/:name
     */
    void handleGetTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle DELETE /api/v1/snapshots/tags/:name
     */
    void handleDeleteTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/snapshots/stats
     */
    void handleGetStats(const httplib::Request& req, httplib::Response& res);

private:
    transaction::SnapshotManager& snapshot_manager_;

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
