/**
 * @file operational_metrics.h
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

#include <string>
#include <map>
#include <atomic>
#include <chrono>
#include <vector>
#include <memory>
#include <mutex>

namespace themisdb {
namespace sharding {

/**
 * @brief Health status levels
 */
enum class HealthStatus {
    HEALTHY,        // All systems operational
    DEGRADED,       // Some issues but still functional
    UNHEALTHY,      // Critical issues, limited functionality
    DOWN            // System is down
};

/**
 * @brief Metric type for Prometheus
 */
enum class MetricType {
    COUNTER,        // Monotonically increasing counter
    GAUGE,          // Value that can go up or down
    HISTOGRAM,      // Distribution of values
    SUMMARY         // Summary statistics
};

/**
 * @brief Individual metric value
 */
struct MetricValue {
    std::string name;
    std::string description;
    MetricType type;
    double value;
    std::map<std::string, std::string> labels;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Shard operational metrics
 */
struct ShardMetrics {
    // Throughput metrics
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> successful_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::atomic<uint64_t> read_requests{0};
    std::atomic<uint64_t> write_requests{0};
    
    // Latency metrics (in microseconds)
    std::atomic<uint64_t> total_latency_us{0};
    std::atomic<uint64_t> min_latency_us{999999999};
    std::atomic<uint64_t> max_latency_us{0};
    
    // Resource metrics
    std::atomic<uint64_t> memory_usage_bytes{0};
    std::atomic<uint64_t> disk_usage_bytes{0};
    std::atomic<uint64_t> network_bytes_sent{0};
    std::atomic<uint64_t> network_bytes_received{0};
    
    // Replication metrics
    std::atomic<uint64_t> replication_lag_ms{0};
    std::atomic<uint64_t> replica_count{0};
    std::atomic<uint64_t> sync_replica_count{0};
    
    // Consistency metrics
    std::atomic<uint64_t> quorum_writes{0};
    std::atomic<uint64_t> quorum_reads{0};
    std::atomic<uint64_t> quorum_failures{0};
    std::atomic<uint64_t> partition_events{0};
    
    // Durability metrics
    std::atomic<uint64_t> wal_syncs{0};
    std::atomic<uint64_t> checkpoints_created{0};
    std::atomic<uint64_t> recovery_operations{0};
    
    // Transaction metrics
    std::atomic<uint64_t> transactions_started{0};
    std::atomic<uint64_t> transactions_committed{0};
    std::atomic<uint64_t> transactions_aborted{0};
    std::atomic<uint64_t> transaction_conflicts{0};
    
    // Health status
    std::atomic<int> health_status{static_cast<int>(HealthStatus::HEALTHY)};
    
    // Timestamps
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point last_update_time;
    
    // Delete copy constructor and assignment (std::atomic is not copyable)
    ShardMetrics(const ShardMetrics&) = delete;
    ShardMetrics& operator=(const ShardMetrics&) = delete;
    
    // Default constructor
    ShardMetrics() = default;
    
    /**
     * @brief Get average latency in microseconds
     */
    double getAverageLatencyUs() const {
        uint64_t total_reqs = total_requests.load();
        if (total_reqs == 0) {
          return 0.0;
        }
        return static_cast<double>(total_latency_us.load()) / total_reqs;
    }
    
    /**
     * @brief Get success rate (0.0 to 1.0)
     */
    double getSuccessRate() const {
        uint64_t total_reqs = total_requests.load();
        if (total_reqs == 0) {
          return 1.0;
        }
        return static_cast<double>(successful_requests.load()) / total_reqs;
    }
    
    /**
     * @brief Get current health status
     */
    HealthStatus getHealthStatus() const {
        return static_cast<HealthStatus>(health_status.load());
    }
    
    /**
     * @brief Set health status
     */
    void setHealthStatus(HealthStatus status) {
        health_status.store(static_cast<int>(status));
    }
    
    /**
     * @brief Reset all metrics
     */
    void reset() {
        total_requests.store(0);
        successful_requests.store(0);
        failed_requests.store(0);
        read_requests.store(0);
        write_requests.store(0);
        total_latency_us.store(0);
        min_latency_us.store(999999999);
        max_latency_us.store(0);
        memory_usage_bytes.store(0);
        disk_usage_bytes.store(0);
        network_bytes_sent.store(0);
        network_bytes_received.store(0);
        replication_lag_ms.store(0);
        replica_count.store(0);
        sync_replica_count.store(0);
        quorum_writes.store(0);
        quorum_reads.store(0);
        quorum_failures.store(0);
        partition_events.store(0);
        wal_syncs.store(0);
        checkpoints_created.store(0);
        recovery_operations.store(0);
        transactions_started.store(0);
        transactions_committed.store(0);
        transactions_aborted.store(0);
        transaction_conflicts.store(0);
        start_time = std::chrono::system_clock::now();
        last_update_time = std::chrono::system_clock::now();
    }
};

/**
 * @brief Operational metrics collector and exporter
 * 
 * Collects metrics from sharding components and exports them
 * in Prometheus format for monitoring and alerting.
 */
class OperationalMetrics {
public:
    OperationalMetrics();
    ~OperationalMetrics() = default;
    
    /**
     * @brief Register a shard for metrics collection
     * @param shard_id Unique shard identifier
     */
    void registerShard(const std::string& shard_id);
    
    /**
     * @brief Unregister a shard
     * @param shard_id Shard identifier
     */
    void unregisterShard(const std::string& shard_id);
    
    /**
     * @brief Get metrics for a specific shard
     * @param shard_id Shard identifier
     * @return Pointer to shard metrics (nullptr if not found)
     */
    ShardMetrics* getShardMetrics(const std::string& shard_id);
    
    /**
     * @brief Get all shard IDs
     */
    std::vector<std::string> getShardIds() const;
    
    /**
     * @brief Export metrics in Prometheus text format
     * @return Prometheus-formatted metrics string
     */
    std::string exportPrometheusMetrics() const;
    
    /**
     * @brief Export metrics as JSON
     * @return JSON-formatted metrics string
     */
    std::string exportJSONMetrics() const;
    
    /**
     * @brief Get aggregated metrics across all shards
     * @param out Output parameter to receive aggregated metrics
     */
    void getAggregatedMetrics(ShardMetrics& out) const;
    
    /**
     * @brief Get cluster-wide health status
     * Based on individual shard health
     */
    HealthStatus getClusterHealth() const;
    
    /**
     * @brief Record a cross-shard RPC call.
     *
     * Called by ShardRPCClient for every call attempt so that latency and
     * outcome distributions are visible in dashboards.
     *
     * @param shard_id  Target shard identifier (metric label).
     * @param method    RPC method name: "prepare", "commit", "abort", etc.
     * @param outcome   "success", "retryable_error", or "non_retryable_error".
     * @param latency_us Round-trip latency of this single attempt in microseconds.
     */
    void recordRpcCall(
        const std::string& shard_id,
        const std::string& method,
        const std::string& outcome,
        uint64_t latency_us
    );

    /**
     * @brief Record a request
     * @param shard_id Shard identifier
     * @param latency_us Latency in microseconds
     * @param success Whether request succeeded
     * @param is_write Whether it's a write operation
     */
    void recordRequest(
        const std::string& shard_id,
        uint64_t latency_us,
        bool success,
        bool is_write
    );
    
    /**
     * @brief Update resource usage
     * @param shard_id Shard identifier
     * @param memory_bytes Memory usage in bytes
     * @param disk_bytes Disk usage in bytes
     */
    void updateResourceUsage(
        const std::string& shard_id,
        uint64_t memory_bytes,
        uint64_t disk_bytes
    );
    
    /**
     * @brief Record network traffic
     * @param shard_id Shard identifier
     * @param bytes_sent Bytes sent
     * @param bytes_received Bytes received
     */
    void recordNetworkTraffic(
        const std::string& shard_id,
        uint64_t bytes_sent,
        uint64_t bytes_received
    );
    
    /**
     * @brief Update replication metrics
     * @param shard_id Shard identifier
     * @param lag_ms Replication lag in milliseconds
     * @param replica_count Total replica count
     * @param sync_replica_count Synchronous replica count
     */
    void updateReplicationMetrics(
        const std::string& shard_id,
        uint64_t lag_ms,
        uint64_t replica_count,
        uint64_t sync_replica_count
    );
    
    /**
     * @brief Record quorum operation
     * @param shard_id Shard identifier
     * @param is_write Whether it's a write operation
     * @param success Whether quorum was achieved
     */
    void recordQuorumOperation(
        const std::string& shard_id,
        bool is_write,
        bool success
    );
    
    /**
     * @brief Record durability operation
     * @param shard_id Shard identifier
     * @param wal_sync Whether WAL was synced
     * @param checkpoint_created Whether checkpoint was created
     */
    void recordDurabilityOperation(
        const std::string& shard_id,
        bool wal_sync,
        bool checkpoint_created
    );
    
    /**
     * @brief Record transaction
     * @param shard_id Shard identifier
     * @param committed Whether transaction committed (vs aborted)
     * @param had_conflict Whether transaction had a conflict
     */
    void recordTransaction(
        const std::string& shard_id,
        bool committed,
        bool had_conflict
    );
    
    /**
     * @brief Record partition event
     * @param shard_id Shard identifier
     */
    void recordPartitionEvent(const std::string& shard_id);
    
    /**
     * @brief Update shard health status
     * @param shard_id Shard identifier
     * @param status New health status
     */
    void updateShardHealth(const std::string& shard_id, HealthStatus status);
    
    /**
     * @brief Convert health status to string
     * Utility method for converting HealthStatus enum to string representation
     */
    static std::string healthStatusToString(HealthStatus status);

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<ShardMetrics>> shard_metrics_;
    
    /**
     * @brief Format metric for Prometheus
     */
    static std::string formatPrometheusMetric(
        const std::string& name,
        MetricType type,
        double value,
        const std::map<std::string, std::string>& labels = {}
    );

    static std::string formatPrometheusMetric(
        const std::string& name,
        MetricType type,
        uint64_t value,
        const std::map<std::string, std::string>& labels = {}
    );

    static std::string formatPrometheusMetric(
        const std::string& name,
        MetricType type,
        int value,
        const std::map<std::string, std::string>& labels = {}
    );
};

}  // namespace sharding
}  // namespace themisdb

