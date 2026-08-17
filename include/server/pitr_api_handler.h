/**
 * @file pitr_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
     */
    explicit PITRApiHandler(PITRManager& pitr_manager);
    
    ~PITRApiHandler() = default;

    // Disable copy, allow move
    PITRApiHandler(const PITRApiHandler&) = delete;
    PITRApiHandler& operator=(const PITRApiHandler&) = delete;
    PITRApiHandler(PITRApiHandler&&) noexcept noexcept = default;
    PITRApiHandler& operator=(PITRApiHandler&&) noexcept noexcept = default;

    /**
     * Register routes with HTTP server
     */
    void registerRoutes(httplib::Server& server);
    
    /**
     * Handle POST /api/v1/restore/pitr
     */
    void handleRestore(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle POST /api/v1/restore/preview
     */
    void handlePreview(const httplib::Request& req, httplib::Response& res);

    /**
     * Handle GET /api/v1/restore/progress
     */
    void handleGetProgress(const httplib::Request& req, httplib::Response& res);

private:
    PITRManager& pitr_manager_;

    /**
     * Parse restore options from JSON request
     */
    PITRManager::RestoreOptions parseRestoreOptions(const json& options_json) const;

    /**
     * Helper: Convert progress to JSON
     */
    json progressToJson(const PITRManager::RestoreProgress& progress) const;

    /**
     * Helper: Convert preview to JSON
     */
    json previewToJson(const PITRManager::RestorePreview& preview) const;

    /**
     * Helper: Convert status to JSON
     */
    json statusToJson(const PITRManager::Status& status) const;

    /**
     * Helper: Convert phase enum to string
     */
    std::string phaseToString(PITRManager::RestoreProgress::Phase phase) const;

    /**
     * Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
};

} // namespace server
} // namespace themis
