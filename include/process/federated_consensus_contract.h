// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file federated_consensus_contract.h
 * @brief Distributed consensus contracts for federated process module deployments.
 * @version 2.1.0-beta
 *
 * @section purpose Purpose
 * Defines consensus protocols, leader election semantics, replication contracts, and
 * split-brain recovery procedures for cross-shard process model consistency in federated
 * deployments (3+ shards).
 *
 * @section consensus_model Consensus Model
 *
 * The federated process module supports three consensus types:
 * - **Raft:** Leader-based, strongly consistent, automatic leader election
 * - **Paxos:** Quorum-based, strongly consistent, no single leader required
 * - **Gossip:** Leaderless, eventual consistency, Byzantine-tolerant
 *
 * Selection is configuration-time; runtime switching deferred to v3.0.
 *
 * @section process_replication Process Replication
 *
 * Process model changes are replicated via consensus log:
 * - **LogEntry:** {term/epoch, index, timestamp, operation_type (import/link/delete), model_id, payload_hash}
 * - **Versioning:** Extends existing process module version clocks with shard_id prefix
 * - **Snapshot:** Full model snapshot + delta log (every N entries)
 * - **Rollback:** Rejected entries expire; diverged replicas sync from leader via snapshot
 *
 * @section leader_election Leader Election (Raft/Paxos)
 *
 * **Raft Heartbeat:**
 * - Leader sends heartbeat every 150ms (configurable)
 * - Heartbeat failure detection: 3-5 consecutive missed → follower → candidate → election
 * - Election timeout: 150-300ms jitter (prevents simultaneous candidates)
 * - Quorum: simple majority (e.g., 3-node cluster requires 2 votes)
 *
 * **Split-Brain Detection & Recovery:**
 * - Split-brain occurs when cluster partitions into two quorums
 * - Outcome: Each partition elects a leader; higher term wins on reunion
 * - Detection window: <30 seconds (heartbeat + election timeout)
 * - Recovery: Leader with higher term invalidates lower-term leader; replicas resync from winner
 *
 * **Byzantine Tolerance (Gossip only):**
 * - Gossip protocol tolerates up to (n-1)/3 faulty nodes
 * - Faulty node detection: version vector divergence + majority voting
 *
 * @section replication_protocol Replication Protocol
 *
 * ### Raft-Style Replication
 *
 * **Phase 1: Leader receives write request**
 * - Write request arrives at leader (or forwarded from follower)
 * - Request logged locally; returns immediately with term + index (uncommitted)
 * - Client waits for confirmation
 *
 * **Phase 2: Leader replicates to followers**
 * - Leader appends entry to each follower's log
 * - Follower acknowledges receipt; leader counts votes
 * - Quorum achieved: entry committed; leader notifies followers
 * - Timeout (e.g., 5 seconds): leader assumes follower failed; retries or excludes
 *
 * **Phase 3: Leader applies and responds**
 * - Committed entry applied to process model state machine
 * - Response sent to client with final version number
 * - Followers eventually apply committed entries (asynchronous)
 *
 * ### Paxos-Style Consensus
 * - Proposer sends Prepare request; acceptors respond with accepted values
 * - Proposer sends Accept request with new proposal number; acceptors persist
 * - Learners collect majority acceptances; model state machine applies
 * - No single leader, but proposer with highest ballot number dominates
 *
 * @section conflict_resolution Conflict Resolution at Federation Boundary
 *
 * When writes arrive at different shards:
 * - Each shard applies consensus locally; version clock records shard_id + term
 * - Federation sync detects version conflicts via version vector comparison
 * - Resolution: Use existing last-write-wins (LWW) with tiebreaker (shard_id → term)
 * - Deterministic outcome: same conflict input → same winner
 *
 * @section failure_modes Failure Modes & Guarantees
 *
 * | Scenario | Raft | Paxos | Gossip | Recovery |
 * |----------|------|-------|---------|----------|
 * | Single node failure | Follower demoted; new leader elected | Proposer switches; learner waits | Node gossips last state | Automatic (new leader within election timeout) |
 * | Network partition | Split brain risk; quorum survives | Same | All nodes may have stale data | Manual intervention or tie-breaker policy |
 * | Leader crash | Automatic election | No leader, proposer switches | No impact | <30 sec to new leader |
 * | Byzantine fault (faulty node) | Not tolerated | Not tolerated | Tolerates <1/3 faulty | Depends on gossip strategy |
 *
 * @section rpc_interface RPC Interface for Consensus
 *
 * **Raft RPCs:**
 * - RequestVote(term, candidate_id, last_log_index, last_log_term) → (term, vote_granted)
 * - AppendEntries(term, leader_id, prev_log_index, prev_log_term, entries[], leader_commit) → (term, success)
 *
 * **Paxos RPCs:**
 * - Prepare(proposal_number) → (highest_accepted_number, accepted_value)
 * - Accept(proposal_number, value) → (success, promised_number)
 *
 * **General RPC Contract:**
 * - Timeout: 5 seconds (configurable); retry with exponential backoff
 * - Correlation ID: W3C Trace Context propagated in all RPC headers
 * - Failure: network error returns transient error; server-side rejection returns permanent
 *
 * @section performance_targets Performance Targets
 *
 * - Single-shard operations: ≤5% consensus overhead (backward compatibility)
 * - Leader election: <30 seconds (Raft/Paxos), eventual (Gossip)
 * - Log replication latency: <50ms P95 for quorum commit (3-node cluster, LAN)
 * - Consensus metadata: <1% storage overhead (vs. model size)
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.1; breaking changes require v3.0.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis::process {

// ============================================================================
// Consensus Type Enum
// ============================================================================

/**
 * @brief Supported consensus protocols for process federation.
 */
enum class ConsensusType : int32_t {
    /// Raft (leader-based, strongly consistent)
    RAFT = 6100,
    /// Paxos (quorum-based, strongly consistent)
    PAXOS = 6101,
    /// Gossip (leaderless, eventual consistency, Byzantine-tolerant)
    GOSSIP = 6102,
};

/**
 * @brief Consensus state for a node in a federated cluster.
 */
enum class ConsensusNodeState : int32_t {
    /// Node is following a leader (Raft/Paxos) or gossiping (all)
    FOLLOWER = 6110,
    /// Node is in election phase (Raft) or proposing (Paxos)
    CANDIDATE = 6111,
    /// Node is the current leader (Raft) or consensus acceptor (Paxos)
    LEADER = 6112,
    /// Node observes consensus but does not participate in voting
    OBSERVER = 6113,
    /// Node is offline or unresponsive
    UNREACHABLE = 6114,
};

// ============================================================================
// Replication Log Structures
// ============================================================================

/**
 * @brief Entry in the consensus replication log.
 *
 * Represents a single process model change (import, link, delete) replicated
 * across the cluster.
 */
struct ConsensusLogEntry {
    /// Logical clock term/epoch (monotonically increasing across leader elections)
    uint64_t term = 0;

    /// Index in the replication log (1-based)
    uint64_t index = 0;

    /// Timestamp of entry creation (UTC, nanoseconds since epoch)
    int64_t timestamp_ns = 0;

    /// Operation type: "import", "link", "delete", "config_change"
    std::string operation_type;

    /// Model ID affected by this operation
    std::string model_id;

    /// Shard ID where operation originated
    std::string shard_id;

    /// Cryptographic hash of the operation payload (for integrity verification)
    std::string payload_hash;

    /// Serialized operation payload (BPMN/CMMN content or link descriptor)
    std::string payload;

    /// Optional: commit term if entry has been committed (leader only)
    std::optional<uint64_t> commit_term;

    /**
     * @brief Serialize to JSON for RPC transmission.
     * @return JSON representation of this log entry.
     */
    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"term", term},
            {"index", index},
            {"timestamp_ns", timestamp_ns},
            {"operation_type", operation_type},
            {"model_id", model_id},
            {"shard_id", shard_id},
            {"payload_hash", payload_hash},
            {"payload", payload},
        };
        if (commit_term) {
            j["commit_term"] = *commit_term;
        }
        return j;
    }
};

/**
 * @brief Snapshot of process model state at a logical point in replication log.
 *
 * Used for log compaction and follower catch-up when leader log is ahead by many entries.
 */
struct ConsensusSnapshot {
    /// Term at which this snapshot was taken
    uint64_t snapshot_term = 0;

    /// Log index at which this snapshot was taken
    uint64_t snapshot_index = 0;

    /// Timestamp of snapshot creation
    int64_t timestamp_ns = 0;

    /// Serialized process model state (opaque to consensus layer)
    std::string model_state;

    /// Cryptographic hash of model_state (for integrity verification)
    std::string model_state_hash;

    /// List of shard IDs that have persisted this snapshot
    std::vector<std::string> persisted_shards;

    /**
     * @brief Check if this snapshot is valid (hash matches state).
     * @param computed_hash Hash computed by verifier
     * @return true if hashes match
     */
    bool isValid(const std::string& computed_hash) const noexcept {
        return model_state_hash == computed_hash;
    }
};

// ============================================================================
// Raft-Specific Structures
// ============================================================================

/**
 * @brief Vote request for Raft leader election.
 */
struct RaftVoteRequest {
    /// Election term (monotonically increasing)
    uint64_t term = 0;

    /// Shard ID of the candidate requesting vote
    std::string candidate_id;

    /// Index of candidate's last log entry
    uint64_t last_log_index = 0;

    /// Term of candidate's last log entry
    uint64_t last_log_term = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Vote response for Raft leader election.
 */
struct RaftVoteResponse {
    /// Current term of voter (to update candidate's term if needed)
    uint64_t term = 0;

    /// true if vote was granted to candidate
    bool vote_granted = false;

    /// Reason for denial (if vote_granted=false)
    std::string denial_reason;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Append entries request for Raft log replication.
 */
struct RaftAppendEntriesRequest {
    /// Leader's current term
    uint64_t term = 0;

    /// Shard ID of leader
    std::string leader_id;

    /// Index of log entry immediately preceding new entries
    uint64_t prev_log_index = 0;

    /// Term of log entry at prev_log_index
    uint64_t prev_log_term = 0;

    /// Log entries to replicate (may be empty for heartbeat)
    std::vector<ConsensusLogEntry> entries;

    /// Leader's commitIndex (hint for follower to apply entries)
    uint64_t leader_commit = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Append entries response for Raft log replication.
 */
struct RaftAppendEntriesResponse {
    /// Current term of follower (to update leader's term if needed)
    uint64_t term = 0;

    /// true if follower contained entry matching prev_log_index and prev_log_term
    bool success = false;

    /// Reason for failure (if success=false)
    std::string failure_reason;

    /// Highest index successfully replicated (for leader to know where to retry from)
    uint64_t match_index = 0;

    /// Correlation ID for tracing
    std::string trace_id;
};

// ============================================================================
// Paxos-Specific Structures
// ============================================================================

/**
 * @brief Paxos Prepare request (Phase 1: proposer promises new ballot).
 */
struct PaxosPrepareRequest {
    /// Proposal number (ballot) - unique per proposer
    uint64_t proposal_number = 0;

    /// Shard ID of proposer
    std::string proposer_id;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Paxos Prepare response (Phase 1: acceptor promises).
 */
struct PaxosPrepareResponse {
    /// Highest proposal number this acceptor has promised to honor
    uint64_t promised_number = 0;

    /// Highest accepted proposal number (if any accepted previous value)
    std::optional<uint64_t> accepted_number;

    /// Value accepted under accepted_number (if any)
    std::optional<ConsensusLogEntry> accepted_value;

    /// true if acceptor promises this proposal_number
    bool promise_granted = false;

    /// Denial reason if promise_granted=false
    std::string denial_reason;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Paxos Accept request (Phase 2: proposer sends value to acceptors).
 */
struct PaxosAcceptRequest {
    /// Proposal number (must match or exceed promised number)
    uint64_t proposal_number = 0;

    /// Shard ID of proposer
    std::string proposer_id;

    /// Log entry value to accept
    ConsensusLogEntry value;

    /// Correlation ID for tracing
    std::string trace_id;
};

/**
 * @brief Paxos Accept response (Phase 2: acceptor persists value).
 */
struct PaxosAcceptResponse {
    /// true if acceptor accepted this proposal
    bool accepted = false;

    /// Highest proposal number this acceptor has accepted
    uint64_t accepted_number = 0;

    /// Denial reason if accepted=false
    std::string denial_reason;

    /// Correlation ID for tracing
    std::string trace_id;
};

// ============================================================================
// Gossip-Specific Structures
// ============================================================================

/**
 * @brief Version vector for detecting causality and divergence in gossip protocol.
 *
 * Maps shard_id → logical clock value; used to detect which events have been seen.
 */
struct GossipVersionVector {
    /// Map of shard_id -> logical clock
    std::map<std::string, uint64_t> clock;

    /**
     * @brief Increment clock for a given shard.
     * @param shard_id Shard to increment
     */
    void increment(const std::string& shard_id) {
        if (clock.find(shard_id) == clock.end()) {
            clock[shard_id] = 1;
        } else {
            clock[shard_id]++;
        }
    }

    /**
     * @brief Merge with another version vector (take maximum per shard).
     * @param other Version vector to merge
     */
    void merge(const GossipVersionVector& other) {
        for (const auto& [shard_id, clock_value] : other.clock) {
            if (clock.find(shard_id) == clock.end()) {
                clock[shard_id] = clock_value;
            } else {
                clock[shard_id] = std::max(clock[shard_id], clock_value);
            }
        }
    }

    /**
     * @brief Check if this version vector has seen all events in another.
     * @param other Version vector to compare
     * @return true if this >= other (elementwise)
     */
    bool happensBefore(const GossipVersionVector& other) const {
        for (const auto& [shard_id, other_clock] : other.clock) {
            auto it = clock.find(shard_id);
            if (it == clock.end() || it->second < other_clock) {
                return true;  // Missing event or older clock
            }
        }
        return false;  // All events in other have been seen
    }
};

/**
 * @brief Gossip push/pull message for anti-entropy.
 */
struct GossipMessage {
    /// Sender shard ID
    std::string sender_id;

    /// Version vector of sender's knowledge
    GossipVersionVector version_vector;

    /// Log entries not yet seen by receiver
    std::vector<ConsensusLogEntry> entries;

    /// Correlation ID for tracing
    std::string trace_id;
};

// ============================================================================
// Split-Brain Detection & Recovery
// ============================================================================

/**
 * @brief Split-brain detection result.
 */
enum class SplitBrainState : int32_t {
    /// No split detected; cluster is unified
    UNIFIED = 6130,
    /// Split detected: two or more partitions with active leaders
    SPLIT_DETECTED = 6131,
    /// Recovery in progress: higher-term leader invalidating lower-term
    RECOVERY_IN_PROGRESS = 6132,
};

/**
 * @brief Configuration for split-brain detection and recovery.
 */
struct SplitBrainConfig {
    /// Maximum time to tolerate missing heartbeat before starting election (ms)
    uint32_t election_timeout_ms = 300;

    /// Maximum detection window for split-brain (ms)
    uint32_t detection_window_ms = 30000;

    /// Policy on split-brain detection: "fail_closed" (reject writes) or "last_write_wins"
    std::string recovery_policy = "fail_closed";

    /// true to enable automatic recovery; false for manual intervention
    bool auto_recover = true;

    /// Correlation ID for tracing
    std::string trace_id;
};

} // namespace themis::process
