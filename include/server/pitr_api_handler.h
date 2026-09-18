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

/**
 * REST API handler for Point-in-Time Recovery (PITR) operations
 * 
 * Provides HTTP endpoints for database restore operations:
 * - POST /api/v1/restore/pitr - Execute restore to sequence/tag/timestamp
 * - POST /api/v1/restore/preview - Preview restore operation (dry-run)
 * - GET /api/v1/restore/progress - Get current restore progress
 * 
 * Thread-safety: NOT thread-safe. Only one restore operation at a time.
 */
class PITRApiHandler {
public:
    /**
     * Construct PITRApiHandler
     * @brief TBD: Describe PITRApiHandler.
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
     * Register routes with HTTP server
     * @brief TBD: Describe registerRoutes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * Handle POST /api/v1/restore/pitr
     * @brief TBD: Describe handleRestore.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle POST /api/v1/restore/preview
     * @brief TBD: Describe handlePreview.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle GET /api/v1/restore/progress
     * @brief TBD: Describe handleGetProgress.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

private:
    PITRManager& pitr_manager_;

    /**
     * Parse restore options from JSON request
     * @brief TBD: Describe parseRestoreOptions.
     * @param[in] options_json Input parameter.
     * @return Return value.
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& options_json) const;

    /**
     * Helper: Convert progress to JSON
     * @brief TBD: Describe progressToJson.
     * @param[in] progress Input parameter.
     * @return Return value.
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * Helper: Convert preview to JSON
     * @brief TBD: Describe previewToJson.
     * @param[in] preview Input parameter.
     * @return Return value.
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * Helper: Convert status to JSON
     * @brief TBD: Describe statusToJson.
     * @param[in] status Input parameter.
     * @return Return value.
     */
    json statusToJson(const PITRManager::Status& status) const;

    /**
     * Helper: Convert phase enum to string
     * @brief TBD: Describe phaseToString.
     * @param[in] phase Input parameter.
     * @return Return value.
     */
    std::string phaseToString(PITRManager::RestoreProgress::Phase phase) const;

    /**
     * Create error response
     * @brief TBD: Describe sendError.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis
