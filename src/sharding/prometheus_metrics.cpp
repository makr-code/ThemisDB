/**
 * @file prometheus_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

/**
 * @brief Record Rpc Call.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] method Input parameter.
 * @param[in] outcome Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */
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

/**
 * @brief Record Shard Health.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] status Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordShardHealth(const std::string& shard_id, const std::string& status) {
    setGauge("themis_shard_health_status", 1.0, {{"shard_id", shard_id}, {"status", status}});
}

/**
 * @brief Record Certificate Expiry.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] seconds_until_expiry Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordCertificateExpiry(const std::string& shard_id, int64_t seconds_until_expiry) {
    setGauge("themis_shard_certificate_expiry_seconds", static_cast<double>(seconds_until_expiry), 
             {{"shard_id", shard_id}});
}

/**
 * @brief Record Routing Request.
 * @param[in] type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRoutingRequest(const std::string& type) {
    incrementCounter("themis_routing_requests_total", {{"type", type}});
}

/**
 * @brief Record Routing Error.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] error_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRoutingError(const std::string& shard_id, const std::string& error_type) {
    incrementCounter("themis_routing_errors_total", {{"shard_id", shard_id}, {"error_type", error_type}});
}

/**
 * @brief Record Routing Latency.
 * @param[in] operation Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordRoutingLatency(const std::string& operation, double latency_ms) {
    observeHistogram("themis_routing_latency_seconds", latency_ms / 1000.0, {{"operation", operation}});
}

/**
 * @brief Record PKIConnection.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] result Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPKIConnection(const std::string& shard_id, const std::string& result) {
    incrementCounter("themis_pki_connections_total", {{"shard_id", shard_id}, {"result", result}});
}

/**
 * @brief Record Certificate Validation.
 * @param[in] result Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordCertificateValidation(const std::string& result) {
    incrementCounter("themis_pki_certificate_validations_total", {{"result", result}});
}

/**
 * @brief Record CRLCheck.
 * @param[in] result Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordCRLCheck(const std::string& result) {
    incrementCounter("themis_pki_crl_checks_total", {{"result", result}});
}

/**
 * @brief Record Migration Progress.
 * @param[in] operation_id Identifier of the operation.
 * @param[in] records Input parameter.
 * @param[in] bytes Input parameter.
 * @param[in] percent Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordMigrationProgress(const std::string& operation_id, int64_t records, 
                                                 int64_t bytes, double percent) {
    setGauge("themis_migration_records_total", static_cast<double>(records), {{"operation_id", operation_id}});
    setGauge("themis_migration_bytes_total", static_cast<double>(bytes), {{"operation_id", operation_id}});
    setGauge("themis_migration_progress_percent", percent, {{"operation_id", operation_id}});
}

/**
 * @brief Record Migration Duration.
 * @param[in] operation_id Identifier of the operation.
 * @param[in] duration_seconds Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordMigrationDuration(const std::string& operation_id, double duration_seconds) {
    setGauge("themis_migration_duration_seconds", duration_seconds, {{"operation_id", operation_id}});
}

/**
 * @brief Record Query Execution.
 * @param[in] query_type Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordQueryExecution(const std::string& query_type, double latency_ms) {
    observeHistogram("themis_query_execution_seconds", latency_ms / 1000.0, {{"query_type", query_type}});
}

/**
 * @brief Record Scatter Gather Fanout.
 * @param[in] num_shards Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordScatterGatherFanout(int num_shards) {
    observeHistogram("themis_scatter_gather_fanout", static_cast<double>(num_shards), {});
}

/**
 * @brief Record Result Merge Time.
 * @param[in] time_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordResultMergeTime(double time_ms) {
    observeHistogram("themis_result_merge_time_seconds", time_ms / 1000.0, {});
}

/**
 * @brief Record Topology Change.
 * @param[in] change_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordTopologyChange(const std::string& change_type) {
    incrementCounter("themis_topology_changes_total", {{"change_type", change_type}});
}

/**
 * @brief Record Cluster Size.
 * @param[in] num_shards Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordClusterSize(int num_shards) {
    setGauge("themis_cluster_size", static_cast<double>(num_shards), {});
}

/**
 * @brief Record Virtual Nodes.
 * @param[in] total_vnodes Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordVirtualNodes(int total_vnodes) {
    setGauge("themis_virtual_nodes_total", static_cast<double>(total_vnodes), {});
}

/**
 * @brief ==================== Phase 6 New Metrics Implementation ====================
 * @param[in] message_type Input parameter.
 * @details Calls: incrementCounter().
 */

void PrometheusMetrics::recordGossipMessage(const std::string& message_type) {
    incrementCounter("themis_gossip_messages_total", {{"type", message_type}});
}

/**
 * @brief Record Gossip Message Size.
 * @param[in] bytes Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordGossipMessageSize(int64_t bytes) {
    observeHistogram("themis_gossip_message_size_bytes", static_cast<double>(bytes), {});
}

/**
 * @brief Record Gossip Round Trip.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordGossipRoundTrip(double latency_ms) {
    observeHistogram("themis_gossip_roundtrip_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Gossip Peer Count.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordGossipPeerCount(int count) {
    setGauge("themis_gossip_peer_count", static_cast<double>(count), {});
}

/**
 * @brief Record Gossip Failed Peer.
 * @param[in] peer_id Identifier of the peer.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipFailedPeer(const std::string& peer_id) {
    incrementCounter("themis_gossip_failed_peers_total", {{"peer_id", peer_id}});
}

/**
 * @brief Record Gossip Version Vector.
 * @param[in] peer_id Identifier of the peer.
 * @param[in] version Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordGossipVersionVector(const std::string& peer_id, uint64_t version) {
    setGauge("themis_gossip_version_vector", static_cast<double>(version), {{"peer_id", peer_id}});
}

/**
 * @brief ==================== Gossip Config Manager Metrics Implementation ====================
 * @param[in] operation Input parameter.
 * @details Calls: incrementCounter().
 */

void PrometheusMetrics::recordGossipConfigUpdate(const std::string& operation) {
    incrementCounter("themis_gossip_config_updates_total", {{"operation", operation}});
}

/**
 * @brief Record Gossip Config Update Latency.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordGossipConfigUpdateLatency(double latency_ms) {
    observeHistogram("themis_gossip_config_update_latency_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Gossip Config Conflict.
 * @param[in] resolution_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipConfigConflict(const std::string& resolution_type) {
    incrementCounter("themis_gossip_config_conflicts_total", {{"resolution", resolution_type}});
}

/**
 * @brief Record Gossip Resource Snapshot.
 * @param[in] operation Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipResourceSnapshot(const std::string& operation) {
    incrementCounter("themis_gossip_resource_snapshots_total", {{"operation", operation}});
}

/**
 * @brief Record Gossip Resource Snapshot Latency.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordGossipResourceSnapshotLatency(double latency_ms) {
    observeHistogram("themis_gossip_resource_snapshot_latency_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Gossip Config Round.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipConfigRound() {
    incrementCounter("themis_gossip_config_rounds_total", {});
}

/**
 * @brief Record Gossip Config Anti Entropy.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipConfigAntiEntropy() {
    incrementCounter("themis_gossip_config_anti_entropy_syncs_total", {});
}

/**
 * @brief Set Gossip Config Peer Count.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setGossipConfigPeerCount(int count) {
    setGauge("themis_gossip_config_peer_count", static_cast<double>(count), {});
}

/**
 * @brief Observe Gossip Propagation Latency.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::observeGossipPropagationLatency(double latency_ms) {
    observeHistogram("themis_gossip_propagation_latency_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Gossip Messages Sent.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipMessagesSent() {
    incrementCounter("themis_gossip_config_messages_sent_total", {});
}

/**
 * @brief Record Gossip Messages Received.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordGossipMessagesReceived() {
    incrementCounter("themis_gossip_config_messages_received_total", {});
}

// ====================================================================================

/**
 * @brief Record Cross Shard Request.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] operation Input parameter.
 * @param[in] outcome Input parameter.
 */
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

/**
 * @brief Record Cross Shard Join.
 * @param[in] strategy Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordCrossShardJoin(const std::string& strategy) {
    incrementCounter("themis_cross_shard_joins_total", {{"strategy", strategy}});
}

/**
 * @brief Record Cross Shard Join Duration.
 * @param[in] strategy Input parameter.
 * @param[in] duration_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordCrossShardJoinDuration(const std::string& strategy, double duration_ms) {
    observeHistogram("themis_cross_shard_join_duration_seconds", duration_ms / 1000.0, {{"strategy", strategy}});
}

/**
 * @brief Record Cross Shard Join Rows.
 * @param[in] strategy Input parameter.
 * @param[in] left_rows Input parameter.
 * @param[in] right_rows Input parameter.
 * @param[in] result_rows Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordCrossShardJoinRows(const std::string& strategy, int64_t left_rows, 
                                                   int64_t right_rows, int64_t result_rows) {
    setGauge("themis_cross_shard_join_left_rows", static_cast<double>(left_rows), {{"strategy", strategy}});
    setGauge("themis_cross_shard_join_right_rows", static_cast<double>(right_rows), {{"strategy", strategy}});
    setGauge("themis_cross_shard_join_result_rows", static_cast<double>(result_rows), {{"strategy", strategy}});
}

/**
 * @brief Record Hash Table Build Time.
 * @param[in] time_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordHashTableBuildTime(double time_ms) {
    observeHistogram("themis_hash_table_build_seconds", time_ms / 1000.0, {});
}

/**
 * @brief Record Probe Phase Time.
 * @param[in] time_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordProbePhaseTime(double time_ms) {
    observeHistogram("themis_probe_phase_seconds", time_ms / 1000.0, {});
}

/**
 * @brief Record Content Processor Invocation.
 * @param[in] processor_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordContentProcessorInvocation(const std::string& processor_type) {
    incrementCounter("themis_content_processor_invocations_total", {{"type", processor_type}});
}

/**
 * @brief Record Content Processor Duration.
 * @param[in] processor_type Input parameter.
 * @param[in] duration_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordContentProcessorDuration(const std::string& processor_type, double duration_ms) {
    observeHistogram("themis_content_processor_duration_seconds", duration_ms / 1000.0, {{"type", processor_type}});
}

/**
 * @brief Record Content Processor Error.
 * @param[in] processor_type Input parameter.
 * @param[in] error_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordContentProcessorError(const std::string& processor_type, const std::string& error_type) {
    incrementCounter("themis_content_processor_errors_total", {{"type", processor_type}, {"error", error_type}});
}

/**
 * @brief Record Content Processor Bytes.
 * @param[in] processor_type Input parameter.
 * @param[in] input_bytes Input parameter.
 * @param[in] output_bytes Input parameter.
 * @details Calls: incrementCounter(), setGauge().
 */
void PrometheusMetrics::recordContentProcessorBytes(const std::string& processor_type, int64_t input_bytes, 
                                                      int64_t output_bytes) {
    incrementCounter("themis_content_processor_input_bytes_total", {{"type", processor_type}});
    setGauge("themis_content_processor_last_input_bytes", static_cast<double>(input_bytes), {{"type", processor_type}});
    setGauge("themis_content_processor_last_output_bytes", static_cast<double>(output_bytes), {{"type", processor_type}});
}

/**
 * @brief Record Metadata Store Operation.
 * @param[in] operation Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordMetadataStoreOperation(const std::string& operation) {
    incrementCounter("themis_metadata_store_operations_total", {{"operation", operation}});
}

/**
 * @brief Record Metadata Store Latency.
 * @param[in] operation Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordMetadataStoreLatency(const std::string& operation, double latency_ms) {
    observeHistogram("themis_metadata_store_latency_seconds", latency_ms / 1000.0, {{"operation", operation}});
}

/**
 * @brief Record Metadata Store Error.
 * @param[in] operation Input parameter.
 * @param[in] error_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordMetadataStoreError(const std::string& operation, const std::string& error_type) {
    incrementCounter("themis_metadata_store_errors_total", {{"operation", operation}, {"error", error_type}});
}

/**
 * @brief Record Health Check Execution.
 * @param[in] check_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordHealthCheckExecution(const std::string& check_type) {
    incrementCounter("themis_health_check_executions_total", {{"type", check_type}});
}

/**
 * @brief Record Health Check Duration.
 * @param[in] check_type Input parameter.
 * @param[in] duration_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordHealthCheckDuration(const std::string& check_type, double duration_ms) {
    observeHistogram("themis_health_check_duration_seconds", duration_ms / 1000.0, {{"type", check_type}});
}

/**
 * @brief Record Health Check Result.
 * @param[in] check_type Input parameter.
 * @param[in] result Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordHealthCheckResult(const std::string& check_type, const std::string& result) {
    incrementCounter("themis_health_check_results_total", {{"type", check_type}, {"result", result}});
}

/**
 * @brief Record Cloud Agent Operation.
 * @param[in] operation Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordCloudAgentOperation(const std::string& operation) {
    incrementCounter("themis_cloud_agent_operations_total", {{"operation", operation}});
}

/**
 * @brief Record Datacenter Latency.
 * @param[in] datacenter Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordDatacenterLatency(const std::string& datacenter, double latency_ms) {
    observeHistogram("themis_datacenter_latency_seconds", latency_ms / 1000.0, {{"datacenter", datacenter}});
}

/**
 * @brief Record Cross DCRequest.
 * @param[in] source_dc Input parameter.
 * @param[in] target_dc Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordCrossDCRequest(const std::string& source_dc, const std::string& target_dc) {
    incrementCounter("themis_cross_dc_requests_total", {{"source", source_dc}, {"target", target_dc}});
}

/**
 * @brief ==================== Replication Metrics Implementation ====================
 * @param[in] replica_id Identifier of the replica.
 * @param[in] entries Input parameter.
 * @param[in] bytes Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), getCounterKey(), fetch_add().
 */

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

/**
 * @brief Record Wal Ship Latency.
 * @param[in] replica_id Identifier of the replica.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordWalShipLatency(const std::string& replica_id, double latency_ms) {
    observeHistogram("themis_wal_ship_latency_seconds", latency_ms / 1000.0, {{"replica_id", replica_id}});
}

/**
 * @brief Record Wal Replication Lag.
 * @param[in] replica_id Identifier of the replica.
 * @param[in] lag_seconds Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordWalReplicationLag(const std::string& replica_id, double lag_seconds) {
    setGauge("themis_wal_replication_lag_seconds", lag_seconds, {{"replica_id", replica_id}});
}

/**
 * @brief Set Wal Backlog Bytes.
 * @param[in] replica_id Identifier of the replica.
 * @param[in] bytes Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setWalBacklogBytes(const std::string& replica_id, int64_t bytes) {
    setGauge("themis_wal_backlog_bytes", static_cast<double>(bytes), {{"replica_id", replica_id}});
}

/**
 * @brief Record Wal Compression Ratio.
 * @param[in] ratio Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordWalCompressionRatio(double ratio) {
    observeHistogram("themis_wal_compression_ratio", ratio, {});
}

/**
 * @brief Record Wal Apply Batch.
 * @param[in] entries Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), getCounterKey(), fetch_add().
 */
void PrometheusMetrics::recordWalApplyBatch(int64_t entries, bool success) {
    std::string result = success ? "success" : "failure";
    incrementCounter("themis_wal_apply_batches_total", {{"result", result}});
    
    if (success) {
        counters_[getCounterKey("themis_wal_apply_entries_total", {})].fetch_add(entries, std::memory_order_relaxed);
    } else {
        incrementCounter("themis_wal_apply_failures_total", {});
    }
}

/**
 * @brief Record Wal Apply Latency.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordWalApplyLatency(double latency_ms) {
    observeHistogram("themis_wal_apply_latency_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Wal Apply Failure.
 * @param[in] error_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordWalApplyFailure(const std::string& error_type) {
    incrementCounter("themis_wal_apply_errors_total", {{"error_type", error_type}});
}

/**
 * @brief Set Wal Last Applied Lsn.
 * @param[in] lsn Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setWalLastAppliedLsn(const std::string& lsn) {
    setGauge("themis_wal_last_applied_lsn", 1.0, {{"lsn", lsn}});
}

/**
 * @brief Record Write Concern Wait.
 * @param[in] level Input parameter.
 * @param[in] wait_time_ms Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */
void PrometheusMetrics::recordWriteConcernWait(const std::string& level, double wait_time_ms, bool success) {
    std::string result = success ? "success" : "timeout";
    incrementCounter("themis_write_concern_waits_total", {{"level", level}, {"result", result}});
    observeHistogram("themis_write_concern_wait_seconds", wait_time_ms / 1000.0, {{"level", level}});
}

/**
 * @brief Set Pending Writes.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setPendingWrites(int64_t count) {
    setGauge("themis_replication_pending_writes", static_cast<double>(count), {});
}

/**
 * @brief Record Quorum Timeout.
 * @param[in] level Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordQuorumTimeout(const std::string& level) {
    incrementCounter("themis_replication_quorum_timeouts_total", {{"level", level}});
}

std::string PrometheusMetrics::getMetrics() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss = {};

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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss = {};

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
        oss << key << "_count " <<sorted.size() << "\n";
    }

    return oss.str();
}

/**
 * @brief ==================== Raft Consensus Metrics Implementation ====================
 * @param[in] shard_id Identifier of the shard.
 * @param[in] role Input parameter.
 * @details Calls: setGauge().
 */

void PrometheusMetrics::setRaftRole(const std::string& shard_id, const std::string& role) {
    setGauge("themis_raft_role", role == "LEADER" ? 2.0 : (role == "CANDIDATE" ? 1.0 : 0.0),
             {{"shard_id", shard_id}, {"role", role}});
}

/**
 * @brief Set Raft Term.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] term Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftTerm(const std::string& shard_id, uint64_t term) {
    setGauge("themis_raft_term", static_cast<double>(term), {{"shard_id", shard_id}});
}

/**
 * @brief Set Raft Commit Index.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] commit_index Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftCommitIndex(const std::string& shard_id, uint64_t commit_index) {
    setGauge("themis_raft_commit_index", static_cast<double>(commit_index), {{"shard_id", shard_id}});
}

/**
 * @brief Set Raft Last Applied.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] last_applied Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftLastApplied(const std::string& shard_id, uint64_t last_applied) {
    setGauge("themis_raft_last_applied", static_cast<double>(last_applied), {{"shard_id", shard_id}});
}

/**
 * @brief Set Raft Log Size.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] log_size Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftLogSize(const std::string& shard_id, uint64_t log_size) {
    setGauge("themis_raft_log_size", static_cast<double>(log_size), {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Leader Election.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] duration_ms Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */
void PrometheusMetrics::recordRaftLeaderElection(const std::string& shard_id, double duration_ms) {
    incrementCounter("themis_raft_leader_elections_total", {{"shard_id", shard_id}});
    observeHistogram("themis_raft_leader_election_duration_seconds", duration_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Leader Change.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] old_leader Input parameter.
 * @param[in] new_leader Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRaftLeaderChange(const std::string& shard_id, 
                                               const std::string& old_leader, 
                                               const std::string& new_leader) {
    incrementCounter("themis_raft_leader_changes_total", 
                    {{"shard_id", shard_id}, {"old_leader", old_leader}, {"new_leader", new_leader}});
}

/**
 * @brief Record Raft Heartbeat.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRaftHeartbeat(const std::string& shard_id, bool success) {
    incrementCounter("themis_raft_heartbeats_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Raft Heartbeat Latency.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordRaftHeartbeatLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_raft_heartbeat_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Log Append.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] entries_count Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */
void PrometheusMetrics::recordRaftLogAppend(const std::string& shard_id, uint64_t entries_count, bool success) {
    incrementCounter("themis_raft_log_appends_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
    if (success) {
        observeHistogram("themis_raft_log_append_entries", static_cast<double>(entries_count), {{"shard_id", shard_id}});
    }
}

/**
 * @brief Record Raft Log Append Latency.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordRaftLogAppendLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_raft_log_append_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Replication Lag.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] follower_id Identifier of the follower.
 * @param[in] lag_entries Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordRaftReplicationLag(const std::string& shard_id, 
                                                 const std::string& follower_id, 
                                                 uint64_t lag_entries) {
    setGauge("themis_raft_replication_lag_entries", static_cast<double>(lag_entries), 
            {{"shard_id", shard_id}, {"follower_id", follower_id}});
}

/**
 * @brief Set Raft Quorum Status.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] has_quorum Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftQuorumStatus(const std::string& shard_id, bool has_quorum) {
    setGauge("themis_raft_has_quorum", has_quorum ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Partition Detected.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRaftPartitionDetected(const std::string& shard_id) {
    incrementCounter("themis_raft_partitions_detected_total", {{"shard_id", shard_id}});
}

/**
 * @brief Record Raft Partition Healed.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRaftPartitionHealed(const std::string& shard_id) {
    incrementCounter("themis_raft_partitions_healed_total", {{"shard_id", shard_id}});
}

/**
 * @brief Set Raft Read Only Mode.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] is_read_only Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setRaftReadOnlyMode(const std::string& shard_id, bool is_read_only) {
    setGauge("themis_raft_read_only_mode", is_read_only ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

/**
 * @brief ==================== Paxos Consensus Metrics Implementation (Phase 1) ====================
 * @param[in] shard_id Identifier of the shard.
 * @param[in] role Input parameter.
 * @details Calls: setGauge().
 */

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

/**
 * @brief Set Paxos Round.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] round Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setPaxosRound(const std::string& shard_id, uint64_t round) {
    setGauge("themis_paxos_round", static_cast<double>(round), {{"shard_id", shard_id}});
}

/**
 * @brief Set Paxos Highest Proposal.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] proposal_number Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setPaxosHighestProposal(const std::string& shard_id, uint64_t proposal_number) {
    setGauge("themis_paxos_highest_proposal", static_cast<double>(proposal_number), {{"shard_id", shard_id}});
}

/**
 * @brief Set Paxos Committed Slot.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] slot Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setPaxosCommittedSlot(const std::string& shard_id, uint64_t slot) {
    setGauge("themis_paxos_committed_slot", static_cast<double>(slot), {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Prepare.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosPrepare(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_prepare_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Paxos Prepare Latency.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordPaxosPrepareLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_prepare_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Promise.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] received Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosPromise(const std::string& shard_id, bool received) {
    incrementCounter("themis_paxos_promise_total", 
                    {{"shard_id", shard_id}, {"result", received ? "received" : "rejected"}});
}

/**
 * @brief Record Paxos Accept.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosAccept(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_accept_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Paxos Accept Latency.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordPaxosAcceptLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_accept_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Accepted.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] received Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosAccepted(const std::string& shard_id, bool received) {
    incrementCounter("themis_paxos_accepted_total", 
                    {{"shard_id", shard_id}, {"result", received ? "received" : "rejected"}});
}

/**
 * @brief Record Paxos Learn.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] slot Input parameter.
 * @details Calls: incrementCounter(), setGauge().
 */
void PrometheusMetrics::recordPaxosLearn(const std::string& shard_id, uint64_t slot) {
    incrementCounter("themis_paxos_learn_total", {{"shard_id", shard_id}});
    setGauge("themis_paxos_last_learned_slot", static_cast<double>(slot), {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Learn Latency.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordPaxosLearnLatency(const std::string& shard_id, double latency_ms) {
    observeHistogram("themis_paxos_learn_latency_seconds", latency_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Proposal.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosProposal(const std::string& shard_id, bool success) {
    incrementCounter("themis_paxos_proposals_total", 
                    {{"shard_id", shard_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Paxos Proposal Duration.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] duration_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordPaxosProposalDuration(const std::string& shard_id, double duration_ms) {
    observeHistogram("themis_paxos_proposal_duration_seconds", duration_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Proposal Retry.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosProposalRetry(const std::string& shard_id) {
    incrementCounter("themis_paxos_proposal_retries_total", {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Proposal Conflict.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosProposalConflict(const std::string& shard_id) {
    incrementCounter("themis_paxos_proposal_conflicts_total", {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Convergence Time.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] time_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordPaxosConvergenceTime(const std::string& shard_id, double time_ms) {
    observeHistogram("themis_paxos_convergence_time_seconds", time_ms / 1000.0, {{"shard_id", shard_id}});
}

/**
 * @brief Set Paxos Quorum Status.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] has_quorum Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setPaxosQuorumStatus(const std::string& shard_id, bool has_quorum) {
    setGauge("themis_paxos_has_quorum", has_quorum ? 1.0 : 0.0, {{"shard_id", shard_id}});
}

/**
 * @brief Record Paxos Quorum Loss.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPaxosQuorumLoss(const std::string& shard_id) {
    incrementCounter("themis_paxos_quorum_loss_total", {{"shard_id", shard_id}});
}

/**
 * @brief ==================== Cross-Shard Transaction Metrics Implementation (Phase 1) ====================
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */

void PrometheusMetrics::record2PCTransaction(const std::string& coordinator_id, bool success) {
    incrementCounter("themis_2pc_transactions_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record2 PCPrepare Phase.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] duration_ms Input parameter.
 * @param[in] all_prepared Input parameter.
 * @details Calls: observeHistogram(), incrementCounter().
 */
void PrometheusMetrics::record2PCPreparePhase(const std::string& coordinator_id, double duration_ms, bool all_prepared) {
    observeHistogram("themis_2pc_prepare_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_2pc_prepare_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", all_prepared ? "all_prepared" : "some_failed"}});
}

/**
 * @brief Record2 PCCommit Phase.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] duration_ms Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: observeHistogram(), incrementCounter().
 */
void PrometheusMetrics::record2PCCommitPhase(const std::string& coordinator_id, double duration_ms, bool success) {
    observeHistogram("themis_2pc_commit_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_2pc_commit_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record2 PCAbort.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] reason Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::record2PCAbort(const std::string& coordinator_id, const std::string& reason) {
    incrementCounter("themis_2pc_aborts_total", 
                    {{"coordinator_id", coordinator_id}, {"reason", reason}});
}

/**
 * @brief Record2 PCParticipant Response.
 * @param[in] participant_id Identifier of the participant.
 * @param[in] phase Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::record2PCParticipantResponse(const std::string& participant_id, 
                                                      const std::string& phase, double latency_ms) {
    observeHistogram("themis_2pc_participant_response_latency_seconds", latency_ms / 1000.0, 
                    {{"participant_id", participant_id}, {"phase", phase}});
}

/**
 * @brief Record3 PCTransaction.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::record3PCTransaction(const std::string& coordinator_id, bool success) {
    incrementCounter("themis_3pc_transactions_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record3 PCPre Commit Phase.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] duration_ms Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: observeHistogram(), incrementCounter().
 */
void PrometheusMetrics::record3PCPreCommitPhase(const std::string& coordinator_id, double duration_ms, bool success) {
    observeHistogram("themis_3pc_precommit_phase_duration_seconds", duration_ms / 1000.0, 
                    {{"coordinator_id", coordinator_id}});
    incrementCounter("themis_3pc_precommit_phase_total", 
                    {{"coordinator_id", coordinator_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record3 PCTimeout.
 * @param[in] coordinator_id Identifier of the coordinator.
 * @param[in] phase Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::record3PCTimeout(const std::string& coordinator_id, const std::string& phase) {
    incrementCounter("themis_3pc_timeouts_total", 
                    {{"coordinator_id", coordinator_id}, {"phase", phase}});
}

/**
 * @brief Record SAGATransaction.
 * @param[in] saga_id Identifier of the saga.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordSAGATransaction(const std::string& saga_id, bool success) {
    incrementCounter("themis_saga_transactions_total", 
                    {{"saga_id", saga_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record SAGAStep.
 * @param[in] saga_id Identifier of the saga.
 * @param[in] step_number Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), std::to_string().
 */
void PrometheusMetrics::recordSAGAStep(const std::string& saga_id, int step_number, bool success) {
    incrementCounter("themis_saga_steps_total", 
                    {{"saga_id", saga_id}, {"step", std::to_string(step_number)}, 
                     {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record SAGACompensation.
 * @param[in] saga_id Identifier of the saga.
 * @param[in] step_number Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter(), std::to_string().
 */
void PrometheusMetrics::recordSAGACompensation(const std::string& saga_id, int step_number, bool success) {
    incrementCounter("themis_saga_compensations_total", 
                    {{"saga_id", saga_id}, {"step", std::to_string(step_number)}, 
                     {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record SAGADuration.
 * @param[in] saga_id Identifier of the saga.
 * @param[in] duration_ms Input parameter.
 * @details Calls: observeHistogram().
 */
void PrometheusMetrics::recordSAGADuration(const std::string& saga_id, double duration_ms) {
    observeHistogram("themis_saga_duration_seconds", duration_ms / 1000.0, {{"saga_id", saga_id}});
}

/**
 * @brief Record Percolator Transaction.
 * @param[in] transaction_id Identifier of the transaction.
 * @param[in] success Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPercolatorTransaction(const std::string& transaction_id, bool success) {
    incrementCounter("themis_percolator_transactions_total", 
                    {{"transaction_id", transaction_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Percolator Lock Acquisition.
 * @param[in] transaction_id Identifier of the transaction.
 * @param[in] latency_ms Input parameter.
 * @param[in] success Input parameter.
 * @details Calls: observeHistogram(), incrementCounter().
 */
void PrometheusMetrics::recordPercolatorLockAcquisition(const std::string& transaction_id, double latency_ms, bool success) {
    observeHistogram("themis_percolator_lock_acquisition_latency_seconds", latency_ms / 1000.0, 
                    {{"transaction_id", transaction_id}});
    incrementCounter("themis_percolator_lock_acquisitions_total", 
                    {{"transaction_id", transaction_id}, {"result", success ? "success" : "failure"}});
}

/**
 * @brief Record Percolator Lock Release.
 * @param[in] transaction_id Identifier of the transaction.
 * @param[in] latency_ms Input parameter.
 * @details Calls: observeHistogram(), incrementCounter().
 */
void PrometheusMetrics::recordPercolatorLockRelease(const std::string& transaction_id, double latency_ms) {
    observeHistogram("themis_percolator_lock_release_latency_seconds", latency_ms / 1000.0, 
                    {{"transaction_id", transaction_id}});
    incrementCounter("themis_percolator_lock_releases_total", {{"transaction_id", transaction_id}});
}

/**
 * @brief Record Percolator Write Intent.
 * @param[in] transaction_id Identifier of the transaction.
 * @param[in] intent_count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::recordPercolatorWriteIntent(const std::string& transaction_id, int intent_count) {
    setGauge("themis_percolator_write_intents", static_cast<double>(intent_count), 
            {{"transaction_id", transaction_id}});
}

/**
 * @brief Record Percolator Conflict.
 * @param[in] transaction_id Identifier of the transaction.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordPercolatorConflict(const std::string& transaction_id) {
    incrementCounter("themis_percolator_conflicts_total", {{"transaction_id", transaction_id}});
}

/**
 * @brief Set Active Transactions.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setActiveTransactions(int count) {
    setGauge("themis_active_transactions", static_cast<double>(count), {});
}

/**
 * @brief Set Blocked Transactions.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setBlockedTransactions(int count) {
    setGauge("themis_blocked_transactions", static_cast<double>(count), {});
}

/**
 * @brief Record Transaction Timeout.
 * @param[in] transaction_type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordTransactionTimeout(const std::string& transaction_type) {
    incrementCounter("themis_transaction_timeouts_total", {{"type", transaction_type}});
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── Shard repair / anti-entropy metrics ─────────────────────────────────────────────────────────────────────────────
 * @param[in] success Input parameter.
 * @param[in] duration_ms Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */

void PrometheusMetrics::recordRepairOperation(bool success, double duration_ms) {
    incrementCounter("themis_shard_repair_operations_total",
                     {{"result", success ? "success" : "failure"}});
    observeHistogram("themis_shard_repair_duration_seconds", duration_ms / 1000.0, {});
}

/**
 * @brief Record Repair Shard Status.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] status Input parameter.
 * @details Calls: setGauge().
 */
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

/**
 * @brief Record Repair Scan.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordRepairScan() {
    incrementCounter("themis_shard_repair_scans_total", {});
}
/**
 * @brief ─── MVCC / HLC Metrics ───────────────────────────────────────────────────────
 * @param[in] latency_ms Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */

void PrometheusMetrics::recordMvccWrite(double latency_ms) {
    incrementCounter("themis_mvcc_writes_total", {});
    observeHistogram("themis_mvcc_write_latency_seconds", latency_ms / 1000.0, {});
}

/**
 * @brief Record Mvcc Read.
 * @param[in] read_type Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: incrementCounter(), observeHistogram().
 */
void PrometheusMetrics::recordMvccRead(const std::string& read_type, double latency_ms) {
    incrementCounter("themis_mvcc_reads_total", {{"read_type", read_type}});
    observeHistogram("themis_mvcc_read_latency_seconds", latency_ms / 1000.0, {{"read_type", read_type}});
}

/**
 * @brief Record Mvcc Gc.
 * @param[in] versions_deleted Input parameter.
 * @details Calls: incrementCounter(), addToCounter(), observeHistogram().
 */
void PrometheusMetrics::recordMvccGc(uint64_t versions_deleted) {
    incrementCounter("themis_mvcc_gc_runs_total", {});
    addToCounter("themis_mvcc_gc_versions_deleted_total", static_cast<int64_t>(versions_deleted), {});
    // Store the batch count in the histogram for distribution analysis.
    observeHistogram("themis_mvcc_gc_batch_size", static_cast<double>(versions_deleted), {});
}

/**
 * @brief Set Mvcc Version Count.
 * @param[in] count Input parameter.
 * @details Calls: setGauge().
 */
void PrometheusMetrics::setMvccVersionCount(int64_t count) {
    setGauge("themis_mvcc_version_entries", static_cast<double>(count), {});
}

/**
 * @brief Record Hlc Advance.
 * @param[in] type Input parameter.
 * @details Calls: incrementCounter().
 */
void PrometheusMetrics::recordHlcAdvance(const std::string& type) {
    incrementCounter("themis_hlc_advances_total", {{"type", type}});
}

void PrometheusMetrics::incrementCounter(const std::string& name, 
                                          const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[key]++;
}

void PrometheusMetrics::addToCounter(const std::string& name,
                                      int64_t amount,
                                      const std::map<std::string, std::string>& labels) {
    std::string key = getCounterKey(name, labels);
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
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
    
    std::ostringstream oss = {};
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
