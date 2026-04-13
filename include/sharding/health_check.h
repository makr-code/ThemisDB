/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            health_check.h                                     ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     133                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_SHARDING_HEALTH_CHECK_H
#define THEMIS_SHARDING_HEALTH_CHECK_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <functional>

namespace themis {
namespace sharding {

enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    CRITICAL
};

struct ShardHealthInfo {
    std::string shard_id;
    HealthStatus status;
    bool cert_valid;
    int64_t cert_expiry_seconds;
    bool storage_ok;
    double storage_usage_percent;
    bool network_ok;
    double response_time_ms;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct ClusterHealthInfo {
    HealthStatus cluster_status;
    int total_shards;
    int healthy_shards;
    int degraded_shards;
    int unhealthy_shards;
    int critical_shards;
    bool has_quorum;
    std::vector<ShardHealthInfo> shard_health;
    std::vector<std::string> cluster_warnings;
};

/**
 * Health check system for monitoring shard and cluster health.
 * 
 * Performs periodic health checks on shards including:
 * - Certificate validity and expiration
 * - Storage capacity
 * - Network connectivity
 * - Response time
 * 
 * Aggregates shard health into cluster-wide status.
 */
class HealthCheckSystem {
public:
    struct Config {
        int check_interval_ms = 30000;           // 30 seconds
        int cert_expiry_warning_days = 7;
        int storage_warning_percent = 90;
        int storage_critical_percent = 95;
        int response_time_degraded_ms = 100;
        int response_time_unhealthy_ms = 500;
        bool enable_auto_remediation = false;
        std::string ca_cert_path;
    };

    using HealthCheckCallback = std::function<void(const ClusterHealthInfo&)>;

    explicit HealthCheckSystem(const Config& config);
    ~HealthCheckSystem() = default;

    // Perform health check on single shard
    ShardHealthInfo checkShardHealth(const std::string& shard_id, 
                                      const std::string& endpoint,
                                      const std::string& cert_path);

    // Perform health check on entire cluster
    ClusterHealthInfo checkClusterHealth(const std::map<std::string, std::string>& shard_endpoints);

    // Register callback for health status changes
    void registerCallback(HealthCheckCallback callback);

    // Start periodic health checks
    void startPeriodicChecks(const std::map<std::string, std::string>& shard_endpoints);

    // Stop periodic health checks
    void stopPeriodicChecks();

    // Get current cluster health
    ClusterHealthInfo getCurrentHealth() const;

private:
    Config config_;
    HealthCheckCallback callback_;
    ClusterHealthInfo current_health_;
    bool running_ = false;

    bool checkCertificateValidity(const std::string& cert_path, int64_t& seconds_until_expiry);
    bool checkStorageCapacity(const std::string& endpoint, double& usage_percent);
    bool checkNetworkConnectivity(const std::string& endpoint, double& response_time_ms);
    
    HealthStatus aggregateHealth(const std::vector<ShardHealthInfo>& shard_health);
    bool hasQuorum(int healthy_shards, int total_shards);
};

} // namespace sharding
} // namespace themis

#endif // THEMIS_SHARDING_HEALTH_CHECK_H
