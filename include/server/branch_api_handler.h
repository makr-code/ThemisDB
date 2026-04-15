/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            branch_api_handler.h                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "transaction/branch_manager.h"
#include <httplib.h>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for branch operations
 * 
 * Provides HTTP endpoints for:
 * - Creating branches
 * - Listing branches
 * - Switching branches
 * - Merging branches (including conflict preview and per-key resolution)
 * - Deleting branches
 * - Getting branch statistics
 * 
 * Conflict resolution endpoints:
 * - POST /api/v1/branches/merge/preview  – dry-run merge with full conflict details
 * - POST /api/v1/branches/merge/resolve  – apply per-key resolutions and complete merge
 */
class BranchApiHandler {
public:
    /**
     * @brief Construct BranchApiHandler
     * @param branch_manager Reference to BranchManager
     */
    explicit BranchApiHandler(transaction::BranchManager& branch_manager);
    
    /**
     * @brief Register all branch routes with the HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);
    
    // Route handlers
    void handleCreateBranch(const httplib::Request& req, httplib::Response& res);
    void handleListBranches(const httplib::Request& req, httplib::Response& res);
    void handleGetBranch(const httplib::Request& req, httplib::Response& res);
    void handleSwitchBranch(const httplib::Request& req, httplib::Response& res);
    void handleMergeBranches(const httplib::Request& req, httplib::Response& res);
    void handleDeleteBranch(const httplib::Request& req, httplib::Response& res);
    void handleGetStats(const httplib::Request& req, httplib::Response& res);
    void handleGetActiveBranch(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/branches/merge/preview
     * 
     * Returns full conflict details (base, source, and target values per key)
     * without applying any changes. Used by conflict resolution UIs.
     * 
     * Request body:
     * {
     *   "source_branch": "feature-x",
     *   "target_branch": "main",
     *   "base_branch": "common-ancestor"   // optional; enables true 3-way merge detection
     * }
     */
    void handlePreviewMergeBranches(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle POST /api/v1/branches/merge/resolve
     * 
     * Applies per-key conflict resolutions and completes the merge.
     * 
     * Request body:
     * {
     *   "source_branch": "feature-x",
     *   "target_branch": "main",
     *   "base_branch": "common-ancestor",  // optional; enables true 3-way merge
     *   "resolutions": [
     *     { "key": "users:1", "resolved_value": "Alice" },
     *     { "key": "users:2" }   // omit resolved_value to delete the key
     *   ]
     * }
     */
    void handleResolveMergeBranches(const httplib::Request& req, httplib::Response& res);

private:
    transaction::BranchManager& branch_manager_;
    
    // Helper methods
    void sendJson(httplib::Response& res, const json& data, int status_code = 200);
    void sendError(httplib::Response& res, int status_code, const std::string& message);
    bool parseJsonBody(const httplib::Request& req, json& out, httplib::Response& res);
};

} // namespace server
} // namespace themis
