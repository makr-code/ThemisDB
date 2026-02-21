/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            branch_api_handler.h                               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     86                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_BRANCH_API_HANDLER_H
#define THEMIS_BRANCH_API_HANDLER_H

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
 * - Merging branches
 * - Deleting branches
 * - Getting branch statistics
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

private:
    transaction::BranchManager& branch_manager_;
    
    // Helper methods
    void sendJson(httplib::Response& res, const json& data, int status_code = 200);
    void sendError(httplib::Response& res, int status_code, const std::string& message);
    bool parseJsonBody(const httplib::Request& req, json& out, httplib::Response& res);
};

} // namespace server
} // namespace themis

#endif // THEMIS_BRANCH_API_HANDLER_H
