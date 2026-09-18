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

class BranchApiHandler {
public:
    /**
     * @brief Branch Api Handler.
     * @param[in,out] branch_manager Input/output parameter.
     * @return Return value.
     */
    explicit BranchApiHandler(transaction::BranchManager& branch_manager);
    
    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);
    
    // Route handlers
    /**
     * @brief Handle Create Branch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleCreateBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle List Branches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleListBranches(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Get Branch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Switch Branch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleSwitchBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Merge Branches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMergeBranches(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Delete Branch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleDeleteBranch(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetStats(const httplib::Request& req, httplib::Response& res);
    /**
     * @brief Handle Get Active Branch.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetActiveBranch(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Preview Merge Branches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handlePreviewMergeBranches(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Resolve Merge Branches.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleResolveMergeBranches(const httplib::Request& req, httplib::Response& res);

private:
    transaction::BranchManager& branch_manager_;
    
    // Helper methods
    void sendJson(httplib::Response& res, const json& data, int status_code = 200);
    /**
     * @brief Send Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message);
    /**
     * @brief Parse Json Body.
     * @param[in] req Input parameter.
     * @param[in,out] out Input/output parameter.
     * @param[in,out] res Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseJsonBody(const httplib::Request& req, json& out, httplib::Response& res);
};

} // namespace server
} // namespace themis
