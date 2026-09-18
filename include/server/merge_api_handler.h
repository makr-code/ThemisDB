/**
 * @file merge_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class MergeApiHandler {
public:
    /**
     * @brief Merge Api Handler.
     * @param[in,out] merge_engine Input/output parameter.
     * @param[in,out] snapshot_manager Input/output parameter.
     * @return Return value.
     */
    explicit MergeApiHandler(
        transaction::MergeEngine& merge_engine,
        transaction::SnapshotManager& snapshot_manager
    );
    
    ~MergeApiHandler() = default;

    // Disable copy, allow move
    MergeApiHandler(const MergeApiHandler&) = delete;
    MergeApiHandler& operator=(const MergeApiHandler&) = delete;
    MergeApiHandler(MergeApiHandler&&) noexcept = default;
    MergeApiHandler& operator=(MergeApiHandler&&) noexcept = default;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);

    /**
     * @brief Handle Merge.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMerge(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Merge Preview.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMergePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Merge By Tag.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleMergeByTag(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Can Fast Forward.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleCanFastForward(const httplib::Request& req, httplib::Response& res);
#endif

private:
    transaction::MergeEngine& merge_engine_;
    transaction::SnapshotManager& snapshot_manager_;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Parse Merge Options.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    transaction::MergeEngine::MergeOptions parseMergeOptions(const json& body) const;

    /**
     * @brief Send Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
#endif
};

} // namespace server
} // namespace themis
