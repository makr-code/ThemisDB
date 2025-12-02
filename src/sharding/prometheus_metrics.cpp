#include "sharding/prometheus_metrics.h"
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace themis {
namespace sharding {

PrometheusMetrics::PrometheusMetrics(const Config& config)
    : config_(config) {
}

void PrometheusMetrics::recordShardHealth(const std::string& shard_id, const std::string& status) {
    setGauge("themis_shard_health_status", 1.0, {{"shard_id", shard_id}, {"status", status}});
}

void PrometheusMetrics::recordCertificateExpiry(const std::string& shard_id, int64_t seconds_until_expiry) {
    setGauge("themis_shard_certificate_expiry_seconds", static_cast<double>(seconds_until_expiry), 
             {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRoutingRequest(const std::string& type) {
    incrementCounter("themis_routing_requests_total", {{"type", type}});
}

void PrometheusMetrics::recordRoutingError(const std::string& shard_id, const std::string& error_type) {
    incrementCounter("themis_routing_errors_total", {{"shard_id", shard_id}, {"error_type", error_type}});
}

void PrometheusMetrics::recordRoutingLatency(const std::string& operation, double latency_ms) {
    observeHistogram("themis_routing_latency_seconds", latency_ms / 1000.0, {{"operation", operation}});
}

void PrometheusMetrics::recordPKIConnection(const std::string& shard_id, const std::string& result) {
    incrementCounter("themis_pki_connections_total", {{"shard_id", shard_id}, {"result", result}});
}

void PrometheusMetrics::recordCertificateValidation(const std::string& result) {
    incrementCounter("themis_pki_certificate_validations_total", {{"result", result}});
}

void PrometheusMetrics::recordCRLCheck(const std::string& result) {
    incrementCounter("themis_pki_crl_checks_total", {{"result", result}});
}

void PrometheusMetrics::recordMigrationProgress(const std::string& operation_id, int64_t records, 
                                                 int64_t bytes, double percent) {
    setGauge("themis_migration_records_total", static_cast<double>(records), {{"operation_id", operation_id}});
    setGauge("themis_migration_bytes_total", static_cast<double>(bytes), {{"operation_id", operation_id}});
    setGauge("themis_migration_progress_percent", percent, {{"operation_id", operation_id}});
}

void PrometheusMetrics::recordMigrationDuration(const std::string& operation_id, double duration_seconds) {
    setGauge("themis_migration_duration_seconds", duration_seconds, {{"operation_id", operation_id}});
}

void PrometheusMetrics::recordQueryExecution(const std::string& query_type, double latency_ms) {
    observeHistogram("themis_query_execution_seconds", latency_ms / 1000.0, {{"query_type", query_type}});
}

void PrometheusMetrics::recordScatterGatherFanout(int num_shards) {
    observeHistogram("themis_scatter_gather_fanout", static_cast<double>(num_shards), {});
}

void PrometheusMetrics::recordResultMergeTime(double time_ms) {
    observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
}

void PrometheusMetrics::recordTopologyChange(const std::string& change_type) {
    incrementCounter("themis_topology_changes_total", {{"change_type", change_type}});
}

void PrometheusMetrics::recordClusterSize(int num_shards) {
    setGauge("themis_cluster_size", static_cast<double>(num_shards), {});
}

void PrometheusMetrics::recordVirtualNodes(int total_vnodes) {
    setGauge("themis_virtual_nodes_total", static_cast<double>(total_vnodes), {});
}

// ==================== Phase 6 New Metrics Implementation ====================

void PrometheusMetrics::recordGossipMessage(const std::string& message_type) {
    incrementCounter("themis_gossip_messages_total", {{"type", message_type}});
}

void PrometheusMetrics::recordGossipMessageSize(int64_t bytes) {
    observeHistogram("themis_gossip_message_size_bytes", static_cast<double>(bytes), {});
}

void PrometheusMetrics::recordGossipRoundTrip(double latency_ms) {
    observeHistogram("themis_gossip_roundtrip_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordGossipPeerCount(int count) {
    setGauge("themis_gossip_peer_count", static_cast<double>(count), {});
}

void PrometheusMetrics::recordGossipFailedPeer(const std::string& peer_id) {
    incrementCounter("themis_gossip_failed_peers_total", {{"peer_id", peer_id}});
}

void PrometheusMetrics::recordGossipVersionVector(const std::string& peer_id, uint64_t version) {
    setGauge("themis_gossip_version_vector", static_cast<double>(version), {{"peer_id", peer_id}});
}

void PrometheusMetrics::recordCrossShardJoin(const std::string& strategy) {
    incrementCounter("themis_cross_shard_joins_total", {{"strategy", strategy}});
}

void PrometheusMetrics::recordCrossShardJoinDuration(const std::string& strategy, double duration_ms) {
    observeHistogram("themis_cross_shard_join_duration_seconds", duration_ms / 1000.0, {{"strategy", strategy}});
}

void PrometheusMetrics::recordCrossShardJoinRows(const std::string& strategy, int64_t left_rows, 
                                                   int64_t right_rows, int64_t result_rows) {
    setGauge("themis_cross_shard_join_left_rows", static_cast<double>(left_rows), {{"strategy", strategy}});
    setGauge("themis_cross_shard_join_right_rows", static_cast<double>(right_rows), {{"strategy", strategy}});
    setGauge("themis_cross_shard_join_result_rows", static_cast<double>(result_rows), {{"strategy", strategy}});
}

void PrometheusMetrics::recordHashTableBuildTime(double time_ms) {
    observeHistogram("themis_hash_table_build_seconds", time_ms / 1000.0, {});
}

void PrometheusMetrics::recordProbePhaseTime(double time_ms) {
    observeHistogram("themis_probe_phase_seconds", time_ms / 1000.0, {});
}

void PrometheusMetrics::recordContentProcessorInvocation(const std::string& processor_type) {
    incrementCounter("themis_content_processor_invocations_total", {{"type", processor_type}});
}

void PrometheusMetrics::recordContentProcessorDuration(const std::string& processor_type, double duration_ms) {
    observeHistogram("themis_content_processor_duration_seconds", duration_ms / 1000.0, {{"type", processor_type}});
}

void PrometheusMetrics::recordContentProcessorError(const std::string& processor_type, const std::string& error_type) {
    incrementCounter("themis_content_processor_errors_total", {{"type", processor_type}, {"error", error_type}});
}

void PrometheusMetrics::recordContentProcessorBytes(const std::string& processor_type, int64_t input_bytes, 
                                                      int64_t output_bytes) {
    incrementCounter("themis_content_processor_input_bytes_total", {{"type", processor_type}});
    setGauge("themis_content_processor_last_input_bytes", static_cast<double>(input_bytes), {{"type", processor_type}});
    setGauge("themis_content_processor_last_output_bytes", static_cast<double>(output_bytes), {{"type", processor_type}});
}

void PrometheusMetrics::recordMetadataStoreOperation(const std::string& operation) {
    incrementCounter("themis_metadata_store_operations_total", {{"operation", operation}});
}

void PrometheusMetrics::recordMetadataStoreLatency(const std::string& operation, double latency_ms) {
    observeHistogram("themis_metadata_store_latency_seconds", latency_ms / 1000.0, {{"operation", operation}});
}

void PrometheusMetrics::recordMetadataStoreError(const std::string& operation, const std::string& error_type) {
    incrementCounter("themis_metadata_store_errors_total", {{"operation", operation}, {"error", error_type}});
}

void PrometheusMetrics::recordHealthCheckExecution(const std::string& check_type) {
    incrementCounter("themis_health_check_executions_total", {{"type", check_type}});
}

void PrometheusMetrics::recordHealthCheckDuration(const std::string& check_type, double duration_ms) {
    observeHistogram("themis_health_check_duration_seconds", duration_ms / 1000.0, {{"type", check_type}});
}

void PrometheusMetrics::recordHealthCheckResult(const std::string& check_type, const std::string& result) {
    incrementCounter("themis_health_check_results_total", {{"type", check_type}, {"result", result}});
}

void PrometheusMetrics::recordCloudAgentOperation(const std::string& operation) {
    incrementCounter("themis_cloud_agent_operations_total", {{"operation", operation}});
}

void PrometheusMetrics::recordDatacenterLatency(const std::string& datacenter, double latency_ms) {
    observeHistogram("themis_datacenter_latency_seconds", latency_ms / 1000.0, {{"datacenter", datacenter}});
}

void PrometheusMetrics::recordCrossDCRequest(const std::string& source_dc, const std::string& target_dc) {
    incrementCounter("themis_cross_dc_requests_total", {{"source", source_dc}, {"target", target_dc}});
}

std::string PrometheusMetrics::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    // Export counters
    for (const auto& [key, value] : counters_) {
        oss << key << " " << value.load() << "\n";
    }

    // Export gauges
    for (const auto& [key, value] : gauges_) {
        oss << key << " " << value.load() << "\n";
    }

    // Export histograms (simplified - just quantiles)
    for (const auto& [key, values] : histograms_) {
        if (values.empty()) continue;
        
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());
        
        // Calculate quantiles
        auto p50 = sorted[sorted.size() * 50 / 100];
        auto p95 = sorted[sorted.size() * 95 / 100];
        auto p99 = sorted[sorted.size() * 99 / 100];
        
        oss << key << "{quantile=\"0.5\"} " << p50 << "\n";
        oss << key << "{quantile=\"0.95\"} " << p95 << "\n";
        oss << key << "{quantile=\"0.99\"} " << p99 << "\n";
    }

    return oss.str();
}

std::string PrometheusMetrics::getMetricsWithAnnotations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    // Add metric annotations (HELP and TYPE)
    oss << "# HELP themis_routing_requests_total Total number of routing requests\n";
    oss << "# TYPE themis_routing_requests_total counter\n";
    oss << "# HELP themis_routing_latency_seconds Routing operation latency\n";
    oss << "# TYPE themis_routing_latency_seconds histogram\n";
    oss << "# HELP themis_gossip_messages_total Total gossip messages sent/received\n";
    oss << "# TYPE themis_gossip_messages_total counter\n";
    oss << "# HELP themis_gossip_peer_count Current number of known peers\n";
    oss << "# TYPE themis_gossip_peer_count gauge\n";
    oss << "# HELP themis_cross_shard_joins_total Total cross-shard join operations\n";
    oss << "# TYPE themis_cross_shard_joins_total counter\n";
    oss << "# HELP themis_cross_shard_join_duration_seconds Cross-shard join duration\n";
    oss << "# TYPE themis_cross_shard_join_duration_seconds histogram\n";
    oss << "# HELP themis_content_processor_invocations_total Total content processor invocations\n";
    oss << "# TYPE themis_content_processor_invocations_total counter\n";
    oss << "# HELP themis_content_processor_duration_seconds Content processor duration\n";
    oss << "# TYPE themis_content_processor_duration_seconds histogram\n";
    oss << "# HELP themis_cluster_size Current number of shards in the cluster\n";
    oss << "# TYPE themis_cluster_size gauge\n";
    oss << "# HELP themis_datacenter_latency_seconds Latency to datacenter\n";
    oss << "# TYPE themis_datacenter_latency_seconds histogram\n";
    oss << "\n";

    // Export counters
    for (const auto& [key, value] : counters_) {
        oss << key << " " << value.load() << "\n";
    }

    // Export gauges
    for (const auto& [key, value] : gauges_) {
        oss << key << " " << value.load() << "\n";
    }

    // Export histograms
    for (const auto& [key, values] : histograms_) {
        if (values.empty()) continue;
        
        auto sorted = values;
        std::sort(sorted.begin(), sorted.end());
        
        auto p50 = sorted[sorted.size() * 50 / 100];
        auto p95 = sorted[sorted.size() * 95 / 100];
        auto p99 = sorted[sorted.size() * 99 / 100];
        double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
        
        oss << key << "{quantile=\"0.5\"} " << p50 << "\n";
        oss << key << "{quantile=\"0.95\"} " << p95 << "\n";
        oss << key << "{quantile=\"0.99\"} " << p99 << "\n";
        oss << key << "_sum " << sum << "\n";
        oss << key << "_count " << sorted.size() << "\n";
    }

    return oss.str();
}

void PrometheusMetrics::incrementCounter(const std::string& name, 
                                          const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    counters_[key]++;
}

void PrometheusMetrics::setGauge(const std::string& name, double value, 
                                  const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    gauges_[key].store(value);
}

void PrometheusMetrics::observeHistogram(const std::string& name, double value, 
                                          const std::map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = getCounterKey(name, labels);
    histograms_[key].push_back(value);
    
    // Keep only recent values (max 1000)
    if (histograms_[key].size() > 1000) {
        histograms_[key].erase(histograms_[key].begin());
    }
}

std::string PrometheusMetrics::formatLabels(const std::map<std::string, std::string>& labels) const {
    if (labels.empty()) return "";
    
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string PrometheusMetrics::getCounterKey(const std::string& name, 
                                               const std::map<std::string, std::string>& labels) const {
    return name + formatLabels(labels);
}

} // namespace sharding
} // namespace themis
