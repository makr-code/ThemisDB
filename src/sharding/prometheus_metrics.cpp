/**
 * @file prometheus_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=3, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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

void PrometheusMetrics::recordRpcCall(
    const std::string& shard_id,
    const std::string& method,
    const std::string& outcome,
    double latency_ms
) {
    incrementCounter("themis_cross_shard_rpc_calls_total",
                     {{"shard_id", shard_id}, {"method", method}, {"outcome", outcome}});
    observeHistogram("themis_cross_shard_rpc_latency_seconds",
                     latency_ms / 1000.0,
                     {{"shard_id", shard_id}, {"method", method}});
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

void PrometheusMetrics::recordScatterGatherFanout([[maybe_unused]] int num_shards) {
    observeHistogram("themis_scatter_gather_fanout", static_cast<double>(num_shards), {});
}

void PrometheusMetrics::recordResultMergeTime([[maybe_unused]] double time_ms) {
    observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
}

void PrometheusMetrics::recordTopologyChange(const std::string& change_type) {
    incrementCounter("themis_topology_changes_total", {{"change_type", change_type}});
}

void PrometheusMetrics::recordClusterSize([[maybe_unused]] int num_shards) {
    setGauge("themis_cluster_size", static_cast<double>(num_shards), {});
}

void PrometheusMetrics::recordVirtualNodes([[maybe_unused]] int total_vnodes) {
    setGauge("themis_virtual_nodes_total", static_cast<double>(total_vnodes), {});
}

// ==================== Phase 6 New Metrics Implementation ====================

void PrometheusMetrics::recordGossipMessage(const std::string& message_type) {
    incrementCounter("themis_gossip_messages_total", {{"type", message_type}});
}

void PrometheusMetrics::recordGossipMessageSize(int64_t bytes) {
    observeHistogram("themis_gossip_message_size_bytes", static_cast<double>(bytes), {});
}

void PrometheusMetrics::recordGossipRoundTrip([[maybe_unused]] double latency_ms) {
    observeHistogram("themis_gossip_roundtrip_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordGossipPeerCount([[maybe_unused]] int count) {
    setGauge("themis_gossip_peer_count", static_cast<double>(count), {});
}

void PrometheusMetrics::recordGossipFailedPeer(const std::string& peer_id) {
    incrementCounter("themis_gossip_failed_peers_total", {{"peer_id", peer_id}});
}

void PrometheusMetrics::recordGossipVersionVector(const std::string& peer_id, uint64_t version) {
    setGauge("themis_gossip_version_vector", static_cast<double>(version), {{"peer_id", peer_id}});
}

// ==================== Gossip Config Manager Metrics Implementation ====================

void PrometheusMetrics::recordGossipConfigUpdate(const std::string& operation) {
    incrementCounter("themis_gossip_config_updates_total", {{"operation", operation}});
}

void PrometheusMetrics::recordGossipConfigUpdateLatency([[maybe_unused]] double latency_ms) {
    observeHistogram("themis_gossip_config_update_latency_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordGossipConfigConflict(const std::string& resolution_type) {
    incrementCounter("themis_gossip_config_conflicts_total", {{"resolution", resolution_type}});
}

void PrometheusMetrics::recordGossipResourceSnapshot(const std::string& operation) {
    incrementCounter("themis_gossip_resource_snapshots_total", {{"operation", operation}});
}

void PrometheusMetrics::recordGossipResourceSnapshotLatency([[maybe_unused]] double latency_ms) {
    observeHistogram("themis_gossip_resource_snapshot_latency_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordGossipConfigRound() {
    incrementCounter("themis_gossip_config_rounds_total", {});
}

void PrometheusMetrics::recordGossipConfigAntiEntropy() {
    incrementCounter("themis_gossip_config_anti_entropy_syncs_total", {});
}

void PrometheusMetrics::setGossipConfigPeerCount([[maybe_unused]] int count) {
    setGauge("themis_gossip_config_peer_count", static_cast<double>(count), {});
}

void PrometheusMetrics::observeGossipPropagationLatency([[maybe_unused]] double latency_ms) {
    observeHistogram("themis_gossip_propagation_latency_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordGossipMessagesSent() {
    incrementCounter("themis_gossip_config_messages_sent_total", {});
}

void PrometheusMetrics::recordGossipMessagesReceived() {
    incrementCounter("themis_gossip_config_messages_received_total", {});
}

// ====================================================================================

void PrometheusMetrics::recordCrossShardRequest(
    const std::string& shard_id,
    const std::string& operation,
    const std::string& outcome)
{
    incrementCounter("sharding_cross_shard_requests_total",
                     {{"shard_id", shard_id},
                      {"operation", operation},
                      {"outcome",   outcome}});
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

void PrometheusMetrics::recordHashTableBuildTime([[maybe_unused]] double time_ms) {
    observeHistogram("themis_hash_table_build_seconds", time_ms / 1000.0, {});
}

void PrometheusMetrics::recordProbePhaseTime([[maybe_unused]] double time_ms) {
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

// ==================== Replication Metrics Implementation ====================

void PrometheusMetrics::recordWalShipBatch(const std::string& replica_id, int64_t entries, 
                                            int64_t bytes, bool success) {
    std::string result = success ? "success" : "failure";
    incrementCounter("themis_wal_ship_batches_total", {{"replica_id", replica_id}, {"result", result}});
    
    if (success) {
        // Track cumulative shipped entries and bytes
        auto entries_key = getCounterKey("themis_wal_ship_entries_total", {{"replica_id", replica_id}});
        auto bytes_key = getCounterKey("themis_wal_ship_bytes_total", {{"replica_id", replica_id}});
        
        counters_[entries_key].fetch_add(entries, std::memory_order_relaxed);
        counters_[bytes_key].fetch_add(bytes, std::memory_order_relaxed);
    } else {
        incrementCounter("themis_wal_ship_failures_total", {{"replica_id", replica_id}});
    }
}

void PrometheusMetrics::recordWalShipLatency(const std::string& replica_id, double latency_ms) {
    observeHistogram("themis_wal_ship_latency_seconds", latency_ms / 1000.0, {{"replica_id", replica_id}});
}

void PrometheusMetrics::recordWalReplicationLag(const std::string& replica_id, double lag_seconds) {
    setGauge("themis_wal_replication_lag_seconds", lag_seconds, {{"replica_id", replica_id}});
}

void PrometheusMetrics::setWalBacklogBytes(const std::string& replica_id, int64_t bytes) {
    setGauge("themis_wal_backlog_bytes", static_cast<double>(bytes), {{"replica_id", replica_id}});
}

void PrometheusMetrics::recordWalCompressionRatio([[maybe_unused]] double ratio) {
    observeHistogram("themis_wal_compression_ratio", ratio, {});
}

void PrometheusMetrics::recordWalApplyBatch(int64_t entries, bool success) {
    std::string result = success ? "success" : "failure";
    incrementCounter("themis_wal_apply_batches_total", {{"result", result}});
    
    if (success) {
        counters_[getCounterKey("themis_wal_apply_entries_total", {})].fetch_add(entries, std::memory_order_relaxed);
    } else {
        incrementCounter("themis_wal_apply_failures_total", {});
    }
}

void PrometheusMetrics::recordWalApplyLatency([[maybe_unused]] double latency_ms) {
    observeHistogram("themis_wal_apply_latency_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordWalApplyFailure(const std::string& error_type) {
    incrementCounter("themis_wal_apply_errors_total", {{"error_type", error_type}});
}

void PrometheusMetrics::setWalLastAppliedLsn(const std::string& lsn) {
    setGauge("themis_wal_last_applied_lsn", 1.0, {{"lsn", lsn}});
}

void PrometheusMetrics::recordWriteConcernWait(const std::string& level, double wait_time_ms, bool success) {
    std::string result = success ? "success" : "timeout";
    incrementCounter("themis_write_concern_waits_total", {{"level", level}, {"result", result}});
    observeHistogram("themis_write_concern_wait_seconds", wait_time_ms / 1000.0, {{"level", level}});
}

void PrometheusMetrics::setPendingWrites(int64_t count) {
    setGauge("themis_replication_pending_writes", static_cast<double>(count), {});
}

void PrometheusMetrics::recordQuorumTimeout(const std::string& level) {
    incrementCounter("themis_replication_quorum_timeouts_total", {{"level", level}});
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
        if (values.empty()) {
          continue;
        }
        
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
        if (values.empty()) {
          continue;
        }
        
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

// ==================== Raft Consensus Metrics Implementation ====================

void PrometheusMetrics::setRaftRole(const std::string& shard_id, const std::string& role) {
    setGauge("themis_raft_role", role == "LEADER" ? 2.0 : (role == "CANDIDATE" ? 1.0 : 0.0),
             {{"shard_id", shard_id}, {"role", role}});
}

void PrometheusMetrics::setRaftTerm(const std::string& shard_id, uint64_t term) {
    setGauge("themis_raft_term", static_cast<double>(term), {{"shard_id", shard_id}});
}

void PrometheusMetrics::setRaftCommitIndex(const std::string& shard_id, uint64_t commit_index) {
    setGauge("themis_raft_commit_index", static_cast<double>(commit_index), {{"shard_id", shard_id}});
}

void PrometheusMetrics::setRaftLastApplied(const std::string& shard_id, uint64_t last_applied) {
    setGauge("themis_raft_last_applied", static_cast<double>(last_applied), {{"shard_id", shard_id}});
}

void PrometheusMetrics::setRaftLogSize(const std::string& shard_id, uint64_t log_size) {
    setGauge("themis_raft_log_size", static_cast<double>(log_size), {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftLeaderElection(const std::string& shard_id, double duration_ms) {
    incrementCounter("themis_raft_leader_elections_total", {{"shard_id", shard_id}});
    observeHistogram("themis_raft_leader_election_duration_seconds", duration_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftLeaderChange(const std::string& shard_id, 
                                               const std::string& old_leader, 
                                               const std::string& new_leader) {
    incrementCounter("themis_raft_leader_changes_total", 
                    {{"shard_id", shard_id}, {"old_leader", old_leader}, {"new_leader", new_leader}});
}

void PrometheusMetrics::recordRaftHeartbeat(const std::string& shard_id, bool success) {
    incrementCounter("themis_raft_heartbeats_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordRaftHeartbeatLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_raft_heartbeat_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftLogAppend(const std::string& shard_id, uint64_t entries_count, bool success) {
    incrementCounter("themis_raft_log_appends_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
    if (success) {
        observeHistogram("themis_raft_log_append_entries", static_cast<double>(entries_count), {{"shard_id", shard_id}});
    }
}

void PrometheusMetrics::recordRaftLogAppendLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_raft_log_append_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftReplicationLag(const std::string& shard_id, 
                                                 const std::string& follower_id, 
                                                 uint64_t lag_entries) {
    setGauge("themis_raft_replication_lag_entries", static_cast<double>(lag_entries), 
            {{"shard_id", shard_id}, {"follower_id", follower_id}});
}

void PrometheusMetrics::setRaftQuorumStatus(const std::string& shard_id, bool has_quorum) {
    setGauge("themis_raft_has_quorum", has_quorum ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftPartitionDetected(const std::string& shard_id) {
    incrementCounter("themis_raft_partitions_detected_total", {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordRaftPartitionHealed(const std::string& shard_id) {
    incrementCounter("themis_raft_partitions_healed_total", {{"shard_id", shard_id}});
}

void PrometheusMetrics::setRaftReadOnlyMode(const std::string& shard_id, bool is_read_only) {
    setGauge("themis_raft_read_only_mode", is_read_only ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

// ==================== Paxos Consensus Metrics Implementation (Phase 1) ====================

void PrometheusMetrics::setPaxosRole(const std::string& shard_id, const std::string& role) {
    double role_value = 0.0;
    if (role == "LEADER") {
      role_value = 3.0;
    }
    else if (role == "PROPOSER") role_value = 2.0;
    else if (role == "ACCEPTOR") role_value = 1.0;
    else if (role == "LEARNER") role_value = 0.5;
    
    setGauge("themis_paxos_role", role_value, {{"shard_id", shard_id}, {"role", role}});
}

void PrometheusMetrics::setPaxosRound(const std::string& shard_id, uint64_t round) {
    setGauge("themis_paxos_round", static_cast<double>(round), {{"shard_id", shard_id}});
}

void PrometheusMetrics::setPaxosHighestProposal(const std::string& shard_id, uint64_t proposal_number) {
    setGauge("themis_paxos_highest_proposal", static_cast<double>(proposal_number), {{"shard_id", shard_id}});
}

void PrometheusMetrics::setPaxosCommittedSlot(const std::string& shard_id, uint64_t slot) {
    setGauge("themis_paxos_committed_slot", static_cast<double>(slot), {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosPrepare(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_prepare_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordPaxosPrepareLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_prepare_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosPromise(const std::string& shard_id, bool received) {
    incrementCounter("themis_paxos_promise_total", 
                    {{"shard_id", shard_id}, {"result", received ? "received" : "rejected"}});
}

void PrometheusMetrics::recordPaxosAccept(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_accept_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordPaxosAcceptLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_accept_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosAccepted(const std::string& shard_id, bool received) {
    incrementCounter("themis_paxos_accepted_total", 
                    {{"shard_id", shard_id}, {"result", received ? "received" : "rejected"}});
}

void PrometheusMetrics::recordPaxosLearn(const std::string& shard_id, uint64_t slot) {
    incrementCounter("themis_paxos_learn_total", {{"shard_id", shard_id}});
    setGauge("themis_paxos_last_learned_slot", static_cast<double>(slot), {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosLearnLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_learn_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosProposal(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_proposals_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordPaxosProposalDuration(const std::string& shard_id, double duration_ms) {
    observeHistogram("themis_paxos_proposal_duration_seconds", duration_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosProposalRetry(const std::string& shard_id) {
    incrementCounter("themis_paxos_proposal_retries_total", {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosProposalConflict(const std::string& shard_id) {
    incrementCounter("themis_paxos_proposal_conflicts_total", {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosConvergenceTime(const std::string& shard_id, double time_ms) {
    observeHistogram("themis_paxos_convergence_time_seconds", time_ms / 1000.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::setPaxosQuorumStatus(const std::string& shard_id, bool has_quorum) {
    setGauge("themis_paxos_has_quorum", has_quorum ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

void PrometheusMetrics::recordPaxosQuorumLoss(const std::string& shard_id) {
    incrementCounter("themis_paxos_quorum_loss_total", {{"shard_id", shard_id}});
}

// ==================== Cross-Shard Transaction Metrics Implementation (Phase 1) ====================

void PrometheusMetrics::record2PCTransaction(const std::string& coordinator_id, bool success) {
    incrementCounter("themis_2pc_transactions_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::record2PCPreparePhase(const std::string& coordinator_id, double duration_ms, bool all_prepared) {
    observeHistogram("themis_2pc_prepare_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_2pc_prepare_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", all_prepared ? "all_prepared" : "some_failed"}});
}

void PrometheusMetrics::record2PCCommitPhase(const std::string& coordinator_id, double duration_ms, bool success) {
    observeHistogram("themis_2pc_commit_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_2pc_commit_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::record2PCAbort(const std::string& coordinator_id, const std::string& reason) {
    incrementCounter("themis_2pc_aborts_total", 
                    {{"coordinator_id", coordinator_id}, {"reason", reason}});
}

void PrometheusMetrics::record2PCParticipantResponse(const std::string& participant_id, 
                                                      const std::string& phase, double latency_ms) {
    observeHistogram("themis_2pc_participant_response_latency_seconds", latency_ms / 1000.0, 
                    {{"participant_id", participant_id}, {"phase", phase}});
}

void PrometheusMetrics::record3PCTransaction(const std::string& coordinator_id, bool success) {
    incrementCounter("themis_3pc_transactions_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::record3PCPreCommitPhase(const std::string& coordinator_id, double duration_ms, bool success) {
    observeHistogram("themis_3pc_precommit_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_3pc_precommit_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::record3PCTimeout(const std::string& coordinator_id, const std::string& phase) {
    incrementCounter("themis_3pc_timeouts_total", 
                    {{"coordinator_id", coordinator_id}, {"phase", phase}});
}

void PrometheusMetrics::recordSAGATransaction(const std::string& saga_id, bool success) {
    incrementCounter("themis_saga_transactions_total", 
                    {{"saga_id", saga_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordSAGAStep(const std::string& saga_id, int step_number, bool success) {
    incrementCounter("themis_saga_steps_total", 
                    {{"saga_id", saga_id}, {"step", std::to_string(step_number)}, 
                     {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordSAGACompensation(const std::string& saga_id, int step_number, bool success) {
    incrementCounter("themis_saga_compensations_total", 
                    {{"saga_id", saga_id}, {"step", std::to_string(step_number)}, 
                     {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordSAGADuration(const std::string& saga_id, double duration_ms) {
    observeHistogram("themis_saga_duration_seconds", duration_ms / 1000.0, {{"saga_id", saga_id}});
}

void PrometheusMetrics::recordPercolatorTransaction(const std::string& transaction_id, bool success) {
    incrementCounter("themis_percolator_transactions_total", 
                    {{"transaction_id", transaction_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordPercolatorLockAcquisition(const std::string& transaction_id, double latency_ms, bool success) {
    observeHistogram("themis_percolator_lock_acquisition_latency_seconds", latency_ms / 1000.0, 
                    {{"transaction_id", transaction_id}});
    incrementCounter("themis_percolator_lock_acquisitions_total", 
                    {{"transaction_id", transaction_id}, {"result", success ? "success" : "failure"}});
}

void PrometheusMetrics::recordPercolatorLockRelease(const std::string& transaction_id, double latency_ms) {
    observeHistogram("themis_percolator_lock_release_latency_seconds", latency_ms / 1000.0, 
                    {{"transaction_id", transaction_id}});
    incrementCounter("themis_percolator_lock_releases_total", {{"transaction_id", transaction_id}});
}

void PrometheusMetrics::recordPercolatorWriteIntent(const std::string& transaction_id, int intent_count) {
    setGauge("themis_percolator_write_intents", static_cast<double>(intent_count), 
            {{"transaction_id", transaction_id}});
}

void PrometheusMetrics::recordPercolatorConflict(const std::string& transaction_id) {
    incrementCounter("themis_percolator_conflicts_total", {{"transaction_id", transaction_id}});
}

void PrometheusMetrics::setActiveTransactions([[maybe_unused]] int count) {
    setGauge("themis_active_transactions", static_cast<double>(count), {});
}

void PrometheusMetrics::setBlockedTransactions([[maybe_unused]] int count) {
    setGauge("themis_blocked_transactions", static_cast<double>(count), {});
}

void PrometheusMetrics::recordTransactionTimeout(const std::string& transaction_type) {
    incrementCounter("themis_transaction_timeouts_total", {{"type", transaction_type}});
}

// ─────────────────────────────────────────────────────────────────────────────
// Shard repair / anti-entropy metrics
// ─────────────────────────────────────────────────────────────────────────────

void PrometheusMetrics::recordRepairOperation(bool success, double duration_ms) {
    incrementCounter("themis_shard_repair_operations_total",
                     {{"result", success ? "success" : "failure"}});
    observeHistogram("themis_shard_repair_duration_seconds", duration_ms / 1000.0, {});
}

void PrometheusMetrics::recordRepairShardStatus(const std::string& shard_id,
                                                 const std::string& status) {
    // Use a gauge per (shard, status) pair: value is 1 for the active status, 0 for others.
    // Known statuses are defined in PrometheusMetrics::RepairShardStatus.
    static constexpr const char* kKnownStatuses[] = {
        RepairShardStatus::HEALTHY,
        RepairShardStatus::DEGRADED,
        RepairShardStatus::FAILED,
        RepairShardStatus::REBUILDING,
    };
    for (const auto* s : kKnownStatuses) {
        setGauge("themis_shard_repair_health",
                 (status == s) ? 1.0 : 0.0,
                 {{"shard_id", shard_id}, {"status", s}});
    }
}

void PrometheusMetrics::recordRepairScan() {
    incrementCounter("themis_shard_repair_scans_total", {});
}
// ─── MVCC / HLC Metrics ───────────────────────────────────────────────────────

void PrometheusMetrics::recordMvccWrite([[maybe_unused]] double latency_ms) {
    incrementCounter("themis_mvcc_writes_total", {});
    observeHistogram("themis_mvcc_write_latency_seconds", latency_ms / 1000.0, {});
}

void PrometheusMetrics::recordMvccRead(const std::string& read_type, double latency_ms) {
    incrementCounter("themis_mvcc_reads_total", {{"read_type", read_type}});
    observeHistogram("themis_mvcc_read_latency_seconds", latency_ms / 1000.0, {{"read_type", read_type}});
}

void PrometheusMetrics::recordMvccGc([[maybe_unused]] uint64_t versions_deleted) {
    incrementCounter("themis_mvcc_gc_runs_total", {});
    addToCounter("themis_mvcc_gc_versions_deleted_total", static_cast<int64_t>(versions_deleted), {});
    // Store the batch count in the histogram for distribution analysis.
    observeHistogram("themis_mvcc_gc_batch_size", static_cast<double>(versions_deleted), {});
}

void PrometheusMetrics::setMvccVersionCount(int64_t count) {
    setGauge("themis_mvcc_version_entries", static_cast<double>(count), {});
}

void PrometheusMetrics::recordHlcAdvance(const std::string& type) {
    incrementCounter("themis_hlc_advances_total", {{"type", type}});
}

void PrometheusMetrics::incrementCounter(const std::string& name, 
                                          const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[key]++;
}

void PrometheusMetrics::addToCounter(const std::string& name,
                                      int64_t amount,
                                      const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[key] += amount;
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
    if (labels.empty()) {
      return "";
    }
    
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) {
          oss << ",";
        }
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
