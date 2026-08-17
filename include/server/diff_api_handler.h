/**
 * @file diff_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifndef THEMIS_ENABLE_HTTP_SERVER
#define THEMIS_ENABLE_HTTP_SERVER 1
#endif

#include "analytics/diff_engine.h"
#include <memory>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_HTTP_SERVER
#include <httplib.h>
#endif

namespace themis {
namespace server {

using json = nlohmann::json;

/**
 * @brief REST API handler for Diff operations
 * 
 * Provides HTTP endpoints for computing structured diffs between
 * two points in time (sequences, timestamps, or tags).
 * 
 * Endpoints:
 * - GET /api/v1/diff - Compute diff with query parameters
 * - GET /api/v1/diff/cache/stats - Get cache statistics
 * - DELETE /api/v1/diff/cache - Clear diff cache
 */
class DiffApiHandler {
public:
    /**
     * @brief Construct DiffApiHandler
     * @param diff_engine Reference to DiffEngine instance
     */
    explicit DiffApiHandler(analytics::DiffEngine& diff_engine);
    
    ~DiffApiHandler() = default;

    // Disable copy, allow move
    DiffApiHandler(const DiffApiHandler&) = delete;
    DiffApiHandler& operator=(const DiffApiHandler&) = delete;
    DiffApiHandler(DiffApiHandler&&) noexcept noexcept = default;
    DiffApiHandler& operator=(DiffApiHandler&&) noexcept noexcept = default;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Register routes with HTTP server
     * @param server HTTP server instance
     */
    void registerRoutes(httplib::Server& server);

public:
    analytics::DiffEngine& diff_engine_;

    /**
     * @brief Handle GET /api/v1/diff
     * 
     * Query parameters:
     * - from: Start point (sequence number or ISO 8601 timestamp)
     * - to: End point (sequence number or ISO 8601 timestamp)
     * - from_tag: Start tag name (requires Phase 1)
     * - to_tag: End tag name (requires Phase 1)
     * - table: Filter by table name (optional)
     * - key_prefix: Filter by key prefix (optional)
     * - include_values: Include actual values (default: true)
     * - limit: Maximum changes to return (default: 1000)
     * - offset: Skip first N changes (default: 0)
     */
    void handleGetDiff(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle GET /api/v1/diff/cache/stats
     */
    void handleGetCacheStats(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle DELETE /api/v1/diff/cache
     */
    void handleClearCache(const httplib::Request& req, httplib::Response& res);

private:

    /**
     * @brief Parse query parameters into DiffOptions
     */
    analytics::DiffEngine::DiffOptions parseOptions(const httplib::Request& req) const;

    /**
     * @brief Parse timestamp from string (ISO 8601 or milliseconds)
     */
    int64_t parseTimestamp(const std::string& str) const;

    /**
     * @brief Check if string is a sequence number
     */
    bool isSequenceNumber(const std::string& str) const;

    /**
     * @brief Create error response
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    /**
     * @brief Create success response with JSON body
     */
    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
#else
private:
    analytics::DiffEngine& diff_engine_;
#endif
};

} // namespace server
} // namespace themis
