/**
 * @file snapshot_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "transaction/snapshot_manager.h"
#include <httplib.h>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

class SnapshotApiHandler {
public:
    /**
     * @brief Snapshot Api Handler.
     * @param[in,out] snapshot_manager Input/output parameter.
     * @return Return value.
     */
    explicit SnapshotApiHandler(transaction::SnapshotManager& snapshot_manager);
    
    ~SnapshotApiHandler() = default;

    // Disable copy, allow move
    SnapshotApiHandler(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler& operator=(const SnapshotApiHandler&) = delete;
    SnapshotApiHandler(SnapshotApiHandler&&) noexcept = default;
    SnapshotApiHandler& operator=(SnapshotApiHandler&&) noexcept = default;

    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * @brief Handle Create Tag.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleCreateTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle List Tags.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleListTags(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Tag.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Delete Tag.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleDeleteTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetStats(const httplib::Request& req, httplib::Response& res);

private:
    transaction::SnapshotManager& snapshot_manager_;

    /**
     * @brief Send Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis
