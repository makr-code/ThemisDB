/**
 * @file replication_topology_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=3, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ReplicationTopologyApiHandler
 *
 * REST API + embedded web UI for visualizing the live WAL-replication topology.
 *
 * Endpoints
 *   GET /api/v1/replication/topology   – per-replica snapshot (JSON)
 *   GET /api/v1/replication/health     – aggregated health summary (JSON)
 *   GET /ui/replication/topology       – interactive SVG topology viewer (HTML)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server/replication_topology_api_handler.h"

#include <algorithm>
#include <sstream>
#include "utils/input_validator.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

constexpr size_t kMaxReplicationUiPrefixLength = 256;

bool isValidUiApiBasePrefix(const std::string& value) {
    if (value.empty()) {
        return true;
    }

    if (value.front() != '/') {
        return false;
    }

    themis::utils::InputValidator validator;
    if (!validator.validateStringLength(value, kMaxReplicationUiPrefixLength) ||
        !validator.validateHeaderValue(value) ||
        value.find("//") != std::string::npos) {
        return false;
    }

    size_t start = 1;
    while (start <= value.size()) {
        const auto end = value.find('/', start);
        const auto len = (end == std::string::npos) ? value.size() - start : end - start;
        if (len > 0) {
            const auto segment = value.substr(start, len);
            if (!validator.validatePathSegment(segment)) {
                return false;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return true;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ReplicationTopologyApiHandler::ReplicationTopologyApiHandler(
    std::shared_ptr<sharding::ReplicationCoordinator> coordinator,
    std::shared_ptr<sharding::WALManager>             wal_manager,
    std::string                                       primary_id,
    std::shared_ptr<AuthMiddleware>                   auth)
    : coordinator_(std::move(coordinator))
    , wal_manager_(std::move(wal_manager))
    , primary_id_(std::move(primary_id))
    , auth_(std::move(auth))
{
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/replication/topology
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ReplicationTopologyApiHandler::handleTopologyGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleTopologyGet");
    if (!coordinator_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Replication not configured", req);
    }
    auto& coordinator = *coordinator_;

    try {
        const auto replicas = coordinator.getReplicaInfo();
        const uint64_t primary_lsn = wal_manager_
            ? wal_manager_->getCurrentLSN().segment : 0;

        // Build primary (this) node entry
        json primary_node = {
            {"node_id",             primary_id_.empty() ? "primary" : primary_id_},
            {"role",                "PRIMARY"},
            {"is_primary",          true},
            {"health_status",       "HEALTHY"},
            {"replication_lag_ms",  0},
            {"replication_lag_bytes", 0},
            {"endpoint",            ""},
            {"last_confirmed_lsn",  primary_lsn}
        };

        json nodes = json::array();
        nodes.push_back(primary_node);

        // Build replica entries
        for (const auto& r : replicas) {
            json node = {
                {"node_id",              r.replica_id},
                {"role",                 "REPLICA"},
                {"is_primary",           false},
                {"health_status",        r.is_healthy ? "HEALTHY" : "FAILED"},
                {"replication_lag_ms",   r.lag_ms},
                {"replication_lag_bytes", r.lag_bytes},
                {"endpoint",             r.endpoint},
                {"consecutive_failures", r.consecutive_failures},
                {"last_confirmed_lsn",   r.last_confirmed_lsn.segment}
            };
            nodes.push_back(std::move(node));
        }

        // Build directed edges: primary → each replica
        json edges = json::array();
        const std::string local_id = primary_id_.empty() ? "primary" : primary_id_;
        for (const auto& r : replicas) {
            edges.push_back({
                {"from", local_id},
                {"to",   r.replica_id},
                {"type", "WAL_STREAM"}
            });
        }

        json response_body = {
            {"primary_node_id", local_id},
            {"primary_lsn",     primary_lsn},
            {"nodes",           nodes},
            {"edges",           edges},
            {"total_nodes",     nodes.size()},
            {"replica_count",   replicas.size()}
        };

        return makeResponse(http::status::ok, response_body.dump(),
                            "application/json", req);

    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> ReplicationTopologyApiHandler::handleHealthGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleHealthGet");
    if (!coordinator_) {
        return makeErrorResponse(http::status::service_unavailable,
                                 "Replication not configured", req);
    }
    auto& coordinator = *coordinator_;

    try {
        const auto replicas = coordinator.getReplicaInfo();
        const auto stats = coordinator.getShipperStats();

        const auto healthy_replicas = static_cast<uint64_t>(std::count_if(
            replicas.begin(), replicas.end(),
            [](const auto& r) { return r.is_healthy; }));
        const auto failed_replicas = static_cast<uint64_t>(
            replicas.size()) - healthy_replicas;

        uint64_t max_lag_ms = 0;
        uint64_t max_lag_bytes = 0;
        for (const auto& r : replicas) {
            if (r.lag_ms > max_lag_ms) {
                max_lag_ms = r.lag_ms;
            }
            if (r.lag_bytes > max_lag_bytes) {
                max_lag_bytes = r.lag_bytes;
            }
        }

        json response_body = {
            {"overall_status", healthy_replicas == replicas.size() ? "HEALTHY" : "DEGRADED"},
            {"total_nodes", replicas.size() + 1},
            {"healthy_replicas", healthy_replicas},
            {"failed_replicas", failed_replicas},
            {"max_replication_lag_ms", max_lag_ms},
            {"max_replication_lag_bytes", max_lag_bytes},
            {"has_quorum", failed_replicas == 0},
            {"total_entries_shipped", stats.total_entries_shipped},
            {"total_bytes_shipped", stats.total_bytes_shipped},
            {"failed_ships", stats.failed_ships},
            {"retries", stats.retries}
        };

        return makeResponse(http::status::ok, response_body.dump(),
                            "application/json", req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> ReplicationTopologyApiHandler::handleUiGet(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("handleUiGet");
    std::string api_base;
    const std::string target{req.target()};
    const std::string marker = "/ui/replication/topology";
    const auto pos = target.find(marker);
    if (pos != std::string::npos) {
        api_base = target.substr(0, pos);
        if (!isValidUiApiBasePrefix(api_base)) {
            return makeErrorResponse(http::status::bad_request,
                                     "Invalid UI API base prefix", req);
        }
    }

    return makeResponse(http::status::ok, buildUiHtml(api_base), "text/html; charset=utf-8", req);
}

http::response<http::string_body> ReplicationTopologyApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req) const
{
    json body = {
        {"error", message},
        {"status", static_cast<unsigned>(status)}
    };
    return makeResponse(status, body.dump(), "application/json", req);
}

http::response<http::string_body> ReplicationTopologyApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const std::string& content_type,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "themis");
    res.set(http::field::content_type, content_type);
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

std::string ReplicationTopologyApiHandler::buildUiHtml(const std::string& api_base)
{
    const std::string encoded_api_base = json(api_base).dump();

    std::ostringstream html;
    html << "<!doctype html>\n"
         << "<html lang=\"en\">\n"
         << "<head><meta charset=\"utf-8\">\n"
         << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
         << "<title>Themis Replication Topology</title>\n"
         << "<style>body{font-family:system-ui,sans-serif;margin:16px}"
         << "h1{margin:0 0 12px 0;font-size:20px}"
         << "pre{background:#111827;color:#e5e7eb;padding:12px;border-radius:8px;overflow:auto}"
         << "#error{color:#b91c1c;margin:8px 0;display:none}</style></head>\n"
         << "<body><h1>Replication Topology</h1><div id=\"error\"></div>\n"
         << "<h2>Topology</h2><pre id=\"topology\">loading...</pre>\n"
         << "<h2>Health</h2><pre id=\"health\">loading...</pre>\n"
         << "<script>const API_BASE=" << encoded_api_base << ";\n"
         << "const errorEl=document.getElementById('error');\n"
         << "async function load(){try{\n"
         << "const t=await fetch(API_BASE+'/api/v1/replication/topology');\n"
         << "const h=await fetch(API_BASE+'/api/v1/replication/health');\n"
         << "if(!t.ok)throw new Error('topology '+t.status);\n"
         << "if(!h.ok)throw new Error('health '+h.status);\n"
         << "document.getElementById('topology').textContent=JSON.stringify(await t.json(),null,2);\n"
         << "document.getElementById('health').textContent=JSON.stringify(await h.json(),null,2);\n"
         << "errorEl.style.display='none';\n"
         << "}catch(e){errorEl.style.display='block';errorEl.textContent='Fetch failed: '+e.message;}}\n"
         << "load();setInterval(load,5000);</script></body></html>\n";

    return html.str();
}

} // namespace server
} // namespace themis
