/**
 * @file prometheus_metrics.h
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

    /**
     * @brief TBD: Describe PrometheusMetrics.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief Shard health metrics
     * @param[in] shard_id Input parameter.
     * @param[in] status Input parameter.
     */
    void recordShardHealth(const std::string& shard_id, const std::string& status);
    /**
     * @brief TBD: Describe recordCertificateExpiry.
     * @param[in] shard_id Input parameter.
     * @param[in] seconds_until_expiry Input parameter.
     */
    void recordCertificateExpiry(const std::string& shard_id, int64_t seconds_until_expiry);

    /**
     * @brief Routing statistics
     * @param[in] type Input parameter.
     */
    void recordRoutingRequest(const std::string& type); // local/remote/scatter_gather
    /**
     * @brief TBD: Describe recordRoutingError.
     * @param[in] shard_id Input parameter.
     * @param[in] error_type Input parameter.
     */
    void recordRoutingError(const std::string& shard_id, const std::string& error_type);
    /**
     * @brief TBD: Describe recordRoutingLatency.
     * @param[in] operation Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordRoutingLatency(const std::string& operation, double latency_ms);

    /**
     * @brief PKI events
     * @param[in] shard_id Input parameter.
     * @param[in] result Input parameter.
     */
    void recordPKIConnection(const std::string& shard_id, const std::string& result); // success/failure
    /**
     * @brief TBD: Describe recordCertificateValidation.
     * @param[in] result Input parameter.
     */
    void recordCertificateValidation(const std::string& result);
    /**
     * @brief TBD: Describe recordCRLCheck.
     * @param[in] result Input parameter.
     */
    void recordCRLCheck(const std::string& result);

    /**
     * @brief Migration progress
     * @param[in] operation_id Input parameter.
     * @param[in] records Input parameter.
     * @param[in] bytes Input parameter.
     * @param[in] percent Input parameter.
     */
    void recordMigrationProgress(const std::string& operation_id, int64_t records, int64_t bytes, double percent);
    /**
     * @brief TBD: Describe recordMigrationDuration.
     * @param[in] operation_id Input parameter.
     * @param[in] duration_seconds Input parameter.
     */
    void recordMigrationDuration(const std::string& operation_id, double duration_seconds);

    /**
     * @brief Query performance
     * @param[in] query_type Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordQueryExecution(const std::string& query_type, double latency_ms);
    /**
     * @brief TBD: Describe recordScatterGatherFanout.
     * @param[in] num_shards Input parameter.
     */
    void recordScatterGatherFanout(int num_shards);
    /**
     * @brief TBD: Describe recordResultMergeTime.
     * @param[in] time_ms Input parameter.
     */
    void recordResultMergeTime(double time_ms);

    /**
     * @brief Topology changes
     * @param[in] change_type Input parameter.
     */
    void recordTopologyChange(const std::string& change_type); // add/remove
    /**
     * @brief TBD: Describe recordClusterSize.
     * @param[in] num_shards Input parameter.
     */
    void recordClusterSize(int num_shards);
    /**
     * @brief TBD: Describe recordVirtualNodes.
     * @param[in] total_vnodes Input parameter.
     */
    void recordVirtualNodes(int total_vnodes);

    // ==================== Phase 6 New Metrics ====================

    /**
     * @brief Gossip Protocol metrics
     * @param[in] message_type Input parameter.
     */
    void recordGossipMessage(const std::string& message_type); // heartbeat/peer_list/ack
    /**
     * @brief TBD: Describe recordGossipMessageSize.
     * @param[in] bytes Input parameter.
     */
    void recordGossipMessageSize(int64_t bytes);
    /**
     * @brief TBD: Describe recordGossipRoundTrip.
     * @param[in] latency_ms Input parameter.
     */
    void recordGossipRoundTrip(double latency_ms);
    /**
     * @brief TBD: Describe recordGossipPeerCount.
     * @param[in] count Input parameter.
     */
    void recordGossipPeerCount(int count);
    /**
     * @brief TBD: Describe recordGossipFailedPeer.
     * @param[in] peer_id Input parameter.
     */
    void recordGossipFailedPeer(const std::string& peer_id);
    /**
     * @brief TBD: Describe recordGossipVersionVector.
     * @param[in] peer_id Input parameter.
     * @param[in] version Input parameter.
     */
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

    /**
     * @brief Cross-Shard Join metrics
     * @param[in] strategy Input parameter.
     */
    void recordCrossShardJoin(const std::string& strategy); // broadcast_hash/co_located
    /**
     * @brief TBD: Describe recordCrossShardJoinDuration.
     * @param[in] strategy Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordCrossShardJoinDuration(const std::string& strategy, double duration_ms);
    /**
     * @brief TBD: Describe recordCrossShardJoinRows.
     * @param[in] strategy Input parameter.
     * @param[in] left_rows Input parameter.
     * @param[in] right_rows Input parameter.
     * @param[in] result_rows Input parameter.
     */
    void recordCrossShardJoinRows(const std::string& strategy, int64_t left_rows, int64_t right_rows, int64_t result_rows);
    /**
     * @brief TBD: Describe recordHashTableBuildTime.
     * @param[in] time_ms Input parameter.
     */
    void recordHashTableBuildTime(double time_ms);
    /**
     * @brief TBD: Describe recordProbePhaseTime.
     * @param[in] time_ms Input parameter.
     */
    void recordProbePhaseTime(double time_ms);

    /**
     * @brief Content Processor metrics
     * @param[in] processor_type Input parameter.
     */
    void recordContentProcessorInvocation(const std::string& processor_type); // pdf/office/video/audio/geo/image/cad
    /**
     * @brief TBD: Describe recordContentProcessorDuration.
     * @param[in] processor_type Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordContentProcessorDuration(const std::string& processor_type, double duration_ms);
    /**
     * @brief TBD: Describe recordContentProcessorError.
     * @param[in] processor_type Input parameter.
     * @param[in] error_type Input parameter.
     */
    void recordContentProcessorError(const std::string& processor_type, const std::string& error_type);
    /**
     * @brief TBD: Describe recordContentProcessorBytes.
     * @param[in] processor_type Input parameter.
     * @param[in] input_bytes Input parameter.
     * @param[in] output_bytes Input parameter.
     */
    void recordContentProcessorBytes(const std::string& processor_type, int64_t input_bytes, int64_t output_bytes);

    /**
     * @brief etcd/Metadata Store metrics
     * @param[in] operation Input parameter.
     */
    void recordMetadataStoreOperation(const std::string& operation); // get/put/delete/watch
    /**
     * @brief TBD: Describe recordMetadataStoreLatency.
     * @param[in] operation Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordMetadataStoreLatency(const std::string& operation, double latency_ms);
    /**
     * @brief TBD: Describe recordMetadataStoreError.
     * @param[in] operation Input parameter.
     * @param[in] error_type Input parameter.
     */
    void recordMetadataStoreError(const std::string& operation, const std::string& error_type);

    /**
     * @brief Health Check metrics
     * @param[in] check_type Input parameter.
     */
    void recordHealthCheckExecution(const std::string& check_type); // certificate/storage/network
    /**
     * @brief TBD: Describe recordHealthCheckDuration.
     * @param[in] check_type Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordHealthCheckDuration(const std::string& check_type, double duration_ms);
    /**
     * @brief TBD: Describe recordHealthCheckResult.
     * @param[in] check_type Input parameter.
     * @param[in] result Input parameter.
     */
    void recordHealthCheckResult(const std::string& check_type, const std::string& result); // healthy/warning/critical

    /**
     * @brief Cloud Agent metrics
     * @param[in] operation Input parameter.
     */
    void recordCloudAgentOperation(const std::string& operation); // scatter_gather/dc_routing
    /**
     * @brief TBD: Describe recordDatacenterLatency.
     * @param[in] datacenter Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordDatacenterLatency(const std::string& datacenter, double latency_ms);
    /**
     * @brief TBD: Describe recordCrossDCRequest.
     * @param[in] source_dc Input parameter.
     * @param[in] target_dc Input parameter.
     */
    void recordCrossDCRequest(const std::string& source_dc, const std::string& target_dc);

    // ==================== Gossip Config Manager Metrics ====================
    
    /**
     * @brief Config update metrics
     * @param[in] operation Input parameter.
     */
    void recordGossipConfigUpdate(const std::string& operation); // sent/received
    /**
     * @brief TBD: Describe recordGossipConfigUpdateLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordGossipConfigUpdateLatency(double latency_ms);
    /**
     * @brief TBD: Describe recordGossipConfigConflict.
     * @param[in] resolution_type Input parameter.
     */
    void recordGossipConfigConflict(const std::string& resolution_type);
    
    /**
     * @brief Resource snapshot metrics
     * @param[in] operation Input parameter.
     */
    void recordGossipResourceSnapshot(const std::string& operation); // sent/received
    /**
     * @brief TBD: Describe recordGossipResourceSnapshotLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordGossipResourceSnapshotLatency(double latency_ms);
    
    /**
     * @brief Gossip round metrics
     */
    void recordGossipConfigRound();
    /**
     * @brief TBD: Describe recordGossipConfigAntiEntropy.
     */
    void recordGossipConfigAntiEntropy();
    /**
     * @brief TBD: Describe setGossipConfigPeerCount.
     * @param[in] count Input parameter.
     */
    void setGossipConfigPeerCount(int count);
    
    /**
     * @brief Propagation metrics
     * @param[in] latency_ms Input parameter.
     */
    void observeGossipPropagationLatency(double latency_ms);
    /**
     * @brief TBD: Describe recordGossipMessagesSent.
     */
    void recordGossipMessagesSent();
    /**
     * @brief TBD: Describe recordGossipMessagesReceived.
     */
    void recordGossipMessagesReceived();

    // ==================== Replication Metrics (RAID1/10) ====================

    /**
     * @brief WALShipper metrics
     * @param[in] replica_id Input parameter.
     * @param[in] entries Input parameter.
     * @param[in] bytes Input parameter.
     * @param[in] success Input parameter.
     */
    void recordWalShipBatch(const std::string& replica_id, int64_t entries, int64_t bytes, bool success);
    /**
     * @brief TBD: Describe recordWalShipLatency.
     * @param[in] replica_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordWalShipLatency(const std::string& replica_id, double latency_ms);
    /**
     * @brief TBD: Describe recordWalReplicationLag.
     * @param[in] replica_id Input parameter.
     * @param[in] lag_seconds Input parameter.
     */
    void recordWalReplicationLag(const std::string& replica_id, double lag_seconds);
    /**
     * @brief TBD: Describe setWalBacklogBytes.
     * @param[in] replica_id Input parameter.
     * @param[in] bytes Input parameter.
     */
    void setWalBacklogBytes(const std::string& replica_id, int64_t bytes);
    /**
     * @brief TBD: Describe recordWalCompressionRatio.
     * @param[in] ratio Input parameter.
     */
    void recordWalCompressionRatio(double ratio);
    
    /**
     * @brief WALApplier metrics
     * @param[in] entries Input parameter.
     * @param[in] success Input parameter.
     */
    void recordWalApplyBatch(int64_t entries, bool success);
    /**
     * @brief TBD: Describe recordWalApplyLatency.
     * @param[in] latency_ms Input parameter.
     */
    void recordWalApplyLatency(double latency_ms);
    /**
     * @brief TBD: Describe recordWalApplyFailure.
     * @param[in] error_type Input parameter.
     */
    void recordWalApplyFailure(const std::string& error_type);
    /**
     * @brief TBD: Describe setWalLastAppliedLsn.
     * @param[in] lsn Input parameter.
     */
    void setWalLastAppliedLsn(const std::string& lsn);
    
    /**
     * @brief ReplicationCoordinator metrics
     * @param[in] level Input parameter.
     * @param[in] wait_time_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void recordWriteConcernWait(const std::string& level, double wait_time_ms, bool success);
    /**
     * @brief TBD: Describe setPendingWrites.
     * @param[in] count Input parameter.
     */
    void setPendingWrites(int64_t count);
    /**
     * @brief TBD: Describe recordQuorumTimeout.
     * @param[in] level Input parameter.
     */
    void recordQuorumTimeout(const std::string& level);
    
    // ==================== Raft Consensus Metrics ====================
    
    /**
     * @brief Raft state metrics per shard
     * @param[in] shard_id Input parameter.
     * @param[in] role Input parameter.
     */
    void setRaftRole(const std::string& shard_id, const std::string& role); // LEADER/FOLLOWER/CANDIDATE
    /**
     * @brief TBD: Describe setRaftTerm.
     * @param[in] shard_id Input parameter.
     * @param[in] term Input parameter.
     */
    void setRaftTerm(const std::string& shard_id, uint64_t term);
    /**
     * @brief TBD: Describe setRaftCommitIndex.
     * @param[in] shard_id Input parameter.
     * @param[in] commit_index Input parameter.
     */
    void setRaftCommitIndex(const std::string& shard_id, uint64_t commit_index);
    /**
     * @brief TBD: Describe setRaftLastApplied.
     * @param[in] shard_id Input parameter.
     * @param[in] last_applied Input parameter.
     */
    void setRaftLastApplied(const std::string& shard_id, uint64_t last_applied);
    /**
     * @brief TBD: Describe setRaftLogSize.
     * @param[in] shard_id Input parameter.
     * @param[in] log_size Input parameter.
     */
    void setRaftLogSize(const std::string& shard_id, uint64_t log_size);
    
    /**
     * @brief Raft leadership metrics
     * @param[in] shard_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordRaftLeaderElection(const std::string& shard_id, double duration_ms);
    /**
     * @brief TBD: Describe recordRaftLeaderChange.
     * @param[in] shard_id Input parameter.
     * @param[in] old_leader Input parameter.
     * @param[in] new_leader Input parameter.
     */
    void recordRaftLeaderChange(const std::string& shard_id, const std::string& old_leader, const std::string& new_leader);
    /**
     * @brief TBD: Describe recordRaftHeartbeat.
     * @param[in] shard_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordRaftHeartbeat(const std::string& shard_id, bool success);
    /**
     * @brief TBD: Describe recordRaftHeartbeatLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordRaftHeartbeatLatency(const std::string& shard_id, double latency_ms);
    
    /**
     * @brief Raft replication metrics
     * @param[in] shard_id Input parameter.
     * @param[in] entries_count Input parameter.
     * @param[in] success Input parameter.
     */
    void recordRaftLogAppend(const std::string& shard_id, uint64_t entries_count, bool success);
    /**
     * @brief TBD: Describe recordRaftLogAppendLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordRaftLogAppendLatency(const std::string& shard_id, double latency_ms);
    /**
     * @brief TBD: Describe recordRaftReplicationLag.
     * @param[in] shard_id Input parameter.
     * @param[in] follower_id Input parameter.
     * @param[in] lag_entries Input parameter.
     */
    void recordRaftReplicationLag(const std::string& shard_id, const std::string& follower_id, uint64_t lag_entries);
    /**
     * @brief TBD: Describe setRaftQuorumStatus.
     * @param[in] shard_id Input parameter.
     * @param[in] has_quorum Input parameter.
     */
    void setRaftQuorumStatus(const std::string& shard_id, bool has_quorum);
    
    /**
     * @brief Raft partition detection metrics
     * @param[in] shard_id Input parameter.
     */
    void recordRaftPartitionDetected(const std::string& shard_id);
    /**
     * @brief TBD: Describe recordRaftPartitionHealed.
     * @param[in] shard_id Input parameter.
     */
    void recordRaftPartitionHealed(const std::string& shard_id);
    /**
     * @brief TBD: Describe setRaftReadOnlyMode.
     * @param[in] shard_id Input parameter.
     * @param[in] is_read_only Input parameter.
     */
    void setRaftReadOnlyMode(const std::string& shard_id, bool is_read_only);

    // ==================== Paxos Consensus Metrics (Phase 1) ====================
    
    /**
     * @brief Paxos state metrics per shard
     * @param[in] shard_id Input parameter.
     * @param[in] role Input parameter.
     */
    void setPaxosRole(const std::string& shard_id, const std::string& role); // LEADER/FOLLOWER/PROPOSER/ACCEPTOR/LEARNER
    /**
     * @brief TBD: Describe setPaxosRound.
     * @param[in] shard_id Input parameter.
     * @param[in] round Input parameter.
     */
    void setPaxosRound(const std::string& shard_id, uint64_t round);
    /**
     * @brief TBD: Describe setPaxosHighestProposal.
     * @param[in] shard_id Input parameter.
     * @param[in] proposal_number Input parameter.
     */
    void setPaxosHighestProposal(const std::string& shard_id, uint64_t proposal_number);
    /**
     * @brief TBD: Describe setPaxosCommittedSlot.
     * @param[in] shard_id Input parameter.
     * @param[in] slot Input parameter.
     */
    void setPaxosCommittedSlot(const std::string& shard_id, uint64_t slot);
    
    /**
     * @brief Paxos proposal phase metrics
     * @param[in] shard_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordPaxosPrepare(const std::string& shard_id, bool success);
    /**
     * @brief TBD: Describe recordPaxosPrepareLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPaxosPrepareLatency(const std::string& shard_id, double latency_ms);
    /**
     * @brief TBD: Describe recordPaxosPromise.
     * @param[in] shard_id Input parameter.
     * @param[in] received Input parameter.
     */
    void recordPaxosPromise(const std::string& shard_id, bool received);
    
    /**
     * @brief Paxos accept phase metrics
     * @param[in] shard_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordPaxosAccept(const std::string& shard_id, bool success);
    /**
     * @brief TBD: Describe recordPaxosAcceptLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPaxosAcceptLatency(const std::string& shard_id, double latency_ms);
    /**
     * @brief TBD: Describe recordPaxosAccepted.
     * @param[in] shard_id Input parameter.
     * @param[in] received Input parameter.
     */
    void recordPaxosAccepted(const std::string& shard_id, bool received);
    
    /**
     * @brief Paxos learn phase metrics
     * @param[in] shard_id Input parameter.
     * @param[in] slot Input parameter.
     */
    void recordPaxosLearn(const std::string& shard_id, uint64_t slot);
    /**
     * @brief TBD: Describe recordPaxosLearnLatency.
     * @param[in] shard_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPaxosLearnLatency(const std::string& shard_id, double latency_ms);
    
    /**
     * @brief Paxos proposal metrics
     * @param[in] shard_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordPaxosProposal(const std::string& shard_id, bool success);
    /**
     * @brief TBD: Describe recordPaxosProposalDuration.
     * @param[in] shard_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordPaxosProposalDuration(const std::string& shard_id, double duration_ms);
    /**
     * @brief TBD: Describe recordPaxosProposalRetry.
     * @param[in] shard_id Input parameter.
     */
    void recordPaxosProposalRetry(const std::string& shard_id);
    /**
     * @brief TBD: Describe recordPaxosProposalConflict.
     * @param[in] shard_id Input parameter.
     */
    void recordPaxosProposalConflict(const std::string& shard_id);
    
    /**
     * @brief Paxos convergence metrics
     * @param[in] shard_id Input parameter.
     * @param[in] time_ms Input parameter.
     */
    void recordPaxosConvergenceTime(const std::string& shard_id, double time_ms);
    /**
     * @brief TBD: Describe setPaxosQuorumStatus.
     * @param[in] shard_id Input parameter.
     * @param[in] has_quorum Input parameter.
     */
    void setPaxosQuorumStatus(const std::string& shard_id, bool has_quorum);
    /**
     * @brief TBD: Describe recordPaxosQuorumLoss.
     * @param[in] shard_id Input parameter.
     */
    void recordPaxosQuorumLoss(const std::string& shard_id);
    
    // ==================== Cross-Shard Transaction Metrics (Phase 1) ====================
    
    /**
     * @brief 2PC Transaction metrics
     * @param[in] coordinator_id Input parameter.
     * @param[in] success Input parameter.
     */
    void record2PCTransaction(const std::string& coordinator_id, bool success);
    /**
     * @brief TBD: Describe record2PCPreparePhase.
     * @param[in] coordinator_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] all_prepared Input parameter.
     */
    void record2PCPreparePhase(const std::string& coordinator_id, double duration_ms, bool all_prepared);
    /**
     * @brief TBD: Describe record2PCCommitPhase.
     * @param[in] coordinator_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void record2PCCommitPhase(const std::string& coordinator_id, double duration_ms, bool success);
    /**
     * @brief TBD: Describe record2PCAbort.
     * @param[in] coordinator_id Input parameter.
     * @param[in] reason Input parameter.
     */
    void record2PCAbort(const std::string& coordinator_id, const std::string& reason);
    /**
     * @brief TBD: Describe record2PCParticipantResponse.
     * @param[in] participant_id Input parameter.
     * @param[in] phase Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void record2PCParticipantResponse(const std::string& participant_id, const std::string& phase, double latency_ms);
    
    /**
     * @brief 3PC Transaction metrics
     * @param[in] coordinator_id Input parameter.
     * @param[in] success Input parameter.
     */
    void record3PCTransaction(const std::string& coordinator_id, bool success);
    /**
     * @brief TBD: Describe record3PCPreCommitPhase.
     * @param[in] coordinator_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void record3PCPreCommitPhase(const std::string& coordinator_id, double duration_ms, bool success);
    /**
     * @brief TBD: Describe record3PCTimeout.
     * @param[in] coordinator_id Input parameter.
     * @param[in] phase Input parameter.
     */
    void record3PCTimeout(const std::string& coordinator_id, const std::string& phase);
    
    /**
     * @brief SAGA Transaction metrics
     * @param[in] saga_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordSAGATransaction(const std::string& saga_id, bool success);
    /**
     * @brief TBD: Describe recordSAGAStep.
     * @param[in] saga_id Input parameter.
     * @param[in] step_number Input parameter.
     * @param[in] success Input parameter.
     */
    void recordSAGAStep(const std::string& saga_id, int step_number, bool success);
    /**
     * @brief TBD: Describe recordSAGACompensation.
     * @param[in] saga_id Input parameter.
     * @param[in] step_number Input parameter.
     * @param[in] success Input parameter.
     */
    void recordSAGACompensation(const std::string& saga_id, int step_number, bool success);
    /**
     * @brief TBD: Describe recordSAGADuration.
     * @param[in] saga_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordSAGADuration(const std::string& saga_id, double duration_ms);
    
    /**
     * @brief Percolator Transaction metrics
     * @param[in] transaction_id Input parameter.
     * @param[in] success Input parameter.
     */
    void recordPercolatorTransaction(const std::string& transaction_id, bool success);
    /**
     * @brief TBD: Describe recordPercolatorLockAcquisition.
     * @param[in] transaction_id Input parameter.
     * @param[in] latency_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void recordPercolatorLockAcquisition(const std::string& transaction_id, double latency_ms, bool success);
    /**
     * @brief TBD: Describe recordPercolatorLockRelease.
     * @param[in] transaction_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPercolatorLockRelease(const std::string& transaction_id, double latency_ms);
    /**
     * @brief TBD: Describe recordPercolatorWriteIntent.
     * @param[in] transaction_id Input parameter.
     * @param[in] intent_count Input parameter.
     */
    void recordPercolatorWriteIntent(const std::string& transaction_id, int intent_count);
    /**
     * @brief TBD: Describe recordPercolatorConflict.
     * @param[in] transaction_id Input parameter.
     */
    void recordPercolatorConflict(const std::string& transaction_id);
    
    /**
     * @brief Transaction coordinator state metrics
     * @param[in] count Input parameter.
     */
    void setActiveTransactions(int count);
    /**
     * @brief TBD: Describe setBlockedTransactions.
     * @param[in] count Input parameter.
     */
    void setBlockedTransactions(int count);
    /**
     * @brief TBD: Describe recordTransactionTimeout.
     * @param[in] transaction_type Input parameter.
     */
    void recordTransactionTimeout(const std::string& transaction_type);

    // Shard repair / anti-entropy metrics

    /// Valid shard health status strings for recordRepairShardStatus().
    struct RepairShardStatus {
        static constexpr const char* HEALTHY    = "healthy";
        static constexpr const char* DEGRADED   = "degraded";
        static constexpr const char* FAILED     = "failed";
        static constexpr const char* REBUILDING = "rebuilding";
    };

    /**
     * @brief Record a completed repair attempt on a document.
     * @param[in] success Input parameter.
     * @param[in] duration_ms Input parameter.
     * @details @param success Whether the repair succeeded. @param duration_ms Wall-clock time of the repair operation in milliseconds.
     */
    void recordRepairOperation(bool success, double duration_ms);

    /**
     * @brief Update the health gauge for a shard as observed by the repair engine.
     * @param[in] shard_id Input parameter.
     * @param[in] status Input parameter.
     * @details @param shard_id Shard identifier. @param status One of RepairShardStatus::{HEALTHY,DEGRADED,FAILED,REBUILDING}. Unknown values are silently ignored (all known gauges are set to 0).
     */
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

    /**
     * @brief Get metrics in Prometheus text format
     * @return Return value.
     */
    std::string getMetrics() const;

    /**
     * @brief Get metrics with HELP and TYPE annotations
     * @return Return value.
     */
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
