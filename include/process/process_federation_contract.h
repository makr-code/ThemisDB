/**
 * @file process_federation_contract.h
 * @brief Federated concurrency and replication contract for distributed Process Module deployments.
 *
 * Defines API contracts for federated consensus, quorum-based voting, Byzantine fault tolerance,
 * and replication consistency guarantees in multi-node Process Module deployments.
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_1_DESIGN (Q1 2027)
 *
 * ## Overview
 *
 * The federation contract specifies thread-safety guarantees, consensus semantics, and
 * replication consistency for Process Module deployments across multiple nodes/shards.
 *
 * ## Consensus Model
 *
 * - **Algorithm:** Raft-inspired quorum-based voting (configurable quorum size)
 * - **Fault Tolerance:** Byzantine fault tolerant for F failures where N > 3F (e.g., 5 nodes tolerate 1, 7 tolerate 2)
 * - **Message Ordering:** Total ordering via logical clocks on all replicas
 * - **Determinism:** Consensus outcome deterministic given same failure pattern and messages
 *
 * ## Thread-Safety Guarantees
 *
 * ### Concurrency Patterns
 * 1. **Snapshot Isolation (Model Manager)** – Readers do not block writers; writers see consistent snapshots
 * 2. **Fine-Grained Locking (Linker)** – Per-link atomicity; independent links do not block each other
 * 3. **Stateless Serializers** – Fully thread-safe; no per-instance state
 * 4. **Replicated State Machine** – Single writer (leader); multiple readers (followers)
 *
 * ### Lock Ordering (Prevention of Deadlock)
 * ```
 * consensus_mutex_ → replica_mutex_ → log_mutex_ → audit_mutex_
 * ```
 * All code must respect this ordering; no circular acquisition.
 *
 * ## Replication Consistency Guarantees
 *
 * | Consistency Level | Behavior | Use Case |
 * |---|---|---|
 * | **Strong Consistency** | Writes wait for majority quorum | Critical updates (model deletes, state transitions) |
 * | **Eventual Consistency** | Writes complete on leader; async replication | Reads, non-critical updates |
 * | **Snapshot Isolation** | Readers see consistent historical snapshot | Report generation, auditing |
 *
 * ## Network Partition Handling
 *
 * - **Majority Partition:** Continues consensus and accepts writes
 * - **Minority Partition:** Fails-closed to read-only degraded mode; no writes accepted
 * - **Split-Brain Prevention:** Quorum-based voting prevents minority from claiming leadership
 * - **Automatic Healing:** Re-join when partition heals (log sync + catch-up replication)
 *
 * ## Error Handling & Failure Modes
 *
 * ### Fail-Closed Behaviors
 * - Network partition → read-only degraded mode (no data loss)
 * - Replica divergence → automatic re-sync from leader
 * - Consensus timeout → return error; application may retry with exponential backoff
 * - Node failure → automatic leader re-election (if quorum available)
 *
 * ### Byzantine Tolerance
 * - Tolerate up to F faulty nodes where F < N/3
 * - Faulty node can send conflicting messages, but cannot violate safety invariants
 * - Majority must agree on log entries before commit
 *
 * ## Usage Pattern
 *
 * @code
 * // Federated Process Module deployment
 * ProcessFederationConfig config;
 * config.node_id = "node-1";
 * config.quorum_size = 3;  // Deploy with 3+ replicas for fault tolerance
 * config.network_latency_ms = 5;  // Intra-DC assumption
 *
 * auto federation_mgr = ProcessFederationManager::Create(config);
 *
 * // Single-shard code unchanged; federation transparent to application
 * ProcessModelManager model_mgr = federation_mgr->GetModelManager();
 * @endcode
 *
 * ## Production Deployment Requirements
 *
 * 1. **Quorum Configuration:** Minimum 3 nodes (F=1); prefer 5+ for higher availability
 * 2. **Network Assumptions:** Intra-DC latency <5ms; inter-DC latencies documented
 * 3. **Storage Backends:** RocksDB (primary), S3 (backup), file-based (local dev)
 * 4. **Monitoring:** Track consensus latency, replication lag, partition detection time
 *
 * ## Backward Compatibility
 *
 * - v2.x single-shard Process Module API **unchanged**
 * - Federation is **opt-in** (default: 1 replica = single-shard mode)
 * - Existing applications continue to work without federation awareness
 *
 * ## Limitations & Future Work
 *
 * - Cross-federation consensus (multi-shard voting across data centers) deferred to Q2 2027
 * - Lock-free data structures for ultra-high-contention scenarios deferred to Q2 2027
 * - ML-based anomaly detection deferred to Q2 2027
 *
 * @see process_conflict_resolution_callback.h – Conflict resolution strategies
 * @see process_incremental_evolution.h – Audit trail and temporal queries
 * @see process_telemetry_contract.h – Distributed tracing integration
 * @see ROADMAP_FEDERATION.md – Phase 1-6 implementation plan
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_CONTRACT_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_CONTRACT_H

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

namespace themisdb::process {

// ============================================================================
// FEDERATION CONFIGURATION
// ============================================================================

/**
 * @brief Configuration for federated Process Module deployment.
 *
 * @note All members must be set before passing to ProcessFederationManager::Create.
 */
struct ProcessFederationConfig {
  /// Node ID (unique identifier for this replica)
  std::string node_id;

  /// Quorum size (must satisfy N > 3F for Byzantine tolerance)
  std::uint32_t quorum_size = 3;

  /// List of peer node addresses (e.g., "node-2:5000", "node-3:5000")
  std::vector<std::string> peer_addresses;

  /// Network latency assumption for consensus timeout calculation (ms)
  std::chrono::milliseconds network_latency_ms{5};

  /// Consensus timeout (failover to new election if no heartbeat)
  std::chrono::milliseconds consensus_timeout_ms{1000};

  /// Audit trail storage backend ("rocksdb", "file", "s3")
  std::string audit_backend = "rocksdb";

  /// Max replication lag before marking replica as stale (ms)
  std::chrono::milliseconds max_replication_lag_ms{5000};
};

// ============================================================================
// CONSENSUS MODEL & FAILURE MODES
// ============================================================================

/**
 * @brief Consensus state machine states.
 *
 * @section consensus_states State Transitions
 *
 * | From | To | Condition |
 * |------|----|----|
 * | FOLLOWER | CANDIDATE | Heartbeat timeout |
 * | CANDIDATE | LEADER | Won majority vote |
 * | CANDIDATE | FOLLOWER | Discovered leader or lost vote |
 * | LEADER | FOLLOWER | Discovered higher term |
 * | ANY | FAILED | Critical error (consensus corrupted) |
 */
enum class ConsensusState : std::uint8_t {
  kFollower = 0,         ///< Replica following leader's log
  kCandidate = 1,        ///< Replica requesting votes for leadership
  kLeader = 2,           ///< Replica has achieved quorum consensus
  kFailed = 3,           ///< Unrecoverable consensus failure
};

/**
 * @brief Consensus phase enumeration (for span attributes and diagnostics).
 */
enum class ConsensusPhase : std::uint8_t {
  kIdle = 0,                      ///< Waiting for write operation
  kLeaderElection = 1,            ///< Voting phase
  kLogReplication = 2,            ///< Sending entries to followers
  kCommit = 3,                    ///< Applying to state machine
};

/**
 * @brief Byzantine fault tolerance constraints.
 *
 * For N total nodes, maximum tolerable failures: F < N/3
 * - 3 nodes: F < 1 (tolerate 0 failures)
 * - 5 nodes: F < 1.67 (tolerate 1 failure)
 * - 7 nodes: F < 2.33 (tolerate 2 failures)
 */
struct ByzantineFaultToleranceGates {
  static constexpr std::uint32_t kMinNodeCount = 3;     ///< Minimum for consensus
  static constexpr std::uint32_t kRecommendedNodeCount = 5;  ///< Recommended for single-DC
  static constexpr float kFaultToleranceRatio = 1.0f / 3.0f;  ///< F < N/3
};

// ============================================================================
// THREAD-SAFETY PATTERNS
// ============================================================================

/**
 * @brief Thread-safety guarantee patterns for federation-aware APIs.
 */
enum class ConcurrencyPattern : std::uint8_t {
  /// Snapshot isolation: readers do not block writers
  kSnapshotIsolation = 0,

  /// Fine-grained locking: per-entity atomicity
  kFineGrainedLocking = 1,

  /// Stateless: no per-instance state (fully thread-safe)
  kStateless = 2,

  /// Replicated state machine: single writer (leader), multiple readers
  kReplicatedStateMachine = 3,
};

/**
 * @brief Thread-safety guarantee per component.
 *
 * @section thread_safety_table Component Guarantees
 *
 * | Component | Pattern | Details |
 * |---|---|---|
 * | ProcessModelManager | Snapshot Isolation | Readers see consistent snapshots across replicas |
 * | ProcessLinker | Fine-Grained Locking | Per-link atomicity; independent links don't block |
 * | BpmnSerializer | Stateless | Fully thread-safe; no shared state |
 * | FederationConsensusManager | Replicated State Machine | Single leader writer; followers read-only |
 * | ProcessAuditLogger | Fine-Grained Locking | Per-log-entry atomicity |
 *
 * @invariant All locks acquired in order: consensus_mutex_ → replica_mutex_ → log_mutex_
 * @invariant No thread holds more than 2 locks simultaneously
 * @invariant No deadlock: acyclic lock dependency graph
 */
struct ThreadSafetyGuarantee {
  /// Concurrency pattern used
  ConcurrencyPattern pattern;

  /// Maximum concurrent readers allowed (-1 = unlimited)
  std::int32_t max_concurrent_readers = -1;

  /// Whether operation is atomic across replicas
  bool is_replicated_atomic = false;

  /// Consistency level (strong, eventual, snapshot)
  std::string consistency_level;
};

// ============================================================================
// REPLICATION CONSISTENCY
// ============================================================================

/**
 * @brief Consistency level for write operations.
 */
enum class ConsistencyLevel : std::uint8_t {
  /// Writes wait for majority quorum; highest safety
  kStrong = 0,

  /// Writes complete on leader; async replication to followers
  kEventual = 1,

  /// Writes complete on leader; no replication (local-only)
  kLocal = 2,
};

/**
 * @brief Replication status for a given operation.
 *
 * @note Thread-safe for concurrent reads; single writer updates status.
 */
struct ReplicationStatus {
  /// Current replication state (completed, in_progress, failed)
  std::string state;

  /// Nodes that have replicated this operation
  std::vector<std::string> replicated_on;

  /// Nodes that have acknowledged but not yet persisted
  std::vector<std::string> acknowledged_by;

  /// Quorum achieved for this operation?
  bool quorum_achieved = false;

  /// Timestamp of replication completion
  std::chrono::system_clock::time_point completion_time;
};

// ============================================================================
// NETWORK PARTITION HANDLING
// ============================================================================

/**
 * @brief Partition detection result.
 *
 * Returned by FederationConsensusManager::DetectPartition().
 */
struct PartitionDetectionResult {
  /// Is this node in minority or majority partition?
  enum class PartitionRole : std::uint8_t {
    kMajority = 0,  ///< Part of quorum; can operate
    kMinority = 1,  ///< Part of split; read-only degraded
    kIsolated = 2,  ///< No quorum available; read-only
  } role;

  /// Nodes currently reachable (for majority partition)
  std::vector<std::string> reachable_nodes;

  /// Nodes currently unreachable (for minority partition)
  std::vector<std::string> unreachable_nodes;

  /// Confidence in partition detection (0.0 - 1.0)
  float confidence = 0.0f;

  /// Timestamp of detection
  std::chrono::system_clock::time_point detection_time;
};

// ============================================================================
// API DECLARATIONS (Skeleton for Phase 2 Implementation)
// ============================================================================

/**
 * @brief Federation consensus manager.
 *
 * @note Actual implementation in src/process/federation_consensus_manager.cpp
 *
 * @section usage_example Usage Example
 * @code
 * ProcessFederationConfig config;
 * config.node_id = "node-1";
 * config.quorum_size = 3;
 * config.peer_addresses = {"node-2:5000", "node-3:5000"};
 *
 * auto consensus_mgr = std::make_shared<FederationConsensusManager>(config);
 * auto state = consensus_mgr->GetCurrentState();  // kFollower initially
 * @endcode
 */
class FederationConsensusManager {
 public:
  /// Create federation consensus manager (Phase 2 implementation)
  explicit FederationConsensusManager(const ProcessFederationConfig& config);

  /// Get current consensus state
  /// @return Current state (follower, candidate, leader, failed)
  virtual ConsensusState GetCurrentState() const = 0;

  /// Detect network partition
  /// @return Partition detection result with role and reachable nodes
  virtual PartitionDetectionResult DetectPartition() const = 0;

  /// Attempt write with given consistency level
  /// @param data Operation data (Phase 2 will specify format)
  /// @param consistency Consistency level (strong, eventual, local)
  /// @return Status (success, timeout, partition_detected, failed)
  virtual std::string AttemptWrite(
      const std::string& data,
      ConsistencyLevel consistency) = 0;

  /// Get replication status for operation
  /// @param operation_id Unique operation ID
  /// @return Replication status (state, replicated_on, quorum_achieved)
  virtual ReplicationStatus GetReplicationStatus(
      const std::string& operation_id) const = 0;

  virtual ~FederationConsensusManager() = default;

 protected:
  ProcessFederationConfig config_;
  ConsensusState current_state_ = ConsensusState::kFollower;
};

// ============================================================================
// DOCUMENTATION SECTIONS (Design-Phase Specifications)
// ============================================================================

/**
 * @section design_constraints Design Constraints
 *
 * 1. **Consensus Algorithm:** Raft-inspired with quorum-based voting
 *    - Leader election via majority vote (timeout-based)
 *    - Log replication with strong consistency on majority
 *    - Automatic re-election if leader fails
 *
 * 2. **Byzantine Fault Tolerance:** F < N/3 constraint
 *    - Up to F faulty nodes can be tolerated in N-node cluster
 *    - Faulty nodes cannot violate safety invariants
 *    - Consensus outcome determined by quorum agreement
 *
 * 3. **Message Ordering:** Total order delivery
 *    - All replicas apply log entries in same order
 *    - Logical clocks ensure deterministic ordering
 *    - Out-of-order messages detected and re-ordered
 *
 * 4. **Determinism:** Consensus outcome deterministic
 *    - Same failure pattern + messages → same leader elected
 *    - Tie-breaking by node ID for deterministic ordering
 *    - No randomness in vote counting (only in election timeout randomization)
 *
 * @section performance_expectations Performance Expectations
 *
 * | Operation | P95 | P99 | Budget |
 * |-----------|-----|-----|--------|
 * | Leader election (3-node) | 50ms | 200ms | ≤100ms |
 * | Log replication (1K entries) | 100ms | 500ms | ≤200ms |
 * | Snapshot transfer (100MB) | 500ms | 1s | ≤1s |
 * | Heartbeat latency | 5ms | 10ms | ≤20ms |
 * | Partition detection | 100ms | 500ms | ≤1s |
 *
 * @section backward_compatibility Backward Compatibility
 *
 * - v2.x single-shard Process Module: **No changes**
 * - Federation is **opt-in** (default: 1 replica)
 * - Existing applications work unchanged
 * - API additions only; no existing API modifications
 */

}  // namespace themisdb::process

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_CONTRACT_H
