/**
 * @file admin_api.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

class ShardRepairEngine;  // forward declaration
class HardwareMigrationManager;  // forward declaration

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

    /**
     * @brief TBD: Describe AdminAPI.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdminAPI(const Config& config);
    ~AdminAPI() = default;

    /**
     * @brief Register handlers
     * @param[in] handler Input parameter.
     */
    void registerTopologyHandler(RequestHandler handler);
    /**
     * @brief TBD: Describe registerRebalanceHandler.
     * @param[in] handler Input parameter.
     */
    void registerRebalanceHandler(RequestHandler handler);
    /**
     * @brief TBD: Describe registerHealthHandler.
     * @param[in] handler Input parameter.
     */
    void registerHealthHandler(RequestHandler handler);
    /**
     * @brief TBD: Describe registerStatsHandler.
     * @param[in] handler Input parameter.
     */
    void registerStatsHandler(RequestHandler handler);
    /// Register handler for repair / anti-entropy operations.
    void registerRepairHandler(RequestHandler handler);

    /**
     * Attach a ShardRepairEngine so that GET /admin/health automatically
     * enriches its response with per-shard repair health reports.
     * The engine is optional; without it the health response is unchanged.
     * @brief TBD: Describe setRepairEngine.
     * @param[in] engine Input parameter.
     */
    void setRepairEngine(std::shared_ptr<ShardRepairEngine> engine);

    /**
     * @brief Handle HTTP request
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @param[in] body Input parameter.
     * @param[in] operator_cert Input parameter.
     * @return Return value.
     */
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

        // Hardware migration endpoint (Phase 5)
        /// POST /api/v1/shards/{id}/migrate-hardware
        ///   Body: {"new_endpoint": "host:port"}
        ///   Response: {"success": bool, "shard_id": "...", "old_endpoint": "...", "new_endpoint": "..."}
        static constexpr const char* MIGRATE_HARDWARE_PREFIX = "/api/v1/shards/";  // + {id}/migrate-hardware
        static constexpr const char* MIGRATE_HARDWARE_SUFFIX = "/migrate-hardware";
    };

    /**
     * @brief Attach a HardwareMigrationManager so that
     *        `POST /api/v1/shards/{id}/migrate-hardware` is handled natively.
     *
     * Without a manager set, the endpoint returns 501 Not Implemented.
     * The manager must outlive this AdminAPI instance (or be kept alive via
     * the shared_ptr).
     * @param[in] mgr Input parameter.
     */
    void setMigrationManager(std::shared_ptr<HardwareMigrationManager> mgr);

    /**
     * @brief Register a custom handler for migrate-hardware requests.
     *
     * Used for testing / custom integration.  Overrides the built-in
     * `HardwareMigrationManager` path when set.  The body will contain at
     * minimum `{"shard_id": "...", "new_endpoint": "..."}`.
     * @param[in] handler Input parameter.
     */
    void registerMigrateHardwareHandler(RequestHandler handler);

private:
    Config config_;
    RequestHandler topology_handler_;
    RequestHandler rebalance_handler_;
    RequestHandler health_handler_;
    RequestHandler stats_handler_;
    RequestHandler repair_handler_;
    RequestHandler migrate_hardware_handler_;
    std::shared_ptr<ShardRepairEngine> repair_engine_;
    std::shared_ptr<HardwareMigrationManager> migration_manager_;

    /**
     * @brief TBD: Describe authorizeRequest.
     * @param[in] operator_cert Input parameter.
     * @return True on success.
     */
    bool authorizeRequest(const std::string& operator_cert);
    /**
     * @brief TBD: Describe auditLog.
     * @param[in] method Input parameter.
     * @param[in] path Input parameter.
     * @param[in] operator_cert Input parameter.
     */
    void auditLog(const std::string& method, const std::string& path, const std::string& operator_cert);
    /**
     * @brief TBD: Describe createErrorResponse.
     * @param[in] code Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    nlohmann::json createErrorResponse(int code, const std::string& message);
    /// Build the repair health section for GET /admin/health.
    nlohmann::json buildRepairHealthJson() const;
    /**
     * @brief TBD: Describe handleMigrateHardware.
     * @param[in] shard_id Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json handleMigrateHardware(const std::string& shard_id,
                                          const nlohmann::json& body);
};

} // namespace sharding
} // namespace themis
