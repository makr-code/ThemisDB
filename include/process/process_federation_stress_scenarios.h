/**
 * @file process_federation_stress_scenarios.h
 * @brief 12 federated stress scenarios for Phase 4 testing.
 *
 * Defines stress test scenarios covering consensus, conflict resolution,
 * incremental evolution, telemetry, and network failure edge cases.
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_1_DESIGN (Q1 2027)
 *
 * ## Stress Scenario Overview
 *
 * 12 scenarios across 5 categories, mapped to Phase 4 test suites:
 *
 * | Category | Count | Scenarios |
 * |----------|-------|-----------|
 * | Consensus | 2 | Byzantine consensus, partition healing |
 * | Conflict Resolution | 2 | Callback under high churn, multi-model conflicts |
 * | Incremental Evolution | 2 | Audit trail under churn, temporal reconstruction |
 * | Distributed Tracing | 2 | Trace completeness, correlation ID propagation |
 * | Network Failure | 4 | Partition detection, split-brain, cascading failures, re-join |
 *
 * All scenarios are deterministic (kFederationTestSeed = 42).
 *
 * @see ROADMAP_FEDERATION.md – Phase 1-6 implementation plan
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_STRESS_SCENARIOS_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_STRESS_SCENARIOS_H

#include <string>
#include <cstdint>
#include <vector>
#include <memory>

namespace themisdb::process::stress_scenarios {

// ============================================================================
// STRESS SCENARIO CONSTANTS
// ============================================================================

/// Canonical seed for deterministic scenario execution
constexpr std::uint32_t kFederationTestSeed = 42;

/// Number of replicas in base test cluster
constexpr std::uint32_t kBaseReplicaCount = 5;

/// High-churn operation count per scenario
constexpr std::uint32_t kHighChurnOperations = 500;

/// Network partition detection timeout (ms)
constexpr std::uint32_t kPartitionDetectionTimeoutMs = 1000;

// ============================================================================
// CONSENSUS SCENARIOS (2)
// ============================================================================

/**
 * @brief Consensus Scenario 1: Byzantine Node Under Quorum
 *
 * Tests Byzantine fault tolerance with F < N/3 constraint.
 *
 * **Setup:**
 * - 5-node cluster (N=5, F<2, tolerate 1 byzantine node)
 * - One node sends conflicting vote messages
 * - Four nodes act normally
 *
 * **Execution:**
 * - Initiate leader election
 * - Byzantine node sends conflicting votes to different candidates
 * - Normal nodes should still converge on single leader (quorum vote)
 *
 * **Verification:**
 * - Exactly one leader elected (no split-brain)
 * - Byzantine votes ignored (quorum voting ignores minority)
 * - All normal nodes have consistent log state
 *
 * **Expected P95:** <100ms election time
 */
class ByzantineConsensusScenario {
 public:
  static std::string Name() { return "ByzantineNodeUnderQuorum"; }
  static std::string Description() {
    return "Byzantine node sends conflicting votes; quorum prevails";
  }
};

/**
 * @brief Consensus Scenario 2: Network Partition and Healing
 *
 * Tests automatic detection and recovery from network partitions.
 *
 * **Setup:**
 * - 5-node cluster (majority: 3 nodes, minority: 2 nodes)
 * - Introduce network partition (message loss between groups)
 * - Run workload on both partitions
 * - Simulate partition healing
 *
 * **Execution:**
 * - Phase 1: Partition for 5 seconds; both partitions detect isolation
 * - Phase 2: Majority partition continues (new leader if needed); minority goes read-only
 * - Phase 3: Partition heals; messages flow again
 * - Phase 4: Minority rejoins; re-syncs log from majority
 *
 * **Verification:**
 * - Majority partition continues consensus (writes accepted)
 * - Minority partition rejects new writes (read-only)
 * - Both partitions detect partition correctly (<partition_detection_timeout_ms)
 * - After healing: consistent log state across all nodes
 * - No data loss during partition
 *
 * **Expected P95:** <500ms detection + re-join
 */
class NetworkPartitionHealingScenario {
 public:
  static std::string Name() { return "NetworkPartitionAndHealing"; }
  static std::string Description() {
    return "Partition for 5s, detect, continue majority, heal, re-sync";
  }
};

// ============================================================================
// CONFLICT RESOLUTION SCENARIOS (2)
// ============================================================================

/**
 * @brief Conflict Resolution Scenario 3: Callback Under High Churn
 *
 * Tests callback invocation and fallback under concurrent updates (>500 ops).
 *
 * **Setup:**
 * - 3-node cluster
 * - Application callback registered (custom conflict resolver)
 * - 500+ concurrent updates to same model version
 * - Some updates trigger conflicts
 *
 * **Execution:**
 * - Generate 500 concurrent model updates to same version
 * - Consensus layer detects conflicts (concurrent updates from different nodes)
 * - Invoke callback for each conflict
 * - Callback returns determistically (based on metadata)
 *
 * **Verification:**
 * - Callback invoked for each conflict detected
 * - All callbacks complete within 5s timeout
 * - Deterministic outcome: same metadata → same winner
 * - Conflict resolution latency <50ms (p95)
 * - No deadlock between consensus and callback
 *
 * **Expected P95:** <50ms per conflict resolution
 */
class CallbackUnderHighChurnScenario {
 public:
  static std::string Name() { return "CallbackUnderHighChurn"; }
  static std::string Description() {
    return "500+ concurrent updates trigger callbacks for conflict resolution";
  }
};

/**
 * @brief Conflict Resolution Scenario 4: Multi-Model Conflicts
 *
 * Tests simultaneous conflicts on multiple models.
 *
 * **Setup:**
 * - 5 models, each with 100 versions
 * - Concurrent updates to multiple models from different nodes
 * - Conflicts on 3+ models simultaneously
 *
 * **Execution:**
 * - Replica 1 updates models M1, M2, M3 at same timestamp
 * - Replica 2 updates same models at same timestamp (concurrent)
 * - Detect conflicts on all 3 models
 * - Invoke callbacks for each conflict independently
 *
 * **Verification:**
 * - All 3 conflicts resolved deterministically
 * - No interference between conflict resolutions
 * - Each model has unique winner (not cross-contaminated)
 * - Callback ordering consistent across replicas
 *
 * **Expected P95:** <100ms total for 3 simultaneous conflicts
 */
class MultiModelConflictScenario {
 public:
  static std::string Name() { return "MultiModelConflicts"; }
  static std::string Description() {
    return "Simultaneous conflicts on 3+ models; each resolved independently";
  }
};

// ============================================================================
// INCREMENTAL EVOLUTION SCENARIOS (2)
// ============================================================================

/**
 * @brief Incremental Evolution Scenario 5: Audit Trail Under High Churn
 *
 * Tests incremental evolution tracking with 500+ entries per second.
 *
 * **Setup:**
 * - 5-node cluster
 * - 10,000+ model mutations (audit trail entries)
 * - Concurrent reads and writes to audit trail
 *
 * **Execution:**
 * - Append 10,000 entries at high rate (500+/sec)
 * - Concurrently read random entries
 * - Verify CRC32 checksums for all entries
 * - Compute snapshot index
 *
 * **Verification:**
 * - All entries persisted and retrievable
 * - CRC32 chain valid for all entries
 * - Snapshot index correctly maps time ranges
 * - No gaps or duplicates in entry sequence
 * - Concurrent reads do not block writes
 *
 * **Expected P95:** <10ms per append, <1ms per read
 */
class AuditTrailHighChurnScenario {
 public:
  static std::string Name() { return "AuditTrailHighChurn"; }
  static std::string Description() {
    return "10K+ audit entries with concurrent reads and writes";
  }
};

/**
 * @brief Incremental Evolution Scenario 6: Temporal Reconstruction
 *
 * Tests point-in-time model reconstruction with large delta sequences.
 *
 * **Setup:**
 * - 1000+ deltas for single model
 * - Temporal queries at various historical points
 *
 * **Execution:**
 * - Create baseline model
 * - Apply 1000 sequential deltas (mutations)
 * - Query model state at 10 different historical timestamps
 * - Verify reconstructed models match expected state
 *
 * **Verification:**
 * - All reconstructions semantically equivalent to direct updates
 * - Delta composition property holds (no state drift)
 * - Reconstruction latency O(log N) for snapshot lookup
 * - Multiple concurrent temporal queries work correctly
 *
 * **Expected P95:** <100ms reconstruction (1K deltas)
 */
class TemporalReconstructionScenario {
 public:
  static std::string Name() { return "TemporalReconstruction"; }
  static std::string Description() {
    return "Reconstruct model at 10 historical timepoints; verify equivalence";
  }
};

// ============================================================================
// DISTRIBUTED TRACING SCENARIOS (2)
// ============================================================================

/**
 * @brief Distributed Tracing Scenario 7: Correlation ID Propagation
 *
 * Tests correlation ID flow across federation RPC boundaries.
 *
 * **Setup:**
 * - 5-node cluster
 * - Single federated write operation (consensus coordination)
 * - Trace context propagation through 4 RPC hops
 *
 * **Execution:**
 * - Generate correlation ID (UUID v4)
 * - Initiate write on node 1
 * - Track RPC calls: Node1→Leader→Followers (4+ hops)
 * - Collect traces from all nodes
 *
 * **Verification:**
 * - Same correlation ID in all span attributes
 * - Span hierarchy correct (root→parent→child)
 * - No correlation ID collisions
 * - W3C Trace Context serialization valid
 * - Telemetry overhead <5%
 *
 * **Expected P95:** <5% overhead over untraced operation
 */
class CorrelationIdPropagationScenario {
 public:
  static std::string Name() { return "CorrelationIdPropagation"; }
  static std::string Description() {
    return "Correlation ID flows through 4+ RPC hops; collected in single trace";
  }
};

/**
 * @brief Distributed Tracing Scenario 8: Trace Completeness Under Partition
 *
 * Tests trace collection under network partition (some spans may be delayed).
 *
 * **Setup:**
 * - 5-node cluster
 * - Write operation that spans majority and minority partitions
 * - Partition introduces for 5 seconds
 *
 * **Execution:**
 * - Initiate write that would normally span all 5 nodes
 * - After 2 seconds: introduce partition
 * - Continue operation (traces emitted from both partitions)
 * - Recover partition; collect all traces
 *
 * **Verification:**
 * - Traces from majority partition complete immediately
 * - Traces from minority partition delayed by partition duration
 * - All traces have same correlation ID
 * - Root span has complete child span list
 * - No trace duplication or corruption
 *
 * **Expected P95:** <100ms for majority; <5s for minority after healing
 */
class TraceCompletenessPartitionScenario {
 public:
  static std::string Name() { return "TraceCompletenessUnderPartition"; }
  static std::string Description() {
    return "Traces from majority partition complete; minority delayed; all collected";
  }
};

// ============================================================================
// NETWORK FAILURE SCENARIOS (4)
// ============================================================================

/**
 * @brief Network Failure Scenario 9: Partition Detection Latency
 *
 * Tests time to detect network partition from normal operation.
 *
 * **Setup:**
 * - 5-node cluster running steady state
 * - At T=0, introduce message loss from leader to follower node
 *
 * **Execution:**
 * - Leader sends heartbeat every 50ms (normally)
 * - At T=0: follower stops receiving heartbeats
 * - Measure time until follower detects partition
 * - Should trigger after heartbeat_timeout (configurable)
 *
 * **Verification:**
 * - Partition detected within consensus_timeout_ms
 * - Follower transitions to degraded mode (read-only)
 * - Leader may detect follower unresponsive and remove from quorum
 * - No false positives (transient delays do not trigger detection)
 *
 * **Expected P95:** <1 second detection time
 */
class PartitionDetectionLatencyScenario {
 public:
  static std::string Name() { return "PartitionDetectionLatency"; }
  static std::string Description() {
    return "Measure time to detect network partition (heartbeat timeout)";
  }
};

/**
 * @brief Network Failure Scenario 10: Split-Brain Prevention
 *
 * Tests that minority partition cannot operate (write rejection).
 *
 * **Setup:**
 * - 5-node cluster (majority: 3, minority: 2)
 * - Introduce partition
 *
 * **Execution:**
 * - Majority partition elects leader normally
 * - Minority partition attempts to elect leader
 * - Minority cannot achieve quorum (2 < 3)
 * - Attempt write to minority partition
 *
 * **Verification:**
 * - Majority partition accepts writes
 * - Minority partition rejects writes (quorum unavailable)
 * - No split-brain (two leaders claiming authority)
 * - No data divergence
 *
 * **Expected:** Minority write rejected immediately
 */
class SplitBrainPreventionScenario {
 public:
  static std::string Name() { return "SplitBrainPrevention"; }
  static std::string Description() {
    return "Minority partition (2/5 nodes) cannot elect leader; writes rejected";
  }
};

/**
 * @brief Network Failure Scenario 11: Cascading Failures (F < N/3)
 *
 * Tests behavior when multiple nodes fail simultaneously (respects F < N/3).
 *
 * **Setup:**
 * - 5-node cluster (tolerate F<2, max 1 failure)
 * - Simulate 2 node failures
 *
 * **Execution:**
 * - Node failure 1: remaining 4 nodes can still achieve quorum (2/3 of 3 needed)
 * - Node failure 2: remaining 3 nodes exactly at quorum boundary
 * - Attempt additional writes
 * - If 3rd node fails: quorum lost, no more writes
 *
 * **Verification:**
 * - After 1 failure: cluster operational
 * - After 2 failures: cluster operational (barely)
 * - After 3+ failures: cluster read-only (quorum lost)
 * - No data loss during failures
 *
 * **Expected:** Cluster remains operational until quorum lost
 */
class CascadingFailuresScenario {
 public:
  static std::string Name() { return "CascadingFailures"; }
  static std::string Description() {
    return "Simulate 2-3 node failures; verify F < N/3 constraints respected";
  }
};

/**
 * @brief Network Failure Scenario 12: Automatic Re-Join and Log Sync
 *
 * Tests log synchronization when partition heals.
 *
 * **Setup:**
 * - 5-node cluster
 * - Introduce partition for 10 seconds
 * - Majority continues to commit entries; minority goes stale
 * - Heal partition
 *
 * **Execution:**
 * - During partition (10s): majority applies 100 new entries (majority-only log)
 * - Minority has stale log
 * - Heal partition: minority rejoins
 * - Minority fetches missing entries from majority (log sync)
 * - Minority catches up and resumes normal operation
 *
 * **Verification:**
 * - Minority can catch up (no unbounded divergence)
 * - Log sync completes within reasonable time (<5s for 100 entries)
 * - All nodes have identical final log state
 * - No data loss or corruption during re-join
 *
 * **Expected P95:** <5 seconds for 100-entry log sync
 */
class RejoinLogSyncScenario {
 public:
  static std::string Name() { return "RejoinLogSync"; }
  static std::string Description() {
    return "Partition heals; minority catches up via log sync; verify consistency";
  }
};

// ============================================================================
// SCENARIO FACTORY (Phase 4 Test Infrastructure)
// ============================================================================

/**
 * @brief Base class for all stress scenarios.
 *
 * Implementations in Phase 4 test files.
 */
class FederationStressScenario {
 public:
  /// Name of scenario (for test registration)
  virtual std::string Name() const = 0;

  /// Human-readable description
  virtual std::string Description() const = 0;

  /// Setup phase (initialize cluster, apply initial state)
  virtual void Setup() = 0;

  /// Execute phase (run scenario, collect results)
  virtual void Execute() = 0;

  /// Verify phase (check invariants, assertions)
  virtual void Verify() = 0;

  /// Cleanup phase (tear down resources)
  virtual void Cleanup() = 0;

  virtual ~FederationStressScenario() = default;
};

/// Vector of all 12 stress scenarios for Phase 4 test registration
std::vector<std::shared_ptr<FederationStressScenario>> GetAllStressScenarios();

}  // namespace themisdb::process::stress_scenarios

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_FEDERATION_STRESS_SCENARIOS_H
