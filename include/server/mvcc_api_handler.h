/**
 * @file mvcc_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/mvcc_store.h"
#include "sharding/prometheus_metrics.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace themis {
namespace server {

using json = nlohmann::json;

class MvccApiHandler {
public:
    explicit MvccApiHandler(
        std::shared_ptr<MVCCStore> store,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );

    ~MvccApiHandler() = default;

    MvccApiHandler(const MvccApiHandler&) = delete;
    MvccApiHandler& operator=(const MvccApiHandler&) = delete;
    MvccApiHandler(MvccApiHandler&&) noexcept = default;
    MvccApiHandler& operator=(MvccApiHandler&&) noexcept = default;

    /**
     * @brief Register Routes.
     * @param[in,out] server Input/output parameter.
     */
    void registerRoutes(httplib::Server& server);


    /**
     * @brief Handle Get Key.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetKey(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Put Key.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handlePutKey(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle List Versions.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleListVersions(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Gc Versions.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGcVersions(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Clock.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetClock(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handle Get Stats.
     * @param[in] req Input parameter.
     * @param[in,out] res Input/output parameter.
     */
    void handleGetStats(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<MVCCStore>                  store_;
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;

    // ─── Accumulated stats (for /stats endpoint) ─────────────────────────
    std::atomic<uint64_t> writes_total_{0};
    std::atomic<uint64_t> reads_latest_total_{0};
    std::atomic<uint64_t> reads_snapshot_total_{0};
    std::atomic<uint64_t> gc_runs_total_{0};
    std::atomic<uint64_t> gc_versions_deleted_total_{0};

    /**
     * @brief Send Error.
     * @param[in,out] res Input/output parameter.
     * @param[in] status_code Input parameter.
     * @param[in] message Input parameter.
     */
    void sendError(httplib::Response& res, int status_code,
                   const std::string& message) const;
    void sendJson(httplib::Response& res, const json& data,
                  int status_code = 200) const;

    /**
     * @brief Extract Key.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    static std::string extractKey(const httplib::Request& req);

    /**
     * @brief Value To String.
     * @param[in] v Input parameter.
     * @return Return value.
     */
    static std::string valueToString(const std::vector<uint8_t>& v);

    /**
     * @brief String To Value.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::vector<uint8_t> stringToValue(const std::string& s);
};

} // namespace server
} // namespace themis
