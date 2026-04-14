/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            merge_api_handler.h                                ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:28:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     158                                            ║
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

#ifndef THEMIS_ENABLE_HTTP_SERVER
#define THEMIS_ENABLE_HTTP_SERVER 1
#endif

#include "transaction/merge_engine.h"
#include "transaction/snapshot_manager.h"
#include <memory>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_HTTP_SERVER
#include <httplib.h>
#endif

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for Three-Way Merge operations
 * 
 * Provides HTTP endpoints for performing Git-like three-way merges
 * between branches or snapshots in ThemisDB's MVCC system.
 * 
 * Endpoints:
 * - POST /api/v1/merge - Perform three-way merge
 * - POST /api/v1/merge/preview - Preview merge without applying
 * - POST /api/v1/merge/by-tag - Merge using snapshot tags
 * - GET /api/v1/merge/can-fast-forward - Check if fast-forward is possible
 */
class MergeApiHandler {
public:
    /**
     * @brief Construct MergeApiHandler
     * @param merge_engine Reference to MergeEngine instance
     * @param snapshot_manager Reference to SnapshotManager for tag resolution
     */
    explicit MergeApiHandler(
        transaction::MergeEngine& merge_engine,
        transaction::SnapshotManager& snapshot_manager
    );
    
    ~MergeApiHandler() = default;

    // Disable copy, allow move
    MergeApiHandler(const MergeApiHandler&) = delete;
    MergeApiHandler& operator=(const MergeApiHandler&) = delete;
    MergeApiHandler(MergeApiHandler&&) = default;
    MergeApiHandler& operator=(MergeApiHandler&&) = default;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Register routes with HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);

    /**
     * @brief Handle POST /api/v1/merge
     * 
     * Request body:
     * {
     *   "base_sequence": 100,
     *   "source_sequence": 150,
     *   "target_sequence": 200,
     *   "strategy": "ours|theirs|manual|fast_forward",
     *   "fail_on_conflict": false,
     *   "manual_resolutions": [...]
     * }
     */
    void handleMerge(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/merge/preview
     * 
     * Request body:
     * {
     *   "base_sequence": 100,
     *   "source_sequence": 150,
     *   "target_sequence": 200
     * }
     */
    void handleMergePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/merge/by-tag
     * 
     * Request body:
     * {
     *   "base_tag": "v1.0.0",
     *   "source_tag": "feature-branch",
     *   "target_tag": "current",
     *   "strategy": "ours|theirs|manual|fast_forward",
     *   "fail_on_conflict": false,
     *   "manual_resolutions": [...]
     * }
     */
    void handleMergeByTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/merge/can-fast-forward
     * 
     * Query parameters:
     * - base_sequence: Base sequence number
     * - source_sequence: Source sequence number
     * - target_sequence: Target sequence number
     */
    void handleCanFastForward(const httplib::Request& req, httplib::Response& res);
#endif

private:
    transaction::MergeEngine& merge_engine_;
    transaction::SnapshotManager& snapshot_manager_;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Parse merge options from request body
     */
    transaction::MergeEngine::MergeOptions parseMergeOptions(const json& body) const;

    /**
     * @brief Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * @brief Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
#endif
};

} // namespace server
} // namespace themis
