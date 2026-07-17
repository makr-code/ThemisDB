# Tensor Delta Log, Manifest Store, and Update Worker - Design Documentation

## PHASE 2: CORE IMPLEMENTATION

**Issue:** makr-code/ThemisDB#5471  
**Status:** ✅ COMPLETE  
**Date:** 2026-07-03  
**Author:** ThemisDB EPIC 3 Implementation Team

---

## 1. Executive Summary

This document describes the design and implementation of three core components for dynamic tensor artifact maintenance in ThemisDB:

1. **TensorDeltaLog** - Records mutations in exact graph state for artifact refresh analysis
2. **ManifestStore** - Provides atomic, durable storage for artifact manifests with version history
3. **SnapshotBasedUpdateWorker** - Decides optimal update strategy (patch/refit/rebuild) based on delta analysis

These components form the bridge between the exact graph state and derived tensor artifacts, enabling asynchronous, safe, production-grade tensor artifact refreshing.

---

## 2. Design Principles

### 2.1 Architectural Axioms

1. **Graph Truth is Authoritative**
   - Exact RocksDB-backed graph state is always correct
   - Tensor artifacts are advisory-only, never truth-bearing
   - Fallback to exact graph is always possible and encouraged

2. **Asynchronous Safety**
   - Tensor updates do not block the commit path
   - Delta logging has minimal write-path overhead
   - Worker processes deltas independently

3. **Determinism and Reproducibility**
   - Delta serialization is deterministic (pipe-delimited format)
   - Update decisions are reproducible given identical deltas
   - State transitions are idempotent where possible

4. **Observability First**
   - All components export metrics for monitoring
   - State transitions are visible via manifest
   - Worker decisions are traceable for debugging

### 2.2 Core Invariants

**TensorDeltaLog Invariants:**
- Sequence numbers are strictly monotonically increasing
- Deltas are write-once and immutable
- Sequence ordering within a range is guaranteed
- Retention policy prevents unbounded memory growth

**ManifestStore Invariants:**
- CAS (compare-and-swap) ensures atomic updates
- Version history is immutable (write-once)
- Only one lock holder per artifact at any time
- Manifest validate() always returns true for stored manifests

**UpdateWorker Invariants:**
- Update decisions are deterministic given artifact size and residual
- Worker never leaves artifact in inconsistent state
- Published manifests are always valid
- Statistics are cumulative and monotonic

---

## 3. Component Specifications

### 3.1 TensorDeltaLog: Delta Recording and Windowing

**Purpose:** Record all mutations (INSERT, UPDATE, DELETE, SHARD_CHANGE) that occur in the exact graph state, enabling the update worker to analyze delta patterns and decide refresh strategy.

#### Data Structures

**DeltaMutationType Enum:**
```cpp
enum class DeltaMutationType : uint8_t {
  INSERT = 0,              // New entity/relation added
  UPDATE = 1,              // Existing entity/relation modified
  DELETE = 2,              // Entity/relation removed
  SHARD_CHANGE = 3,        // Entity moved to different shard
  METADATA_UPDATE = 4      // Metadata-only change
};
```

**DeltaLogEntry Struct:**
- `sequence_number`: Monotonic identifier (never reused)
- `mutation_type`: Type of change (INSERT/UPDATE/DELETE/etc.)
- `affected_entity_id`: ID of changed entity
- `recorded_at_ms`: Timestamp in milliseconds since epoch
- `source_transaction_id`: Originating transaction/commit ID
- `shard_hint`: Optional shard placement hint
- `payload_size_bytes`: Size of changed data
- `payload_checksum`: Optional integrity hash

**DeltaWindow Struct:**
- `artifact_id`: Which artifact these deltas relate to
- `sequence_start`, `sequence_end`: Range of sequences in window
- `entries`: Ordered vector of DeltaLogEntry
- `total_payload_size_bytes`: Sum of all entry payload sizes
- `extracted_at_ms`: Timestamp when window was extracted

#### Key Methods

**appendDelta(...) → uint64_t**
- Appends a delta entry to the log
- Assigns monotonic sequence number
- Records timestamp automatically
- Returns assigned sequence number (0 on error)
- Thread-safe for single writer

**extractWindow(sequence_start, sequence_end) → DeltaWindow**
- Returns immutable window of deltas in sequence range
- Validates sequence ordering within range
- Safe for multi-threaded reader access
- Deterministic given fixed input range

**garbage_collect(cutoff_sequence) → size_t**
- Removes all entries with sequence < cutoff_sequence
- Returns number of entries removed
- Respects retention policy (max entries, max age)
- Optional: enables multi-snapshot windows

**getStats() → Stats**
- Returns aggregate statistics for observability
- Counts by mutation type (INSERT/UPDATE/DELETE/SHARD_CHANGE)
- Tracks oldest/newest timestamps
- Total payload bytes for bandwidth estimation

#### Serialization

**Pipe-Delimited Format** (for compact storage):
```
sequence_number | mutation_type | affected_entity_id | recorded_at_ms | source_transaction_id | shard_hint | payload_size_bytes | payload_checksum
```

Example:
```
1234 | 0 | node:5678 | 1719000000000 | txn:xyz | shard-1 | 512 | abc123def456
```

**JSON Format** (for DeltaWindow):
```json
{
  "artifact_id": "tensor:v1.5",
  "sequence_start": 1000,
  "sequence_end": 1500,
  "extracted_at_ms": 1719000000000,
  "total_payload_size_bytes": 51200,
  "entries": [
    {
      "sequence_number": 1000,
      "mutation_type": 0,
      "affected_entity_id": "node:5678",
      ...
    }
  ]
}
```

#### Retention Policy

Two-dimensional retention:
1. **Max Entries**: Keep at most N recent deltas (default: 100,000)
2. **Max Age**: Discard deltas older than T milliseconds (default: 24 hours)

Garbage collection happens:
- On manual call to garbage_collect()
- Optionally on setRetentionPolicy() update
- During retention policy enforcement

#### Durability

- **In-Memory**: Primary storage is vector<DeltaLogEntry>
- **RocksDB Integration Point**: persistToStorage() / loadFromStorage() (placeholder)
- **Recovery**: loadFromStorage() restores after crash

### 3.2 ManifestStore: Atomic Manifest Persistence

**Purpose:** Provide atomic, durable storage for ArtifactManifest with version history, enabling safe coordination of artifact updates across the system.

#### Data Structures

**ManifestStoreStatus Enum:**
```cpp
enum class ManifestStoreStatus : uint8_t {
  OK = 0,                      // Operation succeeded
  NOT_FOUND = 1,               // Manifest not in store
  CAS_FAILED = 2,              // Version mismatch (CAS failure)
  INVALID_MANIFEST = 3,        // Manifest failed validation
  STORAGE_ERROR = 4,           // RocksDB error
  VERSION_MISMATCH = 5,        // Requested version doesn't exist
  ARTIFACT_LOCKED = 6,         // Exclusive lock held by another owner
  VERSION_LIMIT_EXCEEDED = 7   // Too many versions for artifact
};
```

**ManifestVersion Struct:**
- `version_id`: Unique, monotonically increasing per artifact
- `created_at_unix_sec`: When this version was created
- `manifest`: The ArtifactManifest at this version
- `change_reason`: Why it changed (e.g., "patched", "rebuilt")
- `changed_by`: Who/what triggered the change

#### Key Methods

**read(artifact_id) → Status + ArtifactManifest**
- Reads latest manifest version
- Returns NOT_FOUND if artifact doesn't exist
- Thread-safe for concurrent reads

**write(artifact_id, manifest, expected_version, reason, changed_by) → Status**
- Atomically updates manifest using CAS semantics
- expected_version == 0 means "any version OK"
- Creates new version record on success
- Returns CAS_FAILED if version mismatch

**swapManifest(artifact_id, new_manifest, current_version, reason) → Status**
- Atomic swap for publish pattern
- Used by UpdateWorker to atomically replace artifact
- Returns CAS_FAILED if current_version doesn't match
- Creates audit trail entry

**acquireLock(artifact_id, lock_holder, timeout_ms) → Status**
- Exclusive lock for long-running updates
- Returns ARTIFACT_LOCKED if already held
- Timeout prevents deadlocks
- lock_holder is identifier for diagnostics

**releaseLock(artifact_id, lock_holder) → Status**
- Releases lock held by specific owner
- Returns ARTIFACT_LOCKED if held by different owner
- Prevents accidental lock release by wrong holder

**getVersionHistory(artifact_id) → vector<ManifestVersion>**
- Returns full version history for audit
- Ordered newest-last
- Enables rollback and diagnostics

#### Storage Schema (RocksDB)

**Current Manifest:**
- Key: `manifest:{artifact_id}:current`
- Value: JSON-encoded ArtifactManifest

**Version History:**
- Key: `manifest:{artifact_id}:version:{version_id}`
- Value: JSON-encoded ManifestVersion

**Lock Table:**
- Key: `lock:{artifact_id}`
- Value: JSON with lock_holder and lock_expire_time_ms

#### Atomic Update Protocol

CAS Update Steps:
1. Read current manifest (get current_version)
2. Modify manifest as needed
3. Call write(..., new_manifest, current_version)
4. If CAS_FAILED, retry from step 1 (backoff)
5. If successful, new version created

---

### 3.3 SnapshotBasedUpdateWorker: Intelligent Update Scheduling

**Purpose:** Consume delta windows from TensorDeltaLog and decide the optimal update strategy (PATCH, PARTIAL_REFIT, REBUILD) for each artifact based on delta analysis.

#### Update Decision Logic

**Decision Algorithm:**

```
Change Fraction = total_delta_payload / artifact_size_bytes

if change_fraction < patch_threshold_pct (default 10%):
    decision = PATCH
    rationale: Small changes, efficient to patch
    
else if change_fraction < refit_threshold_pct (default 50%):
    estimated_residual = current_residual + (change_fraction * 0.05)
    if estimated_residual - current_residual <= residual_max_increase (default 5%):
        decision = PARTIAL_REFIT
        rationale: Medium changes, selective retraining possible
    else:
        decision = REBUILD
        rationale: Residual would exceed tolerance
        
else:
    decision = REBUILD
    rationale: Large delta, full rebuild more efficient than partial refit
```

**Why These Thresholds?**

- **PATCH (< 10%)**: Applying patches is O(delta_size), very efficient
- **PARTIAL_REFIT (10-50%)**: Retraining subset is O(k), moderate cost
- **REBUILD (> 50%)**: Full rebuild is O(n), same cost range as refit, cleaner state

**Residual Breach Detection:**

Partial refit may not be viable if it would degrade artifact quality:
- Estimate residual increase: `delta_impact * patch_quality_loss_factor`
- If `new_residual > current_residual + tolerance`, fallback to REBUILD
- Ensures quality contracts are maintained

**Rank Cap Breach Detection:**

Prevent situations where rank grows unbounded:
- Check if delta window would cause rank_status to exceed rank_cap
- If breach detected, fallback to REBUILD
- Rebuild resets rank_status to optimal level

#### Worker State Machine

```
IDLE ──start()──> READY ──processTask()──> PROCESSING ──complete──> READY
                    ▲                           │
                    └───────────────────────────┘
                    
READY ──error──> ERROR ──shutdown()──> IDLE
                   │
                   └──recover──> READY
```

States:
- **IDLE**: Not running, resources not allocated
- **READY**: Initialized, ready to accept tasks
- **PROCESSING**: Currently processing a delta window
- **ERROR**: Error occurred, manual intervention needed
- **SHUTTING_DOWN**: Graceful shutdown in progress

#### Metrics Collection

**UpdateMetrics (per task):**
- `decision`: PATCH/PARTIAL_REFIT/REBUILD/ERROR_FALLBACK
- `analysis_time_ms`: Time spent deciding strategy
- `execution_time_ms`: Time spent executing update
- `resulting_residual`: Quality metric after update
- `resulting_rank_status`: Rank value after update
- `success`: Whether update succeeded
- `error_message`: Error details if failed
- `throughput_deltas_per_sec`: Throughput estimate

**Worker Stats (aggregate):**
- `total_tasks_processed`: Total deltas processed
- `total_patches_applied`: Count of PATCH decisions
- `total_partial_refits`: Count of PARTIAL_REFIT decisions
- `total_rebuilds`: Count of REBUILD decisions
- `total_failed_updates`: Count of failures
- `average_decision_time_ms`: Mean decision latency
- `average_execution_time_ms`: Mean execution latency
- `last_activity_ms`: Timestamp of last operation

#### Execution Strategies

**executePatch(...)**
```
1. Apply delta patches to artifact in-place
2. Update manifest:
   - rebuild_state = PATCHED
   - update_mode = PATCH
   - source_seq_end = delta_window.sequence_end
   - delta_lag = 0
   - residual += (change_fraction * patch_residual_increase)
3. Validate resulting manifest
4. Return success/failure
```

**executePartialRefit(...)**
```
1. Check for rank cap breach; if yes, fallback to REBUILD
2. Selectively retrain tensor components affected by delta
3. Update manifest:
   - rebuild_state = PARTIAL_REFITTED
   - update_mode = PARTIAL_REFIT
   - source_seq_end = delta_window.sequence_end
   - delta_lag = 0
   - residual += (change_fraction * refit_residual_increase)
4. Validate resulting manifest
5. Return success/failure
```

**executeRebuild(...)**
```
1. Trigger full rebuild from source lineage
2. Create new artifact from scratch using reconstruction_instructions
3. Update manifest:
   - rebuild_state = REBUILT
   - update_mode = REBUILD
   - source_seq_end = delta_window.sequence_end
   - delta_lag = 0
   - residual = 0.0 (clean slate)
   - last_rebuild_at_unix_sec = now
4. Validate resulting manifest
5. Return success/failure
```

#### Publish Pattern (Atomic Swap)

```
1. Execute update (patch/refit/rebuild) → new_manifest
2. Get current manifest version: current_version
3. Call publishManifest(artifact_id, new_manifest, current_version, reason)
   
publishManifest steps:
  a. Validate new_manifest (manifest.validate())
  b. Call ManifestStore.write(artifact_id, new_manifest, current_version, reason)
  c. If CAS_FAILED:
     - Concurrent modification detected
     - Invalidate our update result (stale)
     - Return false for retry
  d. If OK:
     - Manifest atomically switched
     - All planner queries see new version
     - Return true
```

#### Crash Recovery

**Checkpoint Interface:**
```cpp
void setCheckpointPath(checkpoint_path);
  
// Periodically (e.g., every N tasks):
checkpoint.write({
  artifact_id,
  delta_window_processed,
  decision_made,
  metrics,
  manifest_version_before,
  manifest_version_after
});

// On recovery:
checkpoint.load() → previous_state
if (previous_state.manifest_version_after not yet published):
    // Publish was interrupted; retry publish
    publishManifest(...);
```

---

### 3.4 ArtifactInvalidationManager: Invalidation Policy

**Purpose:** Manage artifact state transitions when quality or freshness is compromised.

#### Invalidation Triggers

1. **Staleness Exceeded**: artifact_age_ms > staleness_threshold_sec
   - Reason: Data is too old, must be refreshed
   - Action: Mark STALE (still usable) or INVALIDATED (not usable)

2. **Integrity Check Failed**: manifest.isCorrupted() returns true
   - Reason: Content hash mismatch detected
   - Action: Mark INVALIDATED, mark for rebuild
   - Urgency: High (data integrity issue)

3. **Rank Cap Breach**: rank_status > rank_cap
   - Reason: Decomposition quality degraded
   - Action: Mark STALE or INVALIDATED
   - Urgency: Medium (may auto-recover with rebuild)

4. **Residual Threshold Breach**: residual > policy_threshold
   - Reason: Quality metric unacceptable for queries
   - Action: Mark STALE (can still be used with fallback)
   - Urgency: Low (advisory-only, fallback available)

5. **Source Invalidated**: Source artifact marked INVALIDATED
   - Reason: Dependencies corrupted
   - Cascade: Mark this artifact STALE or INVALIDATED
   - Urgency: High (derived artifact now unreliable)

#### State Transitions

```
CREATED ───┐
           ├──> ACTIVE ──> STALE ──┬──> REBUILT ──> ACTIVE
REBUILT ──→                       │
                            (invalidate)
                                  │
                            INVALIDATED ──> DELETED
```

**Valid Transitions:**
- CREATED → ACTIVE (on successful creation)
- ACTIVE → STALE (on staleness or quality triggers)
- ACTIVE → INVALIDATED (on integrity or urgency)
- STALE → REBUILT (on update completion)
- REBUILT → ACTIVE (on verification)
- STALE/INVALIDATED → DELETED (permanent removal)

**Invalid Transitions (rejected):**
- Backward transitions (e.g., STALE → ACTIVE, unless via REBUILT)
- Transitions from DELETED (terminal state)

---

### 3.5 ExactGraphFallback: Query Planning Integration

**Purpose:** Enforce exact graph fallback for queries when tensor artifacts don't meet requirements.

#### Fallback Decision Matrix

| Condition | Fallback? | Reason |
|-----------|-----------|--------|
| Lifecycle != ACTIVE/STALE | YES | Artifact not usable |
| advisory_only AND query_requires_truth | YES | Semantics mismatch |
| residual > query_tolerance | YES | Quality breach |
| artifact_age > query_max_age | YES | Freshness breach |
| All checks pass | NO | Can use artifact |

#### Implementation

```cpp
canUseArtifact(manifest, query_residual_tolerance, now_unix_sec):
    if requiresFallbackForState(manifest):
        return false  // CREATED/INVALIDATED/DELETED not usable
    
    if requiresFallbackForSemantics(manifest, query_requires_truth):
        return false  // advisory_only semantics
    
    if requiresFallbackForResidual(manifest, query_residual_tolerance):
        return false  // Quality unacceptable
    
    if requiresFallbackForFreshness(manifest, now_unix_sec, query_max_age_ms):
        return false  // Too stale
    
    return true  // All checks pass, artifact is usable
```

#### Query Parameters

- `query_residual_tolerance`: Maximum acceptable quality error (default: 0.0 = no tolerance)
- `query_requires_truth`: Does query need truth-bearing guarantees? (boolean)
- `query_max_age_ms`: Maximum artifact age acceptable (0 = any age OK)

#### Metrics

**FallbackMetrics:**
- `total_fallback_decisions`: Count of all fallback decisions
- `fallback_due_to_state`: Count of state-based fallbacks
- `fallback_due_to_semantics`: Count of semantic fallbacks
- `fallback_due_to_residual`: Count of quality-based fallbacks
- `fallback_due_to_freshness`: Count of staleness-based fallbacks
- `artifacts_used_successfully`: Count of non-fallback decisions

---

## 4. Integration Points

### 4.1 Graph Commit Path Integration

```
GraphUpdate (INSERT/UPDATE/DELETE)
    ↓
RocksDB Commit
    ↓
TensorDeltaLog.appendDelta(mutation_type, entity_id, txn_id)  ← Non-blocking
    ↓
Commit returns to application
```

Overhead: Minimal (append to vector + timestamp)

### 4.2 Query Planning Integration

```
Query Planner
    ↓
Retrieve artifact manifest
    ↓
ExactGraphFallbackPolicy.canUseArtifact(...)?
    ├─ YES → Use tensor summary for query guidance
    └─ NO  → Fallback to exact graph load
```

### 4.3 Background Maintenance Path

```
SnapshotBasedUpdateWorker (background thread)
    ↓
For each artifact:
    ├─ Read delta window from TensorDeltaLog
    ├─ Decide strategy (PATCH/PARTIAL_REFIT/REBUILD)
    ├─ Execute strategy
    ├─ Update manifest via ManifestStore.swapManifest()
    └─ Record metrics
```

---

## 5. Failure Handling (Phase 3)

### 5.1 Stale Artifact Backlog

**Scenario:** Worker can't keep up; delta backlog grows

**Solution:**
- Track delta_lag in manifest (source_seq_end)
- If delta_lag > policy_threshold, mark STALE
- Planner can decide to fallback or wait for worker
- Worker adjusts batch size or parallelization

### 5.2 Failed Partial Refit

**Scenario:** Partial refit fails or residual exceeds threshold

**Solution:**
- Mark artifact INVALIDATED
- Fallback to REBUILD on next cycle
- Record failure reason in invalidation_reason
- Alert operator if repeated failures

### 5.3 Rank Cap Breach

**Scenario:** Update would cause rank_status > rank_cap

**Solution:**
- Detect during executePartialRefit()
- Fallback to REBUILD
- REBUILD resets rank_status to optimal
- Record in decision metrics for alerting

### 5.4 Worker Crash Recovery

**Scenario:** Worker crashes during long rebuild

**Solution:**
- Checkpoint current state to disk
- On recovery, load checkpoint
- If manifest.write() incomplete, retry
- Idempotent publish ensures safety

---

## 6. Performance Characteristics

### 6.1 Commit Path Overhead

- `appendDelta()`: O(1) append to vector
- Timestamp retrieval: O(1)
- Serialization (if buffering to RocksDB): O(delta_size)
- Expected impact: <1% commit latency increase

### 6.2 Worker Throughput

- **Patch:** O(delta_size) ≈ milliseconds for small deltas
- **Partial Refit:** O(k) ≈ seconds for subset retraining
- **Rebuild:** O(n) ≈ seconds to minutes for full artifact

Throughput goal: 1,000+ deltas/sec per worker thread

### 6.3 Memory Usage

- **Delta Log:** ~1KB per entry, 100K entries ≈ 100MB
- **Manifest Store:** ~10KB per manifest version, 100 artifacts × 10 versions ≈ 10MB
- **Worker Stats:** Negligible

---

## 7. Testing Strategy

### Unit Tests (Covered by #5472)
- Delta log append/extract/GC
- Manifest CAS semantics
- Update decision algorithm
- State machine transitions

### Integration Tests (Planned for Phase 4)
- End-to-end delta → patch/refit/rebuild → publish
- Crash recovery with checkpoints
- Concurrent reader access
- Cascade invalidation

### Benchmarks (Planned for Phase 5)
- Commit path latency overhead
- Worker throughput by update strategy
- Stale artifact backlog growth
- Planner fallback frequency

---

## 8. References

- **Issue:** makr-code/ThemisDB#5471
- **Related Issue:** makr-code/ThemisDB#5472 (tests/benchmarks)
- **Architecture:** `DISTRIBUTED_TENSOR_SHARDING.md`
- **Manifest Schema:** `ARTIFACT_MANIFEST_IMPLEMENTATION.md`
- **Artifact Classes:** `src/distributed_tensor/include/tensor_artifact_classes.h`

---

## 9. Implementation Status

**Phase 2: COMPLETE** ✅

**Files:**
- `src/distributed_tensor/include/tensor_delta_log.h` ✅
- `src/distributed_tensor/src/tensor_delta_log.cc` ✅
- `src/distributed_tensor/include/manifest_store.h` ✅
- `src/distributed_tensor/src/manifest_store.cc` ✅
- `src/distributed_tensor/include/snapshot_update_worker.h` ✅
- `src/distributed_tensor/src/snapshot_update_worker.cc` ✅
- `src/distributed_tensor/include/artifact_invalidation.h` ✅
- `src/distributed_tensor/src/artifact_invalidation.cc` ✅
- `src/distributed_tensor/include/exact_graph_fallback.h` ✅
- `src/distributed_tensor/src/exact_graph_fallback.cc` ✅

**Remaining Phases:**
- Phase 3: Failure Handling & Edge Cases 📋
- Phase 4: Tests 📋 (builds on #5472)
- Phase 5: Performance & Hardening 📋
- Phase 6: Documentation & Acceptance 📋
- Phase 7: Integration 📋

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-03  
**Author:** ThemisDB EPIC 3 Implementation Team
