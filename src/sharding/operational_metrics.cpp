/**
 * @file operational_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/operational_metrics.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

OperationalMetrics::OperationalMetrics() {
}

void OperationalMetrics::registerShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shard_metrics_.find(shard_id) == shard_metrics_.end()) {
        auto metrics = std::make_unique<ShardMetrics>();
        metrics->start_time = std::chrono::system_clock::now();
        metrics->last_update_time = metrics->start_time;
        shard_metrics_[shard_id] = std::move(metrics);
    }
}

void OperationalMetrics::unregisterShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_metrics_.erase(shard_id);
}

ShardMetrics* OperationalMetrics::getShardMetrics(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_metrics_.find(shard_id);
    if (it != shard_metrics_.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

std::vector<std::string> OperationalMetrics::getShardIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(shard_metrics_.size());
    
    for (const auto& [id, _] : shard_metrics_) {
        ids.push_back(id);
    }
    
    return ids;
}

std::string OperationalMetrics::exportPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    
    // Header comments
    ss << "# HELP themisdb_shard_requests_total Total number of requests\n";
    ss << "# TYPE themisdb_shard_requests_total counter\n";
    
    for (const auto& [shard_id, metrics] : shard_metrics_) {
        std::map<std::string, std::string> labels = {{"shard_id", shard_id}};
        
        // Request metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_requests_total",
            MetricType::COUNTER,
            metrics->total_requests.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_requests_successful",
            MetricType::COUNTER,
            metrics->successful_requests.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_requests_failed",
            MetricType::COUNTER,
            metrics->failed_requests.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_read_requests",
            MetricType::COUNTER,
            metrics->read_requests.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_write_requests",
            MetricType::COUNTER,
            metrics->write_requests.load(),
            labels
        );
        
        // Latency metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_latency_avg_us",
            MetricType::GAUGE,
            metrics->getAverageLatencyUs(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_latency_min_us",
            MetricType::GAUGE,
            metrics->min_latency_us.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_latency_max_us",
            MetricType::GAUGE,
            metrics->max_latency_us.load(),
            labels
        );
        
        // Resource metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_memory_bytes",
            MetricType::GAUGE,
            metrics->memory_usage_bytes.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_disk_bytes",
            MetricType::GAUGE,
            metrics->disk_usage_bytes.load(),
            labels
        );
        
        // Network metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_network_sent_bytes",
            MetricType::COUNTER,
            metrics->network_bytes_sent.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_network_received_bytes",
            MetricType::COUNTER,
            metrics->network_bytes_received.load(),
            labels
        );
        
        // Replication metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_replication_lag_ms",
            MetricType::GAUGE,
            metrics->replication_lag_ms.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_replica_count",
            MetricType::GAUGE,
            metrics->replica_count.load(),
            labels
        );
        
        // Consistency metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_quorum_writes",
            MetricType::COUNTER,
            metrics->quorum_writes.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_quorum_failures",
            MetricType::COUNTER,
            metrics->quorum_failures.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_partition_events",
            MetricType::COUNTER,
            metrics->partition_events.load(),
            labels
        );
        
        // Durability metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_wal_syncs",
            MetricType::COUNTER,
            metrics->wal_syncs.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_checkpoints",
            MetricType::COUNTER,
            metrics->checkpoints_created.load(),
            labels
        );
        
        // Transaction metrics
        ss << formatPrometheusMetric(
            "themisdb_shard_transactions_committed",
            MetricType::COUNTER,
            metrics->transactions_committed.load(),
            labels
        );
        
        ss << formatPrometheusMetric(
            "themisdb_shard_transactions_aborted",
            MetricType::COUNTER,
            metrics->transactions_aborted.load(),
            labels
        );
        
        // Health status
        ss << formatPrometheusMetric(
            "themisdb_shard_health_status",
            MetricType::GAUGE,
            metrics->health_status.load(),
            labels
        );
        
        // Success rate
        ss << formatPrometheusMetric(
            "themisdb_shard_success_rate",
            MetricType::GAUGE,
            metrics->getSuccessRate(),
            labels
        );
    }
    
    return ss.str();
}

std::string OperationalMetrics::exportJSONMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json result = nlohmann::json::object();
    
    for (const auto& [shard_id, metrics] : shard_metrics_) {
        nlohmann::json shard_json = {
            {"shard_id", shard_id},
            {"requests", {
                {"total", metrics->total_requests.load()},
                {"successful", metrics->successful_requests.load()},
                {"failed", metrics->failed_requests.load()},
                {"read", metrics->read_requests.load()},
                {"write", metrics->write_requests.load()}
            }},
            {"latency_us", {
                {"average", metrics->getAverageLatencyUs()},
                {"min", metrics->min_latency_us.load()},
                {"max", metrics->max_latency_us.load()}
            }},
            {"resources", {
                {"memory_bytes", metrics->memory_usage_bytes.load()},
                {"disk_bytes", metrics->disk_usage_bytes.load()}
            }},
            {"network", {
                {"bytes_sent", metrics->network_bytes_sent.load()},
                {"bytes_received", metrics->network_bytes_received.load()}
            }},
            {"replication", {
                {"lag_ms", metrics->replication_lag_ms.load()},
                {"replica_count", metrics->replica_count.load()},
                {"sync_replica_count", metrics->sync_replica_count.load()}
            }},
            {"consistency", {
                {"quorum_writes", metrics->quorum_writes.load()},
                {"quorum_reads", metrics->quorum_reads.load()},
                {"quorum_failures", metrics->quorum_failures.load()},
                {"partition_events", metrics->partition_events.load()}
            }},
            {"durability", {
                {"wal_syncs", metrics->wal_syncs.load()},
                {"checkpoints", metrics->checkpoints_created.load()},
                {"recoveries", metrics->recovery_operations.load()}
            }},
            {"transactions", {
                {"started", metrics->transactions_started.load()},
                {"committed", metrics->transactions_committed.load()},
                {"aborted", metrics->transactions_aborted.load()},
                {"conflicts", metrics->transaction_conflicts.load()}
            }},
            {"health", {
                {"status", healthStatusToString(metrics->getHealthStatus())},
                {"success_rate", metrics->getSuccessRate()}
            }}
        };
        
        result[shard_id] = shard_json;
    }
    
    return result.dump(2);
}

void OperationalMetrics::getAggregatedMetrics(ShardMetrics& aggregated) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    aggregated.reset();
    
    for (const auto& [_, metrics] : shard_metrics_) {
        aggregated.total_requests += metrics->total_requests.load();
        aggregated.successful_requests += metrics->successful_requests.load();
        aggregated.failed_requests += metrics->failed_requests.load();
        aggregated.read_requests += metrics->read_requests.load();
        aggregated.write_requests += metrics->write_requests.load();
        aggregated.total_latency_us += metrics->total_latency_us.load();
        
        // Min/max latency
        uint64_t min_lat = metrics->min_latency_us.load();
        uint64_t max_lat = metrics->max_latency_us.load();
        
        if (min_lat < aggregated.min_latency_us.load()) {
            aggregated.min_latency_us.store(min_lat);
        }
        if (max_lat > aggregated.max_latency_us.load()) {
            aggregated.max_latency_us.store(max_lat);
        }
        
        aggregated.memory_usage_bytes += metrics->memory_usage_bytes.load();
        aggregated.disk_usage_bytes += metrics->disk_usage_bytes.load();
        aggregated.network_bytes_sent += metrics->network_bytes_sent.load();
        aggregated.network_bytes_received += metrics->network_bytes_received.load();
        aggregated.replica_count += metrics->replica_count.load();
        aggregated.quorum_writes += metrics->quorum_writes.load();
        aggregated.quorum_reads += metrics->quorum_reads.load();
        aggregated.quorum_failures += metrics->quorum_failures.load();
        aggregated.partition_events += metrics->partition_events.load();
        aggregated.wal_syncs += metrics->wal_syncs.load();
        aggregated.checkpoints_created += metrics->checkpoints_created.load();
        aggregated.transactions_committed += metrics->transactions_committed.load();
        aggregated.transactions_aborted += metrics->transactions_aborted.load();
    }
}

HealthStatus OperationalMetrics::getClusterHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shard_metrics_.empty()) {
        return HealthStatus::HEALTHY;
    }
    
    int down_count = 0;
    int unhealthy_count = 0;
    int degraded_count = 0;
    
    for (const auto& [_, metrics] : shard_metrics_) {
        HealthStatus status = metrics->getHealthStatus();
        
        switch (status) {
            case HealthStatus::DOWN:
                down_count++;
                break;
            case HealthStatus::UNHEALTHY:
                unhealthy_count++;
                break;
            case HealthStatus::DEGRADED:
                degraded_count++;
                break;
            default:
                break;
        }
    }
    
    // Determine cluster health based on shard health
    if (down_count > 0) {
        return HealthStatus::DOWN;
    } else if (unhealthy_count > 0) {
        return HealthStatus::UNHEALTHY;
    } else if (degraded_count > 0) {
        return HealthStatus::DEGRADED;
    }
    
    return HealthStatus::HEALTHY;
}

void OperationalMetrics::recordRpcCall(
    const std::string& shard_id,
    [[maybe_unused]] const std::string& method,
    const std::string& outcome,
    uint64_t latency_us
) {
    // Map outcome string to success flag for the generic recordRequest path.
    const bool success = (outcome == "success");
    // RPC calls are always cross-shard writes from the perspective of the
    // generic request counter (they carry mutation intent).
    recordRequest(shard_id, latency_us, success, /*is_write=*/true);
    // label carried by PrometheusMetrics; suppresses unused-var warning
}

void OperationalMetrics::recordRequest(
    const std::string& shard_id,
    uint64_t latency_us,
    bool success,
    bool is_write
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->total_requests.fetch_add(1, std::memory_order_relaxed);
    
    if (success) {
        metrics->successful_requests.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics->failed_requests.fetch_add(1, std::memory_order_relaxed);
    }
    
    if (is_write) {
        metrics->write_requests.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics->read_requests.fetch_add(1, std::memory_order_relaxed);
    }
    
    metrics->total_latency_us.fetch_add(latency_us, std::memory_order_relaxed);
    
    // Update min/max latency
    uint64_t current_min = metrics->min_latency_us.load();
    while (latency_us < current_min) {
        if (metrics->min_latency_us.compare_exchange_weak(current_min, latency_us)) {
            break;
        }
    }
    
    uint64_t current_max = metrics->max_latency_us.load();
    while (latency_us > current_max) {
        if (metrics->max_latency_us.compare_exchange_weak(current_max, latency_us)) {
            break;
        }
    }
    
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::updateResourceUsage(
    const std::string& shard_id,
    uint64_t memory_bytes,
    uint64_t disk_bytes
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->memory_usage_bytes.store(memory_bytes, std::memory_order_relaxed);
    metrics->disk_usage_bytes.store(disk_bytes, std::memory_order_relaxed);
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::recordNetworkTraffic(
    const std::string& shard_id,
    uint64_t bytes_sent,
    uint64_t bytes_received
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->network_bytes_sent.fetch_add(bytes_sent, std::memory_order_relaxed);
    metrics->network_bytes_received.fetch_add(bytes_received, std::memory_order_relaxed);
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::updateReplicationMetrics(
    const std::string& shard_id,
    uint64_t lag_ms,
    uint64_t replica_count,
    uint64_t sync_replica_count
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->replication_lag_ms.store(lag_ms, std::memory_order_relaxed);
    metrics->replica_count.store(replica_count, std::memory_order_relaxed);
    metrics->sync_replica_count.store(sync_replica_count, std::memory_order_relaxed);
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::recordQuorumOperation(
    const std::string& shard_id,
    bool is_write,
    bool success
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    if (is_write) {
        metrics->quorum_writes.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics->quorum_reads.fetch_add(1, std::memory_order_relaxed);
    }
    
    if (!success) {
        metrics->quorum_failures.fetch_add(1, std::memory_order_relaxed);
    }
    
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::recordDurabilityOperation(
    const std::string& shard_id,
    bool wal_sync,
    bool checkpoint_created
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    if (wal_sync) {
        metrics->wal_syncs.fetch_add(1, std::memory_order_relaxed);
    }
    
    if (checkpoint_created) {
        metrics->checkpoints_created.fetch_add(1, std::memory_order_relaxed);
    }
    
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::recordTransaction(
    const std::string& shard_id,
    bool committed,
    bool had_conflict
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->transactions_started.fetch_add(1, std::memory_order_relaxed);
    
    if (committed) {
        metrics->transactions_committed.fetch_add(1, std::memory_order_relaxed);
    } else {
        metrics->transactions_aborted.fetch_add(1, std::memory_order_relaxed);
    }
    
    if (had_conflict) {
        metrics->transaction_conflicts.fetch_add(1, std::memory_order_relaxed);
    }
    
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::recordPartitionEvent(const std::string& shard_id) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->partition_events.fetch_add(1, std::memory_order_relaxed);
    metrics->last_update_time = std::chrono::system_clock::now();
}

void OperationalMetrics::updateShardHealth(
    const std::string& shard_id,
    HealthStatus status
) {
    auto* metrics = getShardMetrics(shard_id);
    if (!metrics) {
      return;
    }
    
    metrics->setHealthStatus(status);
    metrics->last_update_time = std::chrono::system_clock::now();
}

std::string OperationalMetrics::healthStatusToString(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY: return "healthy";
        case HealthStatus::DEGRADED: return "degraded";
        case HealthStatus::UNHEALTHY: return "unhealthy";
        case HealthStatus::DOWN: return "down";
        default: return "unknown";
    }
}

std::string OperationalMetrics::formatPrometheusMetric(
    const std::string& name,
    MetricType type,
    double value,
    const std::map<std::string, std::string>& labels
) {
    (void)type;
    std::stringstream ss;
    
    ss << name;
    
    if (!labels.empty()) {
        ss << "{";
        bool first = true;
        for (const auto& [key, val] : labels) {
            if (!first) {
              ss << ",";
            }
            ss << key << "=\"" << val << "\"";
            first = false;
        }
        ss << "}";
    }
    
    ss << " " << std::fixed << std::setprecision(2) << value << "\n";
    
    return ss.str();
}

std::string OperationalMetrics::formatPrometheusMetric(
    const std::string& name,
    MetricType type,
    uint64_t value,
    const std::map<std::string, std::string>& labels
) {
    return formatPrometheusMetric(name, type, static_cast<double>(value), labels);
}

std::string OperationalMetrics::formatPrometheusMetric(
    const std::string& name,
    MetricType type,
    int value,
    const std::map<std::string, std::string>& labels
) {
    return formatPrometheusMetric(name, type, static_cast<double>(value), labels);
}

}  // namespace sharding
}  // namespace themisdb
