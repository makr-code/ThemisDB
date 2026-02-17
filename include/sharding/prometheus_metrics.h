#ifndef THEMIS_SHARDING_PROMETHEUS_METRICS_H
#define THEMIS_SHARDING_PROMETHEUS_METRICS_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

namespace themis {
namespace sharding {

/**
 * Prometheus metrics exporter for horizontal sharding system.
 * 
 * Exposes metrics in Prometheus text format via HTTP endpoint /metrics.
 * Tracks shard health, routing statistics, PKI events, migration progress,
 * query performance, topology changes, gossip protocol, and cross-shard joins.
 */
class PrometheusMetrics {
public:
    struct Config {
        int http_port = 8080;
        std::string http_path = "/metrics";
        bool enable_histograms = true;
        int histogram_buckets = 10;
    };

    explicit PrometheusMetrics(const Config& config);
    ~PrometheusMetrics() = default;

    // Shard health metrics
    void recordShardHealth(const std::string& shard_id, const std::string& status);
    void recordCertificateExpiry(const std::string& shard_id, int64_t seconds_until_expiry);

    // Routing statistics
    void recordRoutingRequest(const std::string& type); // local/remote/scatter_gather
    void recordRoutingError(const std::string& shard_id, const std::string& error_type);
    void recordRoutingLatency(const std::string& operation, double latency_ms);

    // PKI events
    void recordPKIConnection(const std::string& shard_id, const std::string& result); // success/failure
    void recordCertificateValidation(const std::string& result);
    void recordCRLCheck(const std::string& result);

    // Migration progress
    void recordMigrationProgress(const std::string& operation_id, int64_t records, int64_t bytes, double percent);
    void recordMigrationDuration(const std::string& operation_id, double duration_seconds);

    // Query performance
    void recordQueryExecution(const std::string& query_type, double latency_ms);
    void recordScatterGatherFanout(int num_shards);
    void recordResultMergeTime(double time_ms);

    // Topology changes
    void recordTopologyChange(const std::string& change_type); // add/remove
    void recordClusterSize(int num_shards);
    void recordVirtualNodes(int total_vnodes);

    // ==================== Phase 6 New Metrics ====================

    // Gossip Protocol metrics
    void recordGossipMessage(const std::string& message_type); // heartbeat/peer_list/ack
    void recordGossipMessageSize(int64_t bytes);
    void recordGossipRoundTrip(double latency_ms);
    void recordGossipPeerCount(int count);
    void recordGossipFailedPeer(const std::string& peer_id);
    void recordGossipVersionVector(const std::string& peer_id, uint64_t version);

    // Cross-Shard Join metrics
    void recordCrossShardJoin(const std::string& strategy); // broadcast_hash/co_located
    void recordCrossShardJoinDuration(const std::string& strategy, double duration_ms);
    void recordCrossShardJoinRows(const std::string& strategy, int64_t left_rows, int64_t right_rows, int64_t result_rows);
    void recordHashTableBuildTime(double time_ms);
    void recordProbePhaseTime(double time_ms);

    // Content Processor metrics
    void recordContentProcessorInvocation(const std::string& processor_type); // pdf/office/video/audio/geo/image/cad
    void recordContentProcessorDuration(const std::string& processor_type, double duration_ms);
    void recordContentProcessorError(const std::string& processor_type, const std::string& error_type);
    void recordContentProcessorBytes(const std::string& processor_type, int64_t input_bytes, int64_t output_bytes);

    // etcd/Metadata Store metrics
    void recordMetadataStoreOperation(const std::string& operation); // get/put/delete/watch
    void recordMetadataStoreLatency(const std::string& operation, double latency_ms);
    void recordMetadataStoreError(const std::string& operation, const std::string& error_type);

    // Health Check metrics
    void recordHealthCheckExecution(const std::string& check_type); // certificate/storage/network
    void recordHealthCheckDuration(const std::string& check_type, double duration_ms);
    void recordHealthCheckResult(const std::string& check_type, const std::string& result); // healthy/warning/critical

    // Cloud Agent metrics
    void recordCloudAgentOperation(const std::string& operation); // scatter_gather/dc_routing
    void recordDatacenterLatency(const std::string& datacenter, double latency_ms);
    void recordCrossDCRequest(const std::string& source_dc, const std::string& target_dc);

    // ==================== Gossip Config Manager Metrics ====================
    
    // Config update metrics
    void recordGossipConfigUpdate(const std::string& operation); // sent/received
    void recordGossipConfigUpdateLatency(double latency_ms);
    void recordGossipConfigConflict(const std::string& resolution_type);
    
    // Resource snapshot metrics
    void recordGossipResourceSnapshot(const std::string& operation); // sent/received
    void recordGossipResourceSnapshotLatency(double latency_ms);
    
    // Gossip round metrics
    void recordGossipConfigRound();
    void recordGossipConfigAntiEntropy();
    void setGossipConfigPeerCount(int count);
    
    // Propagation metrics
    void observeGossipPropagationLatency(double latency_ms);
    void recordGossipMessagesSent();
    void recordGossipMessagesReceived();

    // ==================== Replication Metrics (RAID1/10) ====================

    // WALShipper metrics
    void recordWalShipBatch(const std::string& replica_id, int64_t entries, int64_t bytes, bool success);
    void recordWalShipLatency(const std::string& replica_id, double latency_ms);
    void recordWalReplicationLag(const std::string& replica_id, double lag_seconds);
    void setWalBacklogBytes(const std::string& replica_id, int64_t bytes);
    void recordWalCompressionRatio(double ratio);
    
    // WALApplier metrics
    void recordWalApplyBatch(int64_t entries, bool success);
    void recordWalApplyLatency(double latency_ms);
    void recordWalApplyFailure(const std::string& error_type);
    void setWalLastAppliedLsn(const std::string& lsn);
    
    // ReplicationCoordinator metrics
    void recordWriteConcernWait(const std::string& level, double wait_time_ms, bool success);
    void setPendingWrites(int64_t count);
    void recordQuorumTimeout(const std::string& level);
    
    // ==================== Raft Consensus Metrics ====================
    
    // Raft state metrics per shard
    void setRaftRole(const std::string& shard_id, const std::string& role); // LEADER/FOLLOWER/CANDIDATE
    void setRaftTerm(const std::string& shard_id, uint64_t term);
    void setRaftCommitIndex(const std::string& shard_id, uint64_t commit_index);
    void setRaftLastApplied(const std::string& shard_id, uint64_t last_applied);
    void setRaftLogSize(const std::string& shard_id, uint64_t log_size);
    
    // Raft leadership metrics
    void recordRaftLeaderElection(const std::string& shard_id, double duration_ms);
    void recordRaftLeaderChange(const std::string& shard_id, const std::string& old_leader, const std::string& new_leader);
    void recordRaftHeartbeat(const std::string& shard_id, bool success);
    void recordRaftHeartbeatLatency(const std::string& shard_id, double latency_ms);
    
    // Raft replication metrics
    void recordRaftLogAppend(const std::string& shard_id, uint64_t entries_count, bool success);
    void recordRaftLogAppendLatency(const std::string& shard_id, double latency_ms);
    void recordRaftReplicationLag(const std::string& shard_id, const std::string& follower_id, uint64_t lag_entries);
    void setRaftQuorumStatus(const std::string& shard_id, bool has_quorum);
    
    // Raft partition detection metrics
    void recordRaftPartitionDetected(const std::string& shard_id);
    void recordRaftPartitionHealed(const std::string& shard_id);
    void setRaftReadOnlyMode(const std::string& shard_id, bool is_read_only);

    // Generic metrics (for extensibility)
    void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {});
    void setGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});
    void observeHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {});

    // Get metrics in Prometheus text format
    std::string getMetrics() const;

    // Get metrics with HELP and TYPE annotations
    std::string getMetricsWithAnnotations() const;

private:
    Config config_;
    mutable std::mutex mutex_;

    // Counters
    std::map<std::string, std::atomic<int64_t>> counters_;
    
    // Gauges
    std::map<std::string, std::atomic<double>> gauges_;
    
    // Histograms (simplified - stores recent values)
    std::map<std::string, std::vector<double>> histograms_;
    
    std::string formatLabels(const std::map<std::string, std::string>& labels) const;
    std::string getCounterKey(const std::string& name, const std::map<std::string, std::string>& labels) const;
};

} // namespace sharding
} // namespace themis

#endif // THEMIS_SHARDING_PROMETHEUS_METRICS_H
