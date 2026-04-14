/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin_api.h                                        ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     131                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_SHARDING_ADMIN_API_H
#define THEMIS_SHARDING_ADMIN_API_H

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

class ShardRepairEngine;  // forward declaration

/**
 * Admin API for cluster management operations.
 * 
 * Provides RESTful HTTP endpoints for:
 * - Topology management (add/remove shards)
 * - Rebalancing operations (trigger/monitor)
 * - Health monitoring (includes per-shard repair status when ShardRepairEngine is set)
 * - Routing statistics
 * - Shard repair / anti-entropy (rebuild triggers & status)
 * 
 * All endpoints require operator certificate for authorization.
 */
class AdminAPI {
public:
    struct Config {
        int http_port = 8080;
        std::string operator_cert_path;
        std::string ca_cert_path;
        bool require_signatures = true;
        bool enable_audit_log = true;
        std::string audit_log_path = "/var/log/themis/admin_audit.log";
    };

    using RequestHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    explicit AdminAPI(const Config& config);
    ~AdminAPI() = default;

    // Register handlers
    void registerTopologyHandler(RequestHandler handler);
    void registerRebalanceHandler(RequestHandler handler);
    void registerHealthHandler(RequestHandler handler);
    void registerStatsHandler(RequestHandler handler);
    /// Register handler for repair / anti-entropy operations.
    void registerRepairHandler(RequestHandler handler);

    /**
     * Attach a ShardRepairEngine so that GET /admin/health automatically
     * enriches its response with per-shard repair health reports.
     * The engine is optional; without it the health response is unchanged.
     */
    void setRepairEngine(std::shared_ptr<ShardRepairEngine> engine);

    // Handle HTTP request
    nlohmann::json handleRequest(const std::string& method, 
                                  const std::string& path,
                                  const nlohmann::json& body,
                                  const std::string& operator_cert);

    // Endpoints
    struct Endpoints {
        static constexpr const char* TOPOLOGY = "/admin/topology";
        static constexpr const char* SHARD_ADD = "/admin/shard/add";
        static constexpr const char* SHARD_REMOVE = "/admin/shard/";  // + {id}
        static constexpr const char* REBALANCE = "/admin/rebalance";
        static constexpr const char* REBALANCE_STATUS = "/admin/rebalance/";  // + {id}
        static constexpr const char* HEALTH = "/admin/health";
        static constexpr const char* STATS = "/admin/stats";
        static constexpr const char* CERTS = "/admin/certs";
        
        // Capability management endpoints
        static constexpr const char* CAPABILITIES = "/admin/capabilities";                    // GET all
        static constexpr const char* SHARD_CAPABILITIES_GET = "/admin/shard/{shard_id}/capabilities";  // GET single
        static constexpr const char* SHARD_CAPABILITIES_PUT = "/admin/shard/{shard_id}/capabilities";  // PUT single
        static constexpr const char* CAPABILITIES_BULK = "/admin/capabilities/bulk";          // POST bulk update

        // Repair / anti-entropy endpoints
        /// POST /admin/repair         – trigger repair (body: {"shard_id":"..."} or {})
        static constexpr const char* REPAIR = "/admin/repair";
        /// POST /admin/repair/scan    – trigger full anti-entropy scan
        static constexpr const char* REPAIR_SCAN = "/admin/repair/scan";
        /// GET  /admin/repair/{job_id} – query repair job status
        static constexpr const char* REPAIR_STATUS = "/admin/repair/";  // + {job_id}
    };

private:
    Config config_;
    RequestHandler topology_handler_;
    RequestHandler rebalance_handler_;
    RequestHandler health_handler_;
    RequestHandler stats_handler_;
    RequestHandler repair_handler_;
    std::shared_ptr<ShardRepairEngine> repair_engine_;

    bool authorizeRequest(const std::string& operator_cert);
    void auditLog(const std::string& method, const std::string& path, const std::string& operator_cert);
    nlohmann::json createErrorResponse(int code, const std::string& message);
    /// Build the repair health section for GET /admin/health.
    nlohmann::json buildRepairHealthJson() const;
};

} // namespace sharding
} // namespace themis

#endif // THEMIS_SHARDING_ADMIN_API_H
