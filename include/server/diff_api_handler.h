/**
 * @file diff_api_handler.h
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

#include "analytics/diff_engine.h"
#include <memory>
#include <nlohmann/json.hpp>

#ifdef THEMIS_ENABLE_HTTP_SERVER
#include <httplib.h>
#endif

namespace themis {
namespace server {

using json = nlohmann::json;

class DiffApiHandler {
public:
    /**
     * @brief Diff Api Handler.
     * @param[in,out] diff_engine Input/output parameter.
     * @return Return value.
     */
    explicit DiffApiHandler(analytics::DiffEngine& diff_engine);
    
    ~DiffApiHandler() = default;

    // Disable copy, allow move
    DiffApiHandler(const DiffApiHandler&) = delete;
    DiffApiHandler& operator=(const DiffApiHandler&) = delete;
    DiffApiHandler(DiffApiHandler&&) noexcept = default;
    DiffApiHandler& operator=(DiffApiHandler&&) noexcept = default;

#ifdef THEMIS_ENABLE_HTTP_SERVER
    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);

public:
    analytics::DiffEngine& diff_engine_;

    /**
     * @brief Handle Get Diff.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetDiff(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Cache Stats.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetCacheStats(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Clear Cache.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleClearCache(const httplib::Request& req, httplib::Response& res);

private:

    /**
     * @brief Parse Options.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    analytics::DiffEngine::DiffOptions parseOptions(const httplib::Request& req) const;

    /**
     * @brief Parse Timestamp.
     * @param[in] str Input parameter.
     * @return Return value.
     */
    int64_t parseTimestamp(const std::string& str) const;

    /**
     * @brief Is Sequence Number.
     * @param[in] str Input parameter.
     * @return True when the operation succeeds.
     */
    bool isSequenceNumber(const std::string& str) const;

    /**
     * @brief Send Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code, const std::string& message) const;

    void sendJson(httplib::Response& res, const json& data, int status_code = 200) const;
#else
private:
    analytics::DiffEngine& diff_engine_;
#endif
};

} // namespace server
} // namespace themis
