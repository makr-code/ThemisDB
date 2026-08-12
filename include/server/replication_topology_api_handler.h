/**
 * @file replication_topology_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "sharding/replication_coordinator.h"
#include "sharding/wal_manager.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

/**
 * @brief REST API handler and web UI for Replication Topology Visualization
 *
 * Exposes endpoints for monitoring and visualizing the WAL-based replication
 * topology:
 *
 *   GET /api/v1/replication/topology  - Full topology: all replicas, health, lag
 *   GET /api/v1/replication/health    - Overall cluster health summary
 *   GET /ui/replication/topology      - Interactive HTML web UI (auto-refreshing)
 *
 * The web UI renders an SVG-based topology graph showing:
 *   - Primary and replica nodes colour-coded by health status
 *   - Directed edges showing WAL data flow
 *   - Per-replica replication lag (bytes and milliseconds)
 *   - Auto-refresh every 5 seconds via JavaScript fetch
 *
 * All JSON endpoints return structured data suitable for programmatic
 * consumption.  The `auth` parameter is stored for future per-route access
 * control; currently the same open-access policy as the geo topology handler
 * applies (auth is delegated to the central HttpServer middleware layer).
 */
class ReplicationTopologyApiHandler {
public:
    /**
     * @param coordinator  Live ReplicationCoordinator (may be nullptr – handler
     *                     returns 503 when replication is disabled).
     * @param wal_manager  WAL manager used to report the current write LSN.
     * @param primary_id   This node's identifier (labels the local/primary node).
     * @param auth         Auth middleware (may be nullptr to disable auth checks).
     */
    ReplicationTopologyApiHandler(
        std::shared_ptr<sharding::ReplicationCoordinator> coordinator,
        std::shared_ptr<sharding::WALManager>             wal_manager,
        std::string                                       primary_id,
        std::shared_ptr<AuthMiddleware>                   auth
    );

    /** GET /api/v1/replication/topology — full per-replica topology snapshot */
    http::response<http::string_body> handleTopologyGet(
        const http::request<http::string_body>& req);

    /** GET /api/v1/replication/health — aggregated cluster health summary */
    http::response<http::string_body> handleHealthGet(
        const http::request<http::string_body>& req);

    /** GET /ui/replication/topology — serve the interactive HTML visualizer */
    http::response<http::string_body> handleUiGet(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ReplicationCoordinator> coordinator_;
    std::shared_ptr<sharding::WALManager>             wal_manager_;
    std::string                                       primary_id_;
    std::shared_ptr<AuthMiddleware>                   auth_;

    http::response<http::string_body> makeErrorResponse(
        http::status          status,
        const std::string&    message,
        const http::request<http::string_body>& req) const;

    http::response<http::string_body> makeResponse(
        http::status          status,
        const std::string&    body,
        const std::string&    content_type,
        const http::request<http::string_body>& req) const;

    /** Build the embedded HTML page for the topology visualizer. */
    static std::string buildUiHtml(const std::string& api_base);
};

} // namespace server
} // namespace themis
