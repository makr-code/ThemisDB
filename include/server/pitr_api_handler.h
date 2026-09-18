/**
 * @file pitr_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/pitr_manager.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace themis {
namespace server {

using json = nlohmann::json;

class PITRApiHandler {
public:
    /**
     * @brief PITRApi Handler.
     * @param[in,out] pitr_manager Input/output parameter.
     * @return Return value.
     */
    explicit PITRApiHandler(PITRManager& pitr_manager);
    
    ~PITRApiHandler() = default;

    // Disable copy, allow move
    PITRApiHandler(const PITRApiHandler&) = delete;
    PITRApiHandler& operator=(const PITRApiHandler&) = delete;
    PITRApiHandler(PITRApiHandler&&) noexcept = default;
    PITRApiHandler& operator=(PITRApiHandler&&) noexcept = default;

    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * @brief Handle Restore.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Preview.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Progress.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

private:
    PITRManager& pitr_manager_;

    /**
     * @brief Parse Restore Options.
     * @param[in] options_json Input parameter.
     * @return Return value.
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& options_json) const;

    /**
     * @brief Progress To Json.
     * @param[in] progress Input parameter.
     * @return Return value.
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * @brief Preview To Json.
     * @param[in] preview Input parameter.
     * @return Return value.
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * @brief Status To Json.
     * @param[in] status Input parameter.
     * @return Return value.
     */
    json statusToJson(const PITRManager::Status& status) const;

    /**
     * @brief Phase To String.
     * @param[in] phase Input parameter.
     * @return Return value.
     */
    std::string phaseToString(PITRManager::RestoreProgress::Phase phase) const;

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
