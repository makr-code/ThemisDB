/**
 * @file mvcc_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "server/mvcc_api_handler.h"
#include <stdexcept>
#include "utils/input_validator.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <chrono>
#include "utils/tracing.h"
#include "utils/logger.h"

namespace themis {
namespace server {

namespace {

constexpr size_t kMaxMvccKeyLength = 256;

bool isValidMvccKey(const std::string& value) {
    themis::utils::InputValidator validator;
    return !value.empty() &&
           validator.validateStringLength(value, kMaxMvccKeyLength) &&
           validator.validatePathSegment(value) &&
           validator.validateHeaderValue(value);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

MvccApiHandler::MvccApiHandler(
    std::shared_ptr<MVCCStore> store,
    std::shared_ptr<sharding::PrometheusMetrics> metrics
)
    : store_(std::move(store))
    , metrics_(std::move(metrics))
{
    if (!store_) {
        throw std::invalid_argument([[maybe_unused]] "MvccApiHandler: store cannot be null");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Route registration
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::registerRoutes([[maybe_unused]] httplib::Server& server) {
    // GET  /api/v1/mvcc/clock
    server.Get("/api/v1/mvcc/clock",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetClock(req, res);
        });

    // GET  /api/v1/mvcc/stats
    server.Get("/api/v1/mvcc/stats",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetStats(req, res);
        });

    // GET  /api/v1/mvcc/keys/{key}/versions
    server.Get(R"(/api/v1/mvcc/keys/([^/]+)/versions)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleListVersions(req, res);
        });

    // DELETE /api/v1/mvcc/keys/{key}/versions
    server.Delete(R"(/api/v1/mvcc/keys/([^/]+)/versions)",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGcVersions(req, res);
        });

    // GET  /api/v1/mvcc/keys/{key}   (also handles ?timestamp= param)
    server.Get(R"(/api/v1/mvcc/keys/([^/]+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            handleGetKey(req, res);
        });

    // POST /api/v1/mvcc/keys/{key}
    server.Post(R"(/api/v1/mvcc/keys/([^/]+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            handlePutKey(req, res);
        });

    spdlog::info("MVCC API routes registered");
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/mvcc/keys/{key}[?timestamp={ts}]
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handleGetKey(const httplib::Request& req,
                                   httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetKey");
    std::string key = extractKey(req);
    if (key.empty()) {
        sendError(res, 400, "Key must not be empty");
        return;
    }

    auto t_start = std::chrono::steady_clock::now();

    try {
        std::optional<std::vector<uint8_t>> result;
        HLCTimestamp used_ts;
        std::string read_type = {};

        if (req.has_param("timestamp")) {
            // Snapshot read
            uint64_t ts_val = 0;
            try {
                ts_val = std::stoull(req.get_param_value("timestamp"));
            } catch (...) {
                THEMIS_WARN([[maybe_unused]] "mvcc_api_handler: unhandled exception caught");
                sendError(res, 400, "Invalid timestamp parameter (must be uint64)");
                return;
            }
            HLCTimestamp ts{ts_val};
            result = store_->getAtTimestamp(key, ts);
            used_ts = ts;
            read_type = "snapshot";
            ++reads_snapshot_total_;
        } else {
            // Linearizable / latest read
            result = store_->getLatest(key);
            used_ts = store_->currentTimestamp();
            read_type = "latest";
            ++reads_latest_total_;
        }

        auto t_end = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();

        if (metrics_) {
            metrics_->recordMvccRead(read_type, latency_ms);
        }

        if (!result.has_value()) {
            if (read_type == "snapshot") {
                sendError(res, 404,
                    fmt::format("No version found at or before timestamp {}",
                                used_ts.value));
            } else {
                sendError(res, 404, fmt::format("Key '{}' not found", key));
            }
            return;
        }

        json response;
        response["key"]       = key;
        response["value"]     = valueToString(*result);
        response["timestamp"] = used_ts.value;
        sendJson(res, response);

    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/mvcc/keys/{key}
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handlePutKey(const httplib::Request& req,
                                   httplib::Response& res) {
    auto span = Tracer::startSpan("handlePutKey");
    std::string key = extractKey(req);
    if (key.empty()) {
        sendError(res, 400, "Key must not be empty");
        return;
    }

    auto t_start = std::chrono::steady_clock::now();

    try {
        json body = json::parse(req.body);
        if (!body.contains("value")) {
            sendError(res, 400, "Missing required field: value");
            return;
        }
        std::string value_str = body["value"].get<std::string>();
        auto value_bytes = stringToValue(value_str);

        HLCTimestamp commit_ts = store_->put(key, value_bytes);
        ++writes_total_;

        auto t_end = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();

        if (metrics_) {
            metrics_->recordMvccWrite(latency_ms);
        }

        json response;
        response["key"]       = key;
        response["timestamp"] = commit_ts.value;
        sendJson(res, response, 201);

    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/mvcc/keys/{key}/versions
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handleListVersions(const httplib::Request& req,
                                         httplib::Response& res) {
    auto span = Tracer::startSpan("handleListVersions");
    std::string key = extractKey(req);
    if (key.empty()) {
        sendError(res, 400, "Key must not be empty");
        return;
    }

    try {
        json versions_array = json::array();
        store_->scanVersions(key, [&]([[maybe_unused]] const MVCCStore::VersionEntry& entry) {
            json v;
            v["timestamp"] = entry.timestamp.value;
            v["value"]     = valueToString(entry.value);
            versions_array.push_back(std::move(v));
            return true; // continue
        });

        json response;
        response["key"]      = key;
        response["versions"] = std::move(versions_array);
        sendJson(res, response);

    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /api/v1/mvcc/keys/{key}/versions
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handleGcVersions(const httplib::Request& req,
                                       httplib::Response& res) {
    auto span = Tracer::startSpan("handleGcVersions");
    std::string key = extractKey(req);
    if (key.empty()) {
        sendError(res, 400, "Key must not be empty");
        return;
    }

    try {
        uint64_t before_ts_val = 0;
        uint32_t min_keep = 1;

        if (!req.body.empty()) {
            json body = json::parse(req.body);
            if (!body.contains("before_timestamp")) {
                sendError(res, 400, "Missing required field: before_timestamp");
                return;
            }
            before_ts_val = body["before_timestamp"].get<uint64_t>();
            if (body.contains("min_versions_to_keep")) {
                min_keep = body["min_versions_to_keep"].get<uint32_t>();
            }
        } else {
            sendError(res, 400,
                "Request body must contain 'before_timestamp'");
            return;
        }

        HLCTimestamp watermark{before_ts_val};
        MVCCStore::GCOptions opts;
        opts.min_versions_to_keep = min_keep;

        uint64_t deleted = store_->gcVersionsBefore(key, watermark, opts);
        ++gc_runs_total_;
        gc_versions_deleted_total_ += deleted;

        if (metrics_) {
            metrics_->recordMvccGc(deleted);
        }

        json response;
        response["key"]              = key;
        response["versions_deleted"] = deleted;
        sendJson(res, response);

    } catch (const json::exception& e) {
        sendError(res, 400, fmt::format("Invalid JSON: {}", e.what()));
    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/mvcc/clock
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handleGetClock(const httplib::Request& /*req*/,
                                     httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetClock");
    try {
        HLCTimestamp ts = store_->currentTimestamp();

        json response;
        response["timestamp"]   = ts.value;
        response["physical_ms"] = ts.physical();
        response["logical"]     = ts.logical();
        sendJson(res, response);

    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/mvcc/stats
// ─────────────────────────────────────────────────────────────────────────────

void MvccApiHandler::handleGetStats(const httplib::Request& /*req*/,
                                     httplib::Response& res) {
    auto span = Tracer::startSpan("handleGetStats");
    try {
        HLCTimestamp current_ts = store_->currentTimestamp();

        json response;
        response["writes_total"]               = writes_total_.load();
        response["reads_latest_total"]         = reads_latest_total_.load();
        response["reads_snapshot_total"]       = reads_snapshot_total_.load();
        response["gc_runs_total"]              = gc_runs_total_.load();
        response["gc_versions_deleted_total"]  = gc_versions_deleted_total_.load();
        response["current_timestamp"]          = current_ts.value;
        response["current_physical_ms"]        = current_ts.physical();
        response["current_logical"]            = current_ts.logical();
        sendJson(res, response);

    } catch (const std::exception& e) {
        sendError(res, 500, fmt::format("Internal error: {}", e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string MvccApiHandler::extractKey([[maybe_unused]] const httplib::Request& req) {
    if (req.matches.size() < 2) return {};
    std::string key = req.matches[1];
    if (!isValidMvccKey(key)) {
        return {};
    }
    return key;
}

std::string MvccApiHandler::valueToString([[maybe_unused]] const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

std::vector<uint8_t> MvccApiHandler::stringToValue([[maybe_unused]] const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

void MvccApiHandler::sendError(httplib::Response& res,
                                int status_code,
                                const std::string& message) const {
    json error;
    error["error"]  = message;
    error["status"] = status_code;
    res.status = status_code;
    res.set_content(error.dump(2), "application/json");
    spdlog::warn("MVCC API error ({}): {}", status_code, message);
}

void MvccApiHandler::sendJson(httplib::Response& res,
                               const json& data,
                               int status_code) const {
    res.status = status_code;
    res.set_content(data.dump(2), "application/json");
}

} // namespace server
} // namespace themis

