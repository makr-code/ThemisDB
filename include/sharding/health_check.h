/**
 * @file health_check.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace themis {
namespace sharding {

/** @brief Health severity level for shard or cluster state. */
enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    UNHEALTHY,
    CRITICAL
};

/** @brief Health snapshot for one shard endpoint. */
struct ShardHealthInfo {
    /** @brief Shard identifier. */
    std::string shard_id;
    /** @brief Current health severity classification. */
    HealthStatus status;
    /** @brief True when certificate could be loaded and is not expired. */
    bool cert_valid;
    /** @brief Seconds remaining until certificate expiry (0 on invalid cert). */
    int64_t cert_expiry_seconds;
    /** @brief True when storage check request succeeded. */
    bool storage_ok;
    /** @brief Storage usage percentage as reported or derived from metrics. */
    double storage_usage_percent;
    /** @brief True when network health probe succeeded. */
    bool network_ok;
    /** @brief Measured health-probe response time in milliseconds. */
    double response_time_ms;
    /** @brief Non-fatal observations impacting health level. */
    std::vector<std::string> warnings;
    /** @brief Fatal observations causing unhealthy/critical state. */
    std::vector<std::string> errors;
};

/** @brief Aggregated health snapshot across all monitored shards. */
struct ClusterHealthInfo {
    /** @brief Computed aggregate cluster health status. */
    HealthStatus cluster_status;
    /** @brief Total number of monitored shards. */
    int total_shards;
    /** @brief Count of shards in HEALTHY state. */
    int healthy_shards;
    /** @brief Count of shards in DEGRADED state. */
    int degraded_shards;
    /** @brief Count of shards in UNHEALTHY state. */
    int unhealthy_shards;
    /** @brief Count of shards in CRITICAL state. */
    int critical_shards;
    /** @brief True when strict majority of shards are healthy. */
    bool has_quorum;
    /** @brief Per-shard health snapshots used for aggregation. */
    std::vector<ShardHealthInfo> shard_health;
    /** @brief Cluster-level warnings (e.g., quorum loss). */
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
    /** @brief Runtime configuration for periodic health checks and thresholds. */
    struct Config {
        /** @brief Interval between periodic health-check rounds in milliseconds. */
        int check_interval_ms = 30000;           // 30 seconds
        /** @brief Warning threshold for certificate expiry (in days). */
        int cert_expiry_warning_days = 7;
        /** @brief Storage usage threshold for DEGRADED status. */
        int storage_warning_percent = 90;
        /** @brief Storage usage threshold for CRITICAL status. */
        int storage_critical_percent = 95;
        /** @brief Response-time threshold for DEGRADED status (ms). */
        int response_time_degraded_ms = 100;
        /** @brief Response-time threshold for UNHEALTHY status (ms). */
        int response_time_unhealthy_ms = 500;
        /** @brief Enable optional auto-remediation hooks (if implemented). */
        bool enable_auto_remediation = false;
        /** @brief CA certificate path used by mTLS client probes. */
        std::string ca_cert_path;
    };

    /** @brief Callback signature for periodic cluster-health updates. */
    using HealthCheckCallback = std::function<void(const ClusterHealthInfo&)>;

    /** @brief Construct health-check system with provided runtime config. */
    explicit HealthCheckSystem(const Config& config);
    /** @brief Destructor stops periodic checks and joins worker thread. */
    ~HealthCheckSystem();

    /** @brief Perform one full health check for a specific shard endpoint. */
    ShardHealthInfo checkShardHealth(const std::string& shard_id, 
                                      const std::string& endpoint,
                                      const std::string& cert_path);

    /** @brief Perform health checks for all shards and aggregate cluster status. */
    ClusterHealthInfo checkClusterHealth(const std::map<std::string, std::string>& shard_endpoints);

    /** @brief Register callback invoked after each periodic cluster check. */
    void registerCallback(HealthCheckCallback callback);

    /** @brief Start background periodic health-check loop. */
    void startPeriodicChecks(const std::map<std::string, std::string>& shard_endpoints);

    /** @brief Stop background periodic health-check loop. */
    void stopPeriodicChecks();

    /** @brief Return latest cluster health snapshot from periodic checks. */
    ClusterHealthInfo getCurrentHealth() const;

private:
    Config config_;
    mutable std::mutex state_mutex_;
    HealthCheckCallback callback_;
    ClusterHealthInfo current_health_;
    std::atomic<bool> running_{false};
    mutable std::mutex lifecycle_mutex_;
    std::thread periodic_thread_;
    mutable std::mutex cv_mutex_;
    std::condition_variable cv_;

    /** @brief Validate certificate file and compute remaining lifetime. */
    bool checkCertificateValidity(const std::string& cert_path, int64_t& seconds_until_expiry);
    /** @brief Probe storage usage for a shard endpoint. */
    bool checkStorageCapacity(const std::string& endpoint, double& usage_percent);
    /** @brief Probe network connectivity and measure response latency. */
    bool checkNetworkConnectivity(const std::string& endpoint, double& response_time_ms);
    
    /** @brief Aggregate per-shard statuses into one cluster-level status. */
    HealthStatus aggregateHealth(const std::vector<ShardHealthInfo>& shard_health);
    /** @brief Evaluate strict-majority quorum condition. */
    bool hasQuorum(int healthy_shards, int total_shards);
};

} // namespace sharding
} // namespace themis
