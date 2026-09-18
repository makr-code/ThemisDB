/**
 * @file branch_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @return Return value.
     */
    explicit BranchApiHandler(transaction::BranchManager& branch_manager);
    
    /**
     * @brief Register all branch routes with the HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * @brief Route handlers
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleCreateBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleListBranches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleListBranches(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleGetBranch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleSwitchBranch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleSwitchBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleMergeBranches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMergeBranches(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleDeleteBranch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleDeleteBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleGetStats.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetStats(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief TBD: Describe handleGetActiveBranch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
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
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
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
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleResolveMergeBranches(const httplib::Request& req, httplib::Response& res);

private:
    transaction::BranchManager& branch_manager_;
    
    // Helper methods
    void sendJson(httplib::Response& res, const json& data, int status_code = 200);
    /**
     * @brief TBD: Describe sendError.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message);
    /**
     * @brief TBD: Describe parseJsonBody.
     * @param[in] req Input parameter.
     * @param[in,out] out Input/output parameter.
     * @param[in,out] res Input/output parameter.
     * @return True on success.
     */
    bool parseJsonBody(const httplib::Request& req, json& out, httplib::Response& res);
};

} // namespace server
} // namespace themis
