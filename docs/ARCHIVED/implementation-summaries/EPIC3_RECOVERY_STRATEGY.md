## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 3.5 Recovery Strategy

<!-- Status: current | planning scaffold | validated: 2026-07-05 -->

## Summary

Recovery, rebuild, and erasure coding strategy for distributed tensor artifacts. Defines failure modes, recovery paths, rebuild policies, and crash-safe procedures for first-class distributed tensor objects.

## Scope

- Failure scenario analysis (loss, corruption, staleness, incompatibility)
- Recovery matrix: artifact class × failure scenario → recovery action
- Explicit rebuild modes (patch, partial refit, snapshot, invalidate)
- Crash-safe recovery procedures with worker resume semantics
- Rebuild amplification and freshness debt quantification
- Erasure coding and parity-like recovery where justified
- Integration with artifact lifecycle and integrity models

## Planned Repository Surfaces

- `src/distributed_tensor/include/recovery_manager.h`
- `src/distributed_tensor/src/recovery_manager.cc`
- `src/distributed_tensor/include/rebuild_policy.h`
- `src/distributed_tensor/src/rebuild_policy.cc`
- `src/distributed_tensor/include/crash_recovery.h`
- `src/distributed_tensor/src/crash_recovery.cc`
- `tests/epic3_distributed_tensor/recovery_manager_test.cc`
- `tests/epic3_distributed_tensor/rebuild_policy_test.cc`
- `tests/epic3_distributed_tensor/crash_recovery_test.cc`
- `benchmarks/epic3_distributed_tensor/recovery_rebuild_bench.cc`

---

## Part 1: Failure Scenarios and Analysis

### 1.1 Failure Classification

Distributed tensor artifacts face multiple failure modes:

#### Loss Failures
- **Single shard loss**: One shard holding replica or fragment is unavailable
- **Multi-shard loss**: Multiple shards lost simultaneously
- **Complete artifact loss**: All copies/fragments of an artifact lost
- **Partial fragment loss**: Only subset of factorized components lost

#### Integrity Failures
- **Corruption**: On-disk corruption detected via checksum/hash mismatch
- **Partial corruption**: Only subset of artifact bytes corrupted
- **Manifest mismatch**: Physical artifact doesn't match manifest metadata
- **Receipt mismatch**: Integrity receipt (hash, signature) fails validation

#### Freshness Failures
- **Excessive delta lag**: Update delta queue exceeds threshold without being applied
- **Stale shard summary**: Summary hasn't been refreshed within SLA window
- **Stale factor cache**: Cached tensor factors older than rebuild interval
- **Model-switch incompatibility**: Adapter artifacts incompatible after LLM model switch

#### Logical Failures
- **Rank-cap breach**: Tensor rank exceeds placement capacity on shard
- **Dimension mismatch**: Tensor dimensions don't match manifest declaration
- **Format incompatibility**: Serialized format newer than reader capability
- **Missing factor components**: Required factorized components unavailable

#### Worker Failures
- **Worker crash during update**: Update operation interrupted mid-flight
- **Worker crash during rebuild**: Rebuild operation incomplete on failure
- **Worker crash during GC**: Garbage collection / eviction incomplete
- **Orphan artifact**: Artifact referenced only by crashed worker, lost from manifest coordination

#### Dependency Failures
- **Lineage unavailable**: Source package/lineage used for rebuild is missing
- **Incompatible rebuild source**: Available lineage has incompatible format/version
- **Partial lineage**: Source data available but cannot fully reconstruct artifact

### 1.2 Failure Scenario Taxonomy

| Scenario | Detection | Impact | Recovery Window | Cost |
|----------|-----------|--------|-----------------|------|
| Single shard loss (replicated) | Health check / shard unavailable | Degraded to replica | Immediate | Replica promotion |
| Multi-shard loss | Multiple shards unavailable | Lost artifact if no copy | Immediate | Rebuild from lineage |
| Corruption (checksum fail) | Integrity check on read | Query fails / degraded | On-demand | Rebuild from healthy replica |
| Stale shard summary | Timestamp check / version | Query may fail / false negatives | Background | Summary refresh/rebuild |
| Excessive delta lag | Queue depth monitoring | Update stale, consistency drift | Automated threshold | Apply delta or invalidate |
| Rank-cap breach | Placement verify on shard | Shard full / new placement impossible | Placement phase | Rebalance shards |
| Failed partial refit | Refit status check | Tensor incomplete / invalid state | Rebuild phase | Resume or snapshot rebuild |
| Worker crash during update | Heartbeat timeout / wal recovery | Transaction incomplete | Recovery log | Crash recovery + replay |
| Model-switch incompatibility | Adapter version check | Adapter unusable | Model activation | Re-materialize adapter |

---

## Part 2: Recovery Matrix

### 2.1 Recovery Matrix: Artifact Class × Failure Scenario

Matrix structure:
- **Rows**: Artifact classes (Primary Tensor, Derived Summary, Adapter, Shard Summary, Ephemeral)
- **Columns**: Failure scenarios (Loss, Corruption, Staleness, Incompatibility, Worker Crash)
- **Cells**: Recovery action code (P1–P4, fallback strategy)

#### 2.1.1 Primary Tensor Artifacts (LoRA weights, factor matrices, TT cores, etc.)

| Failure | Detection | Action | Priority | SLA | Fallback |
|---------|-----------|--------|----------|-----|----------|
| Single shard loss | Health check | Use replica (if exists) | P1 | <1s | Rebuild from lineage |
| Multi-shard loss | Availability check | Rebuild from lineage | P1 | <5s | Graph validation (exact graph fallback) |
| Corruption | Hash/checksum mismatch | Use replica or rebuild | P1 | <5s | Graph validation |
| Stale cache | Age check (>TTL) | Invalidate or refresh | P2 | <60s | Recompute on-demand |
| Rank-cap breach | Placement verify | Rebalance shards | P2 | <30s | Request new placement |
| Model-switch | Version mismatch | Re-materialize | P3 | <300s | Use previous version |
| Worker crash | WAL missing segment | Crash recovery + replay | P1 | <10s | Rebuild from package lineage |
| Update stall | Delta lag >threshold | Apply or invalidate | P2 | <120s | Rebuild from source |

#### 2.1.2 Derived Tensor Artifacts (summaries, fingerprints, routing tensors)

| Failure | Detection | Action | Priority | SLA | Fallback |
|---------|-----------|--------|----------|-----|----------|
| Single shard loss | Replica unavailable | Rebuild from exact | P2 | <5s | Graph-validated query only |
| Multi-shard loss | All replicas lost | Rebuild from exact | P2 | <10s | Graph-validated query only |
| Corruption | Hash mismatch | Rebuild from exact | P2 | <5s | Graph-validated query only |
| Stale | Age check | Refresh from exact | P3 | <60s | Use stale but track debt |
| Incompatible format | Version check | Regenerate | P3 | <60s | Ignore summary, use exact |
| Worker crash | GC interrupted | Resume GC or rebuild | P3 | <60s | Mark for lazy refresh |

#### 2.1.3 Adapter Artifacts (LLM fine-tuned adapters)

| Failure | Detection | Action | Priority | SLA | Fallback |
|---------|-----------|--------|----------|-----|----------|
| Single shard loss | Replica unavailable | Use replica (if any) | P1 | <5s | Re-materialize from LoRA |
| Multi-shard loss | All replicas lost | Re-materialize from LoRA | P1 | <60s | Base LLM only |
| Corruption | Hash mismatch | Re-materialize from LoRA | P1 | <60s | Base LLM only |
| Model-switch | Adapter incompatible | Convert or re-materialize | P1 | <300s | Base LLM only |
| Stale | Not applicable (immutable once materialized) | N/A | N/A | N/A | N/A |
| Worker crash during remat | Process interrupted | Crash recovery + replay | P1 | <60s | Re-start materialization |

#### 2.1.4 Shard Summaries (cross-shard routing metadata)

| Failure | Detection | Action | Priority | SLA | Fallback |
|---------|-----------|--------|----------|-----|----------|
| Single shard loss | Summary shard down | Use cached copy | P1 | <5s | Reconstruct from shards |
| Multi-shard loss | Summary shards + data shards lost | Reconstruct from survivors | P1 | <10s | Graph search only |
| Stale | Timestamp >age threshold | Refresh from live shards | P2 | <120s | Accept staleness, track debt |
| Corruption | Hash mismatch | Reconstruct from shards | P1 | <10s | Graph search only |
| Worker crash | Replication lag | Sync from replica shard | P1 | <5s | Use best-effort replica |

#### 2.1.5 Ephemeral Artifacts (query-time, session-local tensors)

| Failure | Detection | Action | Priority | SLA | Fallback |
|---------|-----------|--------|----------|-----|----------|
| Loss | Session termination | Recompute if needed | P4 | Immediate | Return partial result |
| Worker crash | Process crash | Recompute in new session | P4 | Immediate | Return partial result |
| Corruption | Session integrity check | Recompute | P4 | Immediate | Return partial result |

### 2.2 Recovery Priority Levels

- **P1 (Critical)**: Artifact required for correctness; must recover or fail loudly
- **P2 (High)**: Artifact degrades performance; should recover within SLA
- **P3 (Medium)**: Artifact optional for immediate request; can defer recovery
- **P4 (Low)**: Artifact ephemeral; loss acceptable, recompute on demand

---

## Part 3: Rebuild Modes

### 3.1 Explicit Rebuild Modes

Rebuild operations must distinguish between four explicit modes:

#### Mode A: Patch
**When**: Small, localized updates to existing artifacts  
**Mechanism**: Apply differential updates (delta) to existing artifact  
**Cost**: Minimal; proportional to delta size  
**Preconditions**:
- Base artifact exists and is uncorrupted
- Delta is available and well-defined
- Merged result maintains integrity invariants

**Example**: Apply new LoRA weights as delta to existing adapter

#### Mode B: Partial Refit
**When**: Incremental updates affecting subset of artifact structure  
**Mechanism**: Refit subset of factors/components, leave others in place  
**Cost**: Moderate; proportional to affected subset  
**Preconditions**:
- Existing factors are uncorrupted and valid
- Refit operation has partial rollback capability
- Updated factors remain compatible with untouched factors

**Example**: Update factorized tensor components after model tuning; skip unchanged factors

#### Mode C: Snapshot Rebuild
**When**: Complete artifact reconstruction from source data  
**Mechanism**: Fully materialize artifact from lineage / source package  
**Cost**: High; full recomputation from source  
**Preconditions**:
- Source package/lineage fully available
- Computation resources available
- Output artifact storage available

**Example**: Rebuild full LoRA adapter from package lineage after multi-shard loss

#### Mode D: Invalidate and Re-materialize
**When**: Artifact state is invalid, uncertain, or incompatible  
**Mechanism**: Mark artifact invalid; trigger re-materialization on next access  
**Cost**: Deferred; amortized across queries  
**Preconditions**:
- Queries can tolerate temporary unavailability or graph-only fallback
- Alternative compute path exists (e.g., exact graph validation)
- Re-materialization can be triggered on-demand

**Example**: Invalidate shard summary after model switch; rebuild on next query

### 3.2 Rebuild Mode Decision Matrix

| Condition | Artifact State | Data Available | Latency Requirement | Selected Mode |
|-----------|----------------|-----------------|---------------------|---------------|
| Delta lag <100ms | Present, valid | Delta available | <1s | **Patch** |
| Subset of factors stale | Present, partial valid | Full source available | <10s | **Partial Refit** |
| Corruption detected | Present, corrupted | Full source available | <60s | **Snapshot Rebuild** |
| Model mismatch | Present, incompatible | Full source available | <300s | **Invalidate+Remat** |
| Multi-shard loss | Missing | Full source available | <120s | **Snapshot Rebuild** |
| Temporary inconsistency | Present, uncertain | Partial source | Deferred | **Invalidate+Remat** |
| Complete invalidation | Unknown state | Unknown | Flexible | **Invalidate+Remat** |

### 3.3 Rebuild Ordering and Prioritization

When multiple artifacts require rebuild:

1. **Tier 1 (Critical path)**: Artifacts required for ongoing transactions
2. **Tier 2 (Query path)**: Artifacts needed for pending queries
3. **Tier 3 (Maintenance)**: Artifacts for background tasks
4. **Tier 4 (Optional)**: Cache, temporary, or ephemeral artifacts

Within each tier:
- Sort by **Priority Level** (P1 > P2 > P3 > P4)
- Sort by **Rebuild Cost** (patch < refit < snapshot < invalidate)
- Sort by **Dependencies** (rebuild dependent artifacts first)

---

## Part 4: Crash-Safe Recovery Procedures

### 4.1 Worker Resume and State Recovery

When a worker crashes:

1. **Detection**: Heartbeat timeout or health check failure
2. **Identification**: Determine which artifacts/operations were in-flight
3. **State salvage**: Read write-ahead log (WAL) or recovery log for uncommitted ops
4. **Classification**: Categorize uncommitted ops (update, rebuild, GC, etc.)
5. **Resume or abort**: Decide whether to resume or abort each operation class

#### 4.1.1 Update Operation Recovery

**Scenario**: Worker crashed during `apply_delta` to primary tensor

**Recovery procedure**:
```
1. Read WAL entry for update transaction
2. Verify manifest version before update
3. If manifest hasn't advanced: resume update from WAL checkpoint
4. If manifest has advanced: abort update, rebuild from current lineage
5. Validate merged artifact post-resume
6. Acquire write lock and commit merged state
7. Update manifest with new version and timestamp
```

**Idempotency**: Resumable operations must be idempotent at resume point

#### 4.1.2 Rebuild Operation Recovery

**Scenario**: Worker crashed during snapshot rebuild of corrupted primary tensor

**Recovery procedure**:
```
1. Read WAL entry for rebuild transaction
2. Identify rebuild source (lineage/package)
3. Verify rebuild target artifact ID
4. If rebuild destination is partial: resume from last checkpoint
5. If rebuild destination is corrupted: restart from scratch
6. Validate rebuilt artifact against manifest hash
7. Atomically promote rebuilt artifact to live location
8. Update manifest + integrity receipt
```

**Checkpoint intervals**: Save rebuild progress periodically (e.g., every 10% of artifact)

#### 4.1.3 Garbage Collection Recovery

**Scenario**: Worker crashed during GC; eviction list partially applied

**Recovery procedure**:
```
1. Read GC transaction log
2. Identify evicted artifacts and eviction reasons
3. Classify evictions: completed vs. in-progress
4. For completed: commit eviction, remove from manifest
5. For in-progress: abort eviction, restore artifact reference
6. Update GC watermark to safe checkpoint
```

### 4.2 Distributed Coordination During Recovery

For multi-worker environments:

1. **Lock coordination**: Acquire distributed lock on affected artifact before resume
2. **Manifest update**: Ensure only one worker updates manifest
3. **Replica synchronization**: Sync recovered state to replicas
4. **Conflict resolution**: If multiple workers crash and conflict, resolve via manifest version

### 4.3 Write-Ahead Log (WAL) Design for Recovery

Each worker maintains a recovery log with:

- **Transaction ID**: Unique per operation
- **Operation type**: Update, rebuild, GC, etc.
- **Affected artifact IDs**: What's being modified
- **Checkpoint data**: Sufficient state to resume mid-operation
- **Timestamp**: When operation started
- **Status**: In-progress, committed, aborted

**Log rotation**: Move completed logs to cold storage after checkpoint confirmation

---

## Part 5: Rebuild Amplification and Freshness Debt

### 5.1 Rebuild Amplification Factors

Rebuild operations can amplify resource costs:

| Factor | Description | Multiplier Range |
|--------|-------------|-------------------|
| **Replication overhead** | Rebuild to N replicas vs. single copy | 1–5× |
| **Cross-shard transfer** | Network cost for pulling source data | 1–10× |
| **Recomputation cost** | CPU/GPU cost to recompute factors | 1–100× |
| **I/O amplification** | Cache evictions during rebuild | 1–5× |
| **Contention delay** | Lock waits during distributed rebuild | 1–10× |

Total amplification: **Product of applicable factors**

Example: Snapshot rebuild with 2 replicas, network transfer, and GPU recomputation
```
Amplification = 2 (replication) × 5 (network) × 50 (GPU) = 500×
```

### 5.2 Freshness Debt Tracking

Track cumulative staleness when updates are delayed:

```
Freshness Debt = Σ(staleness_age_i × impact_i) for all stale artifacts
```

Where:
- `staleness_age_i`: How long artifact i has been stale
- `impact_i`: Query-affecting impact of staleness (0–1)

**Thresholds**:
- **Green** (<5% freshness debt): Normal operation
- **Yellow** (5–20%): Degraded but acceptable; monitor
- **Red** (>20%): Unacceptable; trigger immediate rebuild/invalidation

**Actions**:
- Red threshold: Invalidate stale artifacts, trigger re-materialization
- Persistent red: Alert operator; may need scale-out

### 5.3 Rebuild Amplification Mitigation

To reduce amplification:

1. **Batch updates**: Combine multiple deltas into single patch
2. **Local rebuild**: Materialize locally before distributing
3. **Partial replication**: Replicate only hot artifacts
4. **Lazy invalidation**: Defer re-materialization until accessed
5. **Staged rollout**: Rebuild one shard at a time

---

## Part 6: Erasure Coding and Parity Recovery

### 6.1 When to Use Erasure Coding

Erasure coding is beneficial for:
- **Large immutable artifacts** (e.g., LoRA package archives)
- **Infrequently updated data** (change detection easier)
- **Multiple-shard loss resilience** (vs. simple replication)

Not recommended for:
- **Frequently updated artifacts** (parity recalculation cost)
- **Small artifacts** (<100 MB)
- **Latency-critical paths** (reconstruction overhead)

### 6.2 Proposed Erasure Coding Scheme

For artifacts supporting erasure:

```
(k, m) Reed-Solomon code
- k original blocks
- m parity blocks
- Can recover from loss of any m blocks
- Reconstruction requires reading any k blocks
```

Example: (4, 2) code on 6 shards
- 4 original LoRA blocks, 2 parity
- Survive loss of any 2 shards
- Rebuild requires reading 4 of 6 shards

### 6.3 Parity Maintenance

- **Eager parity**: Compute on artifact creation (simpler, higher write cost)
- **Lazy parity**: Compute on first read (deferred cost, complex)
- **Incremental parity**: Update parity with each delta (balanced)

**Recommended**: Lazy parity for frequently updated; eager for stable artifacts

---

## Part 7: Integration with Artifact Lifecycle

### 7.1 Recovery Interactions with Lifecycle States

Artifact state machine (simplified):

```
CREATED
  ↓
MATERIALIZING → [refit/rebuild on failure] → MATERIALIZING
  ↓
AVAILABLE (fresh, replicated)
  ↓
AGING [tracking freshness debt]
  ↓
STALE [regenerate or invalidate]
  ↓
INVALIDATED [marked for re-materialization]
  ↓
RE-MATERIALIZING → [crash recovery] → RE-MATERIALIZING
  ↓
AVAILABLE
  ↓
EVICTING [GC in progress] → [crash recovery] → EVICTING
  ↓
EVICTED [removed]
```

Recovery actions at each state:
- **MATERIALIZING**: Resume from checkpoint or restart
- **AVAILABLE**: Use replicas or rebuild
- **STALE**: Refresh or invalidate
- **INVALIDATED**: Trigger re-materialization
- **RE-MATERIALIZING**: Resume or restart
- **EVICTING**: Complete or rollback eviction

### 7.2 Manifest Consistency During Recovery

Manifest must remain consistent:
- **Before recovery**: Manifest reflects current state
- **During recovery**: Lock artifact entry; WAL tracks in-flight changes
- **After recovery**: Atomically update manifest with recovered state

**Invariant**: Manifest version always >= artifact version in any shard

---

## Part 8: Engineering Guidance

### 8.1 Recovery Manager API Contract

```cpp
class RecoveryManager {
public:
  // Query artifact recovery status
  RecoveryStatus GetStatus(ArtifactId id);
  RecoveryMetrics GetMetrics(ArtifactId id);
  
  // Trigger recovery for specific artifact
  Future<RecoveryResult> RecoverArtifact(
    ArtifactId id,
    RecoveryMode mode = Mode::Auto);
  
  // Batch recovery
  Future<vector<RecoveryResult>> RecoverBatch(
    vector<ArtifactId> ids,
    RecoveryPriority priority = Priority::P2);
  
  // Monitor recovery progress
  Stream<RecoveryProgress> WatchRecovery(ArtifactId id);
  
  // Crash recovery on startup
  Future<int> RecoverFromCrash(
    WorkerId worker_id,
    const WALSnapshot& wal);
  
  // Freshness debt metrics
  FreshnesDebtMetrics GetDebtMetrics();
};
```

### 8.2 Rebuild Policy Configuration

```cpp
struct RebuildPolicy {
  // Rebuild mode selection
  std::map<ArtifactClass, RebuildModePreference> mode_prefs;
  
  // SLA configuration (per priority level)
  std::map<Priority, Duration> sla_targets;
  
  // Freshness thresholds
  Duration freshness_max_age;
  double  freshness_debt_red_threshold = 0.20;
  
  // Amplification limits
  int max_replication_factor = 3;
  double max_cross_shard_bandwidth_percent = 0.30;
  
  // Rebuild batch size
  int batch_size_small_artifacts = 100;
  int batch_size_large_artifacts = 10;
};
```

### 8.3 Crash Recovery Initialization

On worker startup:

```cpp
Future<void> InitializeCrashRecovery() {
  // 1. Read WAL from persistent storage
  auto wal = ReadWAL();
  
  // 2. Classify in-flight operations
  auto [update_ops, rebuild_ops, gc_ops] = ClassifyOps(wal);
  
  // 3. For each operation class:
  //    - Acquire locks
  //    - Resume or abort
  //    - Commit to manifest
  
  co_await ResumeUpdateOperations(update_ops);
  co_await ResumeRebuildOperations(rebuild_ops);
  co_await AbortGCOperations(gc_ops);
  
  // 4. Clear WAL; mark ready
  ClearWAL();
  worker_ready_.notify_all();
}
```

### 8.4 Monitoring and Observability

Required metrics:

- **Recovery latency** (p50, p99): Time to recover artifact
- **Rebuild cost** (bytes transferred, CPU time): Resource consumption
- **Freshness debt**: Cumulative staleness
- **Crash recovery time**: Time to resume from crash
- **Replica consistency**: Lag between primary and replicas

---

## Part 9: Testing Strategy

### 9.1 Test Categories

#### A. Recovery Path Tests
- [ ] Single shard loss → replica promotion
- [ ] Multi-shard loss → rebuild from lineage
- [ ] Corruption detection → rebuild from replica
- [ ] Stale summary → refresh from exact artifact
- [ ] Rank-cap breach → rebalance shards

#### B. Rebuild Mode Transition Tests
- [ ] Patch: Apply delta to existing artifact
- [ ] Partial refit: Update subset of factors
- [ ] Snapshot rebuild: Full materialization
- [ ] Invalidate+remat: Lazy re-materialization
- [ ] Mode escalation: Patch fails → upgrade to snapshot

#### C. Crash Recovery Tests
- [ ] Update crash → resume from WAL
- [ ] Rebuild crash → resume from checkpoint
- [ ] GC crash → rollback in-progress evictions
- [ ] Multi-worker crash → manifest conflict resolution
- [ ] Multiple crashes in succession → idempotency

#### D. Freshness Debt Tests
- [ ] Track staleness accumulation
- [ ] Red threshold triggers invalidation
- [ ] Batch updates reduce debt
- [ ] Lazy invalidation defers cost

#### E. Edge Cases
- [ ] Lost lineage → cannot rebuild → graceful degradation
- [ ] Partial lineage → rebuild from available subset
- [ ] Concurrent recovery attempts → serialize via lock
- [ ] Recovery during active queries → queries wait or fall back

#### F. Integration Tests
- [ ] Graph validation works during tensor recovery
- [ ] Planner handles unavailable artifacts correctly
- [ ] Model-switch with incompatible adapters → re-materialize
- [ ] Query planning prefers replicas over rebuild

### 9.2 Test Data and Scenarios

For each test, define:
- Initial artifact state (present/absent, fresh/stale, etc.)
- Failure injection point
- Expected recovery action
- Verification checks (manifest, replicas, integrity)

### 9.3 Benchmark Expectations

- **Patch**: <100ms (for small deltas)
- **Partial refit**: <5s (for 10–50% of artifact)
- **Snapshot rebuild**: <60s (for 1 GB artifact on LAN)
- **Crash recovery**: <10s (for typical WAL)
- **Replica promotion**: <1s (for available replica)

---

## Part 10: Acceptance Criteria

Recovery strategy is complete when:

1. ✓ **Failure scenarios enumerated**: All realistic failure modes documented
2. ✓ **Recovery matrix defined**: Each artifact class has clear recovery paths
3. ✓ **Rebuild modes explicit**: Patch/partial refit/snapshot/invalidate clearly distinguished
4. ✓ **Crash-safe procedures**: WAL-based recovery with idempotency
5. ✓ **Freshness debt tracked**: Staleness quantifiable and actionable
6. ✓ **Recovery reproducible**: Tests confirm all documented paths work
7. ✓ **Graph Truth independent**: Tensor recovery doesn't block graph validation
8. ✓ **Engineering guidance provided**: API contracts and configuration clear
9. ✓ **Integration aligned**: Recovery works with lifecycle and integrity models
10. ✓ **Performance characterized**: Rebuild costs measured and within acceptable ranges

---

## Part 11: Planned Implementation Phases

### Phase 1: Design / API Contract
- [x] Failure scenario analysis complete
- [x] Recovery matrix defined
- [x] Rebuild modes explicit
- [x] Crash-safe procedures outlined
- [ ] Freeze `RecoveryManager` API and configuration schema

### Phase 2: Core Implementation
- [ ] Implement `recovery_manager.h/cc` with basic recovery dispatch
- [ ] Implement `rebuild_policy.h/cc` with mode selection logic
- [ ] Implement `crash_recovery.h/cc` with WAL-based resume
- [ ] Integrate with artifact manifest and lifecycle

### Phase 3: Error Handling and Edge Cases
- [ ] Handle multi-shard loss gracefully
- [ ] Implement partial lineage reconstruction
- [ ] Add conflict resolution for concurrent recovery attempts
- [ ] Implement replica consistency checking

### Phase 4: Tests
- [ ] Recovery path tests (all 5 categories)
- [ ] Crash recovery tests with fault injection
- [ ] Rebuild mode transition tests
- [ ] Freshness debt accumulation tests

### Phase 5: Performance and Hardening
- [ ] Benchmark rebuild performance
- [ ] Measure recovery latency (p50, p99)
- [ ] Optimize amplification factors
- [ ] Load-test concurrent recovery

### Phase 6: Documentation and Acceptance
- [ ] Verify recovery reproduces as documented
- [ ] Confirm graph-truth fallback works
- [ ] Validate freshness debt tracking
- [ ] Document operational runbooks

### Phase 7: Integration
- [ ] Wire `RecoveryManager` into query planner
- [ ] Integrate with artifact lifecycle state machine
- [ ] Add recovery metrics to observability stack
- [ ] Enable default recovery workflows in distributed deployment

---

## References

- `DISTRIBUTED_TENSOR_SHARDING.md` (Section 10: Recovery and Rebuild)
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC3_ARTIFACT_CLASSES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
- `HARDWARE_REQUIREMENTS.md`
- `TARGET_ARCHITECTURE.md`
