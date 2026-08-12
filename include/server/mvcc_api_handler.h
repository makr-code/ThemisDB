/**
 * @file mvcc_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief REST API handler for MVCC (Multi-Version Concurrency Control)
 *
 * Exposes the MVCCStore over HTTP so callers can perform versioned reads and
 * writes and inspect/manage per-record version histories.
 *
 * ## Endpoints
 *
 * | Method | Path | Description |
 * |--------|------|-------------|
 * | GET | /api/v1/mvcc/keys/{key} | Read the latest value of a key |
 * | GET | /api/v1/mvcc/keys/{key}?timestamp={ts} | Read the value at timestamp ts |
 * | POST | /api/v1/mvcc/keys/{key} | Write a new version (returns commit timestamp) |
 * | GET | /api/v1/mvcc/keys/{key}/versions | List all stored versions |
 * | DELETE | /api/v1/mvcc/keys/{key}/versions | GC versions before a watermark |
 * | GET | /api/v1/mvcc/clock | Get the current HLC timestamp |
 * | GET | /api/v1/mvcc/stats | Prometheus-compatible MVCC stats as JSON |
 *
 * ### Read a key (latest version)
 * ```
 * GET /api/v1/mvcc/keys/user%3A42
 * 200 { "key": "user:42", "value": "...", "timestamp": 1234567890050007 }
 * 404 { "error": "key not found" }
 * ```
 *
 * ### Snapshot read at a specific HLC timestamp
 * ```
 * GET /api/v1/mvcc/keys/user%3A42?timestamp=1234567890050000
 * 200 { "key": "user:42", "value": "...", "timestamp": 1234567890050000 }
 * 404 { "error": "no version found at or before timestamp 1234567890050000" }
 * ```
 *
 * ### Write a new version
 * ```
 * POST /api/v1/mvcc/keys/user%3A42
 * Body: { "value": "hello" }
 * 201 { "key": "user:42", "timestamp": 1234567890050010 }
 * ```
 *
 * ### List all versions
 * ```
 * GET /api/v1/mvcc/keys/user%3A42/versions
 * 200 { "key": "user:42", "versions": [
 *         { "timestamp": 1234567890050000, "value": "old" },
 *         { "timestamp": 1234567890050010, "value": "new" }
 *       ] }
 * ```
 *
 * ### GC versions before a watermark
 * ```
 * DELETE /api/v1/mvcc/keys/user%3A42/versions
 * Body: { "before_timestamp": 1234567890050005, "min_versions_to_keep": 1 }
 * 200 { "key": "user:42", "versions_deleted": 1 }
 * ```
 *
 * ### Clock
 * ```
 * GET /api/v1/mvcc/clock
 * 200 { "timestamp": 1234567890050010, "physical_ms": 1234567890, "logical": 10 }
 * ```
 *
 * ### Stats
 * ```
 * GET /api/v1/mvcc/stats
 * 200 { "writes_total": 42, "reads_total": 100, "gc_runs_total": 5, ... }
 * ```
 */
class MvccApiHandler {
public:
    /**
     * @brief Construct MvccApiHandler.
     *
     * @param store  Shared MVCC store instance.
     * @param metrics Optional Prometheus metrics instance (may be nullptr).
     */
    explicit MvccApiHandler(
        std::shared_ptr<MVCCStore> store,
        std::shared_ptr<sharding::PrometheusMetrics> metrics = nullptr
    );

    ~MvccApiHandler() = default;

    MvccApiHandler(const MvccApiHandler&) = delete;
    MvccApiHandler& operator=(const MvccApiHandler&) = delete;
    MvccApiHandler(MvccApiHandler&&) = default;
    MvccApiHandler& operator=(MvccApiHandler&&) = default;

    /**
     * @brief Register all MVCC routes with an httplib Server.
     * @param server The HTTP server to register routes on.
     */
    void registerRoutes(httplib::Server& server);

    // ─── Route handlers (public for testing) ────────────────────────────────

    /** GET /api/v1/mvcc/keys/{key}[?timestamp={ts}] */
    void handleGetKey(const httplib::Request& req, httplib::Response& res);

    /** POST /api/v1/mvcc/keys/{key} */
    void handlePutKey(const httplib::Request& req, httplib::Response& res);

    /** GET /api/v1/mvcc/keys/{key}/versions */
    void handleListVersions(const httplib::Request& req, httplib::Response& res);

    /** DELETE /api/v1/mvcc/keys/{key}/versions */
    void handleGcVersions(const httplib::Request& req, httplib::Response& res);

    /** GET /api/v1/mvcc/clock */
    void handleGetClock(const httplib::Request& req, httplib::Response& res);

    /** GET /api/v1/mvcc/stats */
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

    // ─── Helpers ─────────────────────────────────────────────────────────
    void sendError(httplib::Response& res, int status_code,
                   const std::string& message) const;
    void sendJson(httplib::Response& res, const json& data,
                  int status_code = 200) const;

    /** Extract the URL-decoded key from req.matches[1]. */
    static std::string extractKey(const httplib::Request& req);

    /** Convert a std::vector<uint8_t> to a std::string for JSON embedding. */
    static std::string valueToString(const std::vector<uint8_t>& v);

    /** Convert a std::string to a std::vector<uint8_t>. */
    static std::vector<uint8_t> stringToValue(const std::string& s);
};

} // namespace server
} // namespace themis
