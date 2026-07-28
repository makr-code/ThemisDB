/**
 * @file prometheus_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: prometheus_metrics.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

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

    // Cross-shard RPC metrics
    /**
     * @brief Record a cross-shard RPC call attempt.
     * @param shard_id  Target shard identifier (label).
     * @param method    RPC method name: "prepare", "commit", "abort", etc.
     * @param outcome   "success", "retryable_error", or "non_retryable_error".
     * @param latency_ms Round-trip latency of this single attempt in milliseconds.
     */
    void recordRpcCall(
        const std::string& shard_id,
        const std::string& method,
        const std::string& outcome,
        double latency_ms
    );

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

    // -------------------------------------------------------------------------
    // sharding_cross_shard_requests_total (Phase C observability gate)
    // -------------------------------------------------------------------------

    /**
     * @brief Increment the canonical cross-shard request counter.
     *
     * Emits the Prometheus counter `sharding_cross_shard_requests_total` with
     * labels `{shard_id, operation, outcome}`.  This counter is the Phase C
     * observability gate required by the sharding module roadmap.
     *
     * @param shard_id   Target shard identifier ("shard-0", "shard-7", …).
     * @param operation  Logical operation: "route", "scatter_gather", "prepare",
     *                   "commit", "abort", "repair", "anti_entropy".
     * @param outcome    "success", "error", or "timeout".
     *
     * @note Call this for every cross-shard RPC dispatch point, not just joins.
     *       For join-specific metrics use recordCrossShardJoin().
     */
    void recordCrossShardRequest(
        const std::string& shard_id,
        const std::string& operation,
        const std::string& outcome);

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

    // ==================== Paxos Consensus Metrics (Phase 1) ====================
    
    // Paxos state metrics per shard
    void setPaxosRole(const std::string& shard_id, const std::string& role); // LEADER/FOLLOWER/PROPOSER/ACCEPTOR/LEARNER
    void setPaxosRound(const std::string& shard_id, uint64_t round);
    void setPaxosHighestProposal(const std::string& shard_id, uint64_t proposal_number);
    void setPaxosCommittedSlot(const std::string& shard_id, uint64_t slot);
    
    // Paxos proposal phase metrics
    void recordPaxosPrepare(const std::string& shard_id, bool success);
    void recordPaxosPrepareLatency(const std::string& shard_id, double latency_ms);
    void recordPaxosPromise(const std::string& shard_id, bool received);
    
    // Paxos accept phase metrics
    void recordPaxosAccept(const std::string& shard_id, bool success);
    void recordPaxosAcceptLatency(const std::string& shard_id, double latency_ms);
    void recordPaxosAccepted(const std::string& shard_id, bool received);
    
    // Paxos learn phase metrics
    void recordPaxosLearn(const std::string& shard_id, uint64_t slot);
    void recordPaxosLearnLatency(const std::string& shard_id, double latency_ms);
    
    // Paxos proposal metrics
    void recordPaxosProposal(const std::string& shard_id, bool success);
    void recordPaxosProposalDuration(const std::string& shard_id, double duration_ms);
    void recordPaxosProposalRetry(const std::string& shard_id);
    void recordPaxosProposalConflict(const std::string& shard_id);
    
    // Paxos convergence metrics
    void recordPaxosConvergenceTime(const std::string& shard_id, double time_ms);
    void setPaxosQuorumStatus(const std::string& shard_id, bool has_quorum);
    void recordPaxosQuorumLoss(const std::string& shard_id);
    
    // ==================== Cross-Shard Transaction Metrics (Phase 1) ====================
    
    // 2PC Transaction metrics
    void record2PCTransaction(const std::string& coordinator_id, bool success);
    void record2PCPreparePhase(const std::string& coordinator_id, double duration_ms, bool all_prepared);
    void record2PCCommitPhase(const std::string& coordinator_id, double duration_ms, bool success);
    void record2PCAbort(const std::string& coordinator_id, const std::string& reason);
    void record2PCParticipantResponse(const std::string& participant_id, const std::string& phase, double latency_ms);
    
    // 3PC Transaction metrics
    void record3PCTransaction(const std::string& coordinator_id, bool success);
    void record3PCPreCommitPhase(const std::string& coordinator_id, double duration_ms, bool success);
    void record3PCTimeout(const std::string& coordinator_id, const std::string& phase);
    
    // SAGA Transaction metrics
    void recordSAGATransaction(const std::string& saga_id, bool success);
    void recordSAGAStep(const std::string& saga_id, int step_number, bool success);
    void recordSAGACompensation(const std::string& saga_id, int step_number, bool success);
    void recordSAGADuration(const std::string& saga_id, double duration_ms);
    
    // Percolator Transaction metrics
    void recordPercolatorTransaction(const std::string& transaction_id, bool success);
    void recordPercolatorLockAcquisition(const std::string& transaction_id, double latency_ms, bool success);
    void recordPercolatorLockRelease(const std::string& transaction_id, double latency_ms);
    void recordPercolatorWriteIntent(const std::string& transaction_id, int intent_count);
    void recordPercolatorConflict(const std::string& transaction_id);
    
    // Transaction coordinator state metrics
    void setActiveTransactions(int count);
    void setBlockedTransactions(int count);
    void recordTransactionTimeout(const std::string& transaction_type);

    // Shard repair / anti-entropy metrics

    /// Valid shard health status strings for recordRepairShardStatus().
    struct RepairShardStatus {
        static constexpr const char* HEALTHY    = "healthy";
        static constexpr const char* DEGRADED   = "degraded";
        static constexpr const char* FAILED     = "failed";
        static constexpr const char* REBUILDING = "rebuilding";
    };

    /// Record a completed repair attempt on a document.
    /// @param success      Whether the repair succeeded.
    /// @param duration_ms  Wall-clock time of the repair operation in milliseconds.
    void recordRepairOperation(bool success, double duration_ms);

    /// Update the health gauge for a shard as observed by the repair engine.
    /// @param shard_id  Shard identifier.
    /// @param status    One of RepairShardStatus::{HEALTHY,DEGRADED,FAILED,REBUILDING}.
    ///                  Unknown values are silently ignored (all known gauges are set to 0).
    void recordRepairShardStatus(const std::string& shard_id, const std::string& status);

    /// Record one anti-entropy scan completion.
    void recordRepairScan();
    // ==================== MVCC / HLC Metrics ====================

    /**
     * @brief Record a completed MVCC write operation.
     * @param latency_ms Write latency in milliseconds.
     */
    void recordMvccWrite(double latency_ms);

    /**
     * @brief Record a completed MVCC read operation.
     * @param read_type "latest" for linearizable reads, "snapshot" for
     *        point-in-time reads.
     * @param latency_ms Read latency in milliseconds.
     */
    void recordMvccRead(const std::string& read_type, double latency_ms);

    /**
     * @brief Record a completed MVCC garbage-collection run.
     * @param versions_deleted Number of old version entries removed.
     */
    void recordMvccGc(uint64_t versions_deleted);

    /**
     * @brief Update the gauge tracking total live MVCC version entries.
     * @param count Current total count of stored versions.
     */
    void setMvccVersionCount(int64_t count);

    /**
     * @brief Record a clock advance (HLC `now()` or `update()` call).
     * @param type "local" for `now()`, "received" for `update()`.
     */
    void recordHlcAdvance(const std::string& type);

    // Generic metrics (for extensibility)
    void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {});
    void addToCounter(const std::string& name, int64_t amount, const std::map<std::string, std::string>& labels = {});
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
