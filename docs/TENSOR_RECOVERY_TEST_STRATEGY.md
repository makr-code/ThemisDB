# Tensor Recovery Test Strategy

**Status:** Test Planning Document  
**Date:** 2026-07-05  
**Related Issue:** makr-code/ThemisDB#5433

---

## Purpose

Define comprehensive test strategy for distributed tensor artifact recovery, rebuild, and crash-safe procedures. Tests validate that all documented recovery paths work correctly under realistic failure conditions.

---

## 1. Test Categories and Scope

### 1.1 Recovery Path Tests

**Objective**: Verify that each documented failure scenario → recovery action path works correctly.

#### Test Suite A: Single Shard Loss

| Test Case | Setup | Inject Failure | Expected Outcome |
|-----------|-------|---|---|
| `test_replica_available_loss` | Replicated artifact on 2 shards | Shard 1 unavailable | Use replica from Shard 2 |
| `test_no_replica_loss` | Single-copy artifact | Shard unavailable | Trigger rebuild from lineage |
| `test_multi_replica_loss` | 3-copy artifact | 2 shards down | Rebuild from 1 remaining replica |
| `test_cascade_loss` | 3-copy artifact | Shard 1 → Shard 2 → Shard 3 lost in sequence | Handle each loss independently |

#### Test Suite B: Corruption Detection

| Test Case | Setup | Inject Failure | Expected Outcome |
|-----------|-------|---|---|
| `test_corruption_checksum_fail` | Artifact with hash | Flip bits in artifact | Hash verification fails; rebuild |
| `test_partial_corruption` | Artifact split across shards | Corrupt 1 of 3 shards | Use healthy shards, rebuild missing |
| `test_manifest_mismatch` | Artifact with versioned manifest | Update manifest but not data | Mismatch detected; rebuild data |
| `test_integrity_receipt_fail` | Artifact with integrity signature | Break signature | Receipt validation fails; rebuild |

#### Test Suite C: Staleness Handling

| Test Case | Setup | Inject Failure | Expected Outcome |
|-----------|-------|---|---|
| `test_stale_shard_summary` | Summary with age | No refresh for >age_threshold | Summary invalidated; rebuild triggered |
| `test_delta_lag_accumulation` | Update queue with pending deltas | Queue depth exceeds threshold | Trigger apply or invalidate |
| `test_freshness_debt_yellow` | Multiple stale artifacts | Cumulative staleness hits 5% | Log warning; schedule refresh |
| `test_freshness_debt_red` | Multiple stale artifacts | Cumulative staleness hits 20% | Trigger forced invalidation |

#### Test Suite D: Incompatibility Failures

| Test Case | Setup | Inject Failure | Expected Outcome |
|-----------|-------|---|---|
| `test_model_switch_adapter_compat` | Adapter for LLM v1 | Switch to LLM v2 | Adapter incompatible; re-materialize |
| `test_format_version_mismatch` | Artifact in old format | Try to read with new reader | Format version mismatch; rebuild |
| `test_rank_cap_breach` | Tensor with rank R | Try to store on shard with max R-1 | Rank exceeds; rebalance placement |

#### Test Suite E: Worker Failures

| Test Case | Setup | Inject Failure | Expected Outcome |
|-----------|-------|---|---|
| `test_crash_during_update` | Update in progress | Worker crash mid-update | WAL recovery; resume or abort |
| `test_crash_during_rebuild` | Rebuild in progress | Worker crash at 50% | Resume from checkpoint or restart |
| `test_crash_during_gc` | GC/eviction in progress | Worker crash | Rollback incomplete evictions |
| `test_orphan_artifact_detection` | Artifact referenced only by crashed worker | Worker never comes back | Orphan detector marks artifact |

### 1.2 Rebuild Mode Transition Tests

**Objective**: Verify mode transitions work correctly under different conditions.

#### Test Suite F: Patch Mode

- `test_patch_small_delta`: Apply small (<1MB) delta successfully
- `test_patch_large_delta`: Apply large (>100MB) delta with checkpointing
- `test_patch_delta_coalesce`: Multiple deltas coalesced before application
- `test_patch_invalid_delta`: Delta doesn't apply cleanly → upgrade to snapshot
- `test_patch_idempotent`: Apply same patch twice (should be idempotent)

#### Test Suite G: Partial Refit Mode

- `test_partial_refit_subset`: Refit 30% of factors, keep 70%
- `test_partial_refit_checkpoint`: Resume from checkpoint after crash
- `test_partial_refit_validation`: Verify refitted factors maintain compatibility
- `test_partial_refit_not_viable`: Missing source factors → upgrade to snapshot
- `test_partial_refit_all_factors`: Refit 100% (equivalent to snapshot)

#### Test Suite H: Snapshot Rebuild Mode

- `test_snapshot_from_replica`: Rebuild by copying healthy replica
- `test_snapshot_from_exact_source`: Rebuild from exact artifact source
- `test_snapshot_from_lineage`: Rebuild full artifact from package lineage
- `test_snapshot_checkpoint_resume`: Resume from checkpoint after crash
- `test_snapshot_validation`: Verify hash matches expected value
- `test_snapshot_no_source_available`: No source → graceful degradation

#### Test Suite I: Invalidate+Remat Mode

- `test_invalidate_lazy_remat`: Mark invalid; remat on next query
- `test_invalidate_on_model_switch`: Invalidate adapters on LLM switch
- `test_invalidate_concurrent_access`: Multiple queries during remat
- `test_invalidate_timeout`: Remat doesn't complete in time → use fallback

### 1.3 Crash Recovery Tests

**Objective**: Verify crash safety, idempotency, and state recovery.

#### Test Suite J: WAL Replay

- `test_replay_update_committed`: Committed update replayed successfully
- `test_replay_update_uncommitted`: Uncommitted update resumed correctly
- `test_replay_rebuild_committed`: Committed rebuild verified and completed
- `test_replay_rebuild_uncommitted`: Uncommitted rebuild resumed from checkpoint
- `test_replay_gc_committed`: GC already completed; skip
- `test_replay_gc_uncommitted`: Incomplete GC rolled back; artifacts restored
- `test_replay_out_of_order`: WAL entries in unexpected order; handled correctly

#### Test Suite K: Idempotency

- `test_idempotent_update_resume`: Resume same update twice (shouldn't apply twice)
- `test_idempotent_rebuild_resume`: Resume same rebuild twice
- `test_idempotent_manifest_update`: Multiple manifest updates with same version
- `test_version_check_prevents_double_apply`: Manifest version check prevents re-application

#### Test Suite L: Multi-Worker Scenarios

- `test_concurrent_recovery_same_artifact`: Two workers try to recover same artifact → serialize
- `test_conflicting_manifest_updates`: Concurrent manifest updates → conflict resolution
- `test_replica_consistency_after_recovery`: Replicas synchronized post-recovery
- `test_distributed_lock_coordination`: Lock manager handles recovery coordination

### 1.4 Freshness Debt Tests

**Objective**: Verify freshness debt accumulation, thresholds, and triggering.

#### Test Suite M: Debt Tracking

- `test_debt_accumulation`: Staleness accumulates over time
- `test_debt_multiple_artifacts`: Multiple stale artifacts contribute to debt
- `test_debt_linear_growth`: Debt grows linearly with staleness age
- `test_debt_reset_after_refresh`: Debt resets when artifact refreshed

#### Test Suite N: Threshold Triggers

- `test_yellow_threshold_warning`: Debt 5% triggers warning
- `test_red_threshold_action`: Debt 20% triggers invalidation
- `test_yellow_to_red_transition`: Debt crosses from yellow to red
- `test_red_recovery`: Debt reduced by invalidation; back to yellow

#### Test Suite O: Amplification Metrics

- `test_rebuild_amplification_replication`: Track replication overhead
- `test_rebuild_amplification_network`: Track network transfer cost
- `test_rebuild_amplification_computation`: Track CPU/GPU recomputation cost
- `test_total_amplification`: Product of factors calculated correctly

### 1.5 Edge Cases and Error Handling

#### Test Suite P: Partial Failures

- `test_partial_lineage_available`: Source package partially lost; rebuild from available subset
- `test_partial_replica_corruption`: Some replicas corrupted, others healthy
- `test_partial_factor_loss`: Some factors available, others missing
- `test_partial_manifest_corruption`: Manifest partially unreadable

#### Test Suite Q: Extreme Conditions

- `test_simultaneous_multi_shard_loss`: >1 shard lost simultaneously
- `test_cascading_failures`: Failure during recovery triggers another failure
- `test_resource_exhaustion`: Recovery stalled due to storage/memory limits
- `test_network_partition`: Workers isolated during recovery

#### Test Suite R: Fallback Mechanisms

- `test_fallback_to_graph_validation`: Tensor unavailable; use graph-only query
- `test_fallback_to_base_llm`: Adapter unavailable; use base LLM
- `test_fallback_to_stale_artifact`: No fresh replica; accept stale artifact
- `test_fallback_chain`: Multiple fallbacks applied in sequence

---

## 2. Test Data and Fixtures

### 2.1 Artifact Types for Testing

```cpp
// Small primary tensor (testing basic paths)
struct SmallLoRAFixture {
  size_t size = 1MB;
  int replication_factor = 2;
  vector<Shard> shards;
  ArtifactManifest manifest;
};

// Large primary tensor (testing scaling)
struct LargeLoRAFixture {
  size_t size = 1GB;
  int replication_factor = 3;
  vector<Shard> shards;
  ArtifactManifest manifest;
  // Includes factorization with 4 components
};

// Derived tensor (testing rebuild from exact)
struct SummaryCacheFixture {
  size_t size = 10MB;
  vector<Shard> shards;
  ArtifactId source_artifact;  // What it's derived from
  ArtifactManifest manifest;
};

// Adapter for testing model compatibility
struct AdapterFixture {
  LLMVersion compatible_version = v2;
  size_t size = 500MB;
  ArtifactManifest manifest;
  LoRASourcePackageId source_package;
};
```

### 2.2 Failure Injection Utilities

```cpp
// Shard failure injection
class ShardFailureSimulator {
public:
  // Make shard appear unavailable
  void SimulateShardDown(ShardId id);
  
  // Corrupt artifact bytes on shard
  void CorruptArtifactBytes(ShardId id, ArtifactId artifact,
                            int num_bytes);
  
  // Introduce network latency
  void AddNetworkLatency(ShardId id, Duration latency);
  
  // Block shard for duration
  void BlockShardTemporarily(ShardId id, Duration duration);
};

// Worker crash simulation
class WorkerCrashSimulator {
public:
  // Crash worker mid-operation
  void CrashDuringUpdate(WorkerId worker, ArtifactId artifact);
  void CrashDuringRebuild(WorkerId worker, ArtifactId artifact);
  void CrashDuringGC(WorkerId worker);
  
  // Restart worker from saved WAL
  Future<void> RestartWorker(WorkerId worker);
};

// Manifest/integrity failure
class ManifestFailureSimulator {
public:
  // Break manifest hash
  void BreakManifestHash(ArtifactId artifact);
  
  // Create version mismatch
  void CreateVersionMismatch(ArtifactId artifact);
  
  // Add stale timestamp
  void MakeArtifactStale(ArtifactId artifact, Duration age);
};
```

### 2.3 Verification Utilities

```cpp
class RecoveryVerifier {
public:
  // Verify artifact integrity post-recovery
  bool VerifyArtifactIntegrity(ArtifactId id);
  
  // Check manifest consistency
  bool VerifyManifestConsistency(ArtifactId id);
  
  // Verify replica consistency
  bool VerifyReplicaConsistency(ArtifactId id,
                                vector<ShardId> replicas);
  
  // Check recovery metrics
  RecoveryMetrics GetRecoveryMetrics(ArtifactId id);
  
  // Trace recovery operations
  vector<RecoveryOp> GetRecoveryTrace(ArtifactId id);
};
```

---

## 3. Test Implementation Roadmap

### Phase 1: Recovery Path Tests (Weeks 1-2)

**Tests**: A, B, C (basic scenarios)

```bash
bazel test //tests/epic3_distributed_tensor:recovery_paths_test
```

**Expected Coverage**: ~60% of test cases

### Phase 2: Rebuild Mode Tests (Weeks 2-3)

**Tests**: F, G, H, I (all rebuild modes)

```bash
bazel test //tests/epic3_distributed_tensor:rebuild_modes_test
```

**Expected Coverage**: ~80% of test cases

### Phase 3: Crash Recovery Tests (Weeks 3-4)

**Tests**: J, K, L (crash-safe mechanisms)

```bash
bazel test //tests/epic3_distributed_tensor:crash_recovery_test
```

**Expected Coverage**: ~90% of test cases

### Phase 4: Freshness & Edge Cases (Weeks 4-5)

**Tests**: M, N, O, P, Q, R (advanced scenarios)

```bash
bazel test //tests/epic3_distributed_tensor:freshness_and_edges_test
```

**Expected Coverage**: ~95% of test cases

### Phase 5: Integration & Benchmarks (Weeks 5-6)

- Full integration with query planner
- End-to-end scenarios with mixed workloads
- Performance benchmarks

```bash
bazel test //tests/epic3_distributed_tensor:integration_test
bazel run //benchmarks/epic3_distributed_tensor:recovery_rebuild_bench
```

---

## 4. Test Execution Environment

### 4.1 Test Harness Setup

```cpp
class RecoveryTestHarness {
public:
  RecoveryTestHarness()
      : manifest_store_(std::make_unique<InMemoryManifestStore>()),
        shard_cluster_(std::make_unique<MockShardCluster>()),
        recovery_manager_(std::make_unique<RecoveryManager>(
            manifest_store_.get(), shard_cluster_.get())) {}
  
  // Inject failures
  void InjectShardLoss(ShardId id) { shard_cluster_->Down(id); }
  void InjectCorruption(ShardId id, ArtifactId artifact) {
    shard_cluster_->Corrupt(id, artifact);
  }
  
  // Execute recovery
  Future<void> TriggerRecovery(ArtifactId id) {
    return recovery_manager_->RecoverArtifact(id);
  }
  
  // Verify results
  void VerifyRecoverySuccessful(ArtifactId id) {
    auto artifact = manifest_store_->Get(id);
    ASSERT_TRUE(artifact.is_available);
    ASSERT_TRUE(VerifyIntegrity(artifact));
  }

private:
  std::unique_ptr<ManifestStore> manifest_store_;
  std::unique_ptr<ShardCluster> shard_cluster_;
  std::unique_ptr<RecoveryManager> recovery_manager_;
};
```

### 4.2 Parameterized Testing

```cpp
// Test each recovery scenario with multiple artifact sizes
INSTANTIATE_TEST_SUITE_P(
    RecoveryPathSizes,
    RecoveryPathTest,
    ::testing::Values(
        1MB,      // Small artifact
        100MB,    // Medium artifact
        1GB,      // Large artifact
        10GB      // Very large artifact
    ));

// Test each rebuild mode with different source availability
INSTANTIATE_TEST_SUITE_P(
    RebuildModeSourceAvail,
    RebuildModeTest,
    ::testing::Combine(
        ::testing::Values(RebuildMode::PATCH,
                         RebuildMode::PARTIAL_REFIT,
                         RebuildMode::SNAPSHOT,
                         RebuildMode::INVALIDATE),
        ::testing::Values(SourceAvailable::REPLICA,
                         SourceAvailable::EXACT,
                         SourceAvailable::LINEAGE,
                         SourceAvailable::NONE)
    ));
```

### 4.3 Chaos Testing

For production-grade testing, include chaos engineering:

```cpp
class ChaosRecoveryTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Random seed for reproducibility if needed
    random_gen_.seed(::testing::UnitTest::GetInstance()
                         ->random_seed());
  }
  
  // Randomly inject failures
  void RandomlyInjectFailures(int num_failures) {
    std::uniform_int_distribution<> shard_dist(0, num_shards_ - 1);
    std::uniform_int_distribution<> artifact_dist(0, num_artifacts_ - 1);
    
    for (int i = 0; i < num_failures; ++i) {
      auto shard = shard_dist(random_gen_);
      auto artifact = artifact_dist(random_gen_);
      
      // 50% chance of loss, 50% chance of corruption
      if (random_gen_() % 2 == 0) {
        shard_cluster_->Down(shard);
      } else {
        shard_cluster_->Corrupt(shard, artifact);
      }
    }
  }

private:
  std::mt19937 random_gen_;
};

TEST_F(ChaosRecoveryTest, RecoverFromRandomFailures) {
  RandomlyInjectFailures(10);
  
  // Should recover all artifacts
  for (auto artifact : all_artifacts_) {
    auto result = recovery_manager_->RecoverArtifact(artifact).get();
    EXPECT_EQ(result.status, RecoveryStatus::HEALTHY);
  }
}
```

---

## 5. Test Acceptance Criteria

Each test category must meet:

1. **Code Coverage**: ≥90% line coverage for recovery paths
2. **Pass Rate**: 100% of implemented tests pass
3. **Performance**: All tests complete in <30 minutes total
4. **Determinism**: Tests pass consistently (no flakiness)
5. **Documentation**: Clear comments explaining failure injection points
6. **Reproducibility**: Failed tests can be debugged and reproduced locally

---

## 6. Benchmark Specifications

### 6.1 Benchmark: Rebuild Latency

```cpp
void BenchmarkSnapshotRebuild(benchmark::State& state) {
  auto artifact_size = state.range(0);  // Parameter: 100MB, 1GB, 10GB
  
  for (auto _ : state) {
    auto artifact = CreateArtifact(artifact_size);
    manifest_store_->Store(artifact.id, artifact);
    
    // Inject loss
    shard_cluster_->Down(primary_shard_);
    
    // Measure rebuild time
    auto start = Clock::now();
    recovery_manager_->RecoverArtifact(artifact.id).get();
    auto duration = Clock::now() - start;
  }
}
BENCHMARK(BenchmarkSnapshotRebuild)->Range(100MB, 10GB);
```

**Expected Results**:
- 100MB: <5 seconds
- 1GB: <60 seconds
- 10GB: <600 seconds

### 6.2 Benchmark: Crash Recovery Time

```cpp
void BenchmarkCrashRecovery(benchmark::State& state) {
  auto num_pending_ops = state.range(0);  // 10, 100, 1000 ops
  
  for (auto _ : state) {
    // Create WAL with pending operations
    auto wal = CreateWAL(num_pending_ops);
    
    // Measure replay time
    auto start = Clock::now();
    recovery_manager_->RecoverFromCrash(wal).get();
    auto duration = Clock::now() - start;
  }
}
BENCHMARK(BenchmarkCrashRecovery)->Range(10, 1000);
```

**Expected Results**:
- 10 ops: <100ms
- 100 ops: <500ms
- 1000 ops: <5 seconds

---

## 7. Test Checklist

- [ ] All test fixtures created and validated
- [ ] Failure injection utilities implemented
- [ ] Recovery verifier utilities implemented
- [ ] Test harness set up and working
- [ ] Phase 1 tests (recovery paths) written and passing
- [ ] Phase 2 tests (rebuild modes) written and passing
- [ ] Phase 3 tests (crash recovery) written and passing
- [ ] Phase 4 tests (freshness & edge cases) written and passing
- [ ] Phase 5 integration tests written and passing
- [ ] Benchmarks implemented and baselined
- [ ] Chaos tests validating random failure scenarios
- [ ] Code coverage report ≥90%
- [ ] All tests deterministic and no flakiness
- [ ] Performance benchmarks within targets

---

## References

- `EPIC3_RECOVERY_STRATEGY.md`
- `TENSOR_RECOVERY_PROCEDURES.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
