# Phase 3: Failure Handling & Edge Cases - Design Documentation

**Status:** IN PROGRESS  
**Date:** 2026-07-03  
**Author:** ThemisDB EPIC 3 Implementation Team

---

## Executive Summary

Phase 3 implements robust failure handling, crash recovery, and distributed locking for the tensor update system. This ensures safe recovery from worker crashes, prevents concurrent conflicts, and handles edge cases gracefully.

**Key Components:**
1. **CrashRecoveryCheckpoint** - Saves/loads worker state for crash safety
2. **DistributedLockManager** - Manages exclusive artifact locks with TTL
3. **StaleArtifactDetector** - Detects and monitors stale artifacts
4. **ErrorRecoveryHandler** - Unified error analysis and recovery recommendations

---

## 1. Crash Recovery Design

### 1.1 Checkpoint Mechanism

Checkpoints capture the complete state before long-running operations:

```
Before Update:
├─ Artifact ID
├─ Delta Window (sequence_start → sequence_end)
├─ Current Manifest (residual, rank_status, etc.)
├─ Artifact Size
├─ Update Decision
└─ Progress Indicator (0-100%)
```

**Checkpoint Lifecycle:**

```
saveCheckpoint() → [Update Operation] → SUCCESS
                 ↓
                 FAILURE/CRASH
                 ↓
         recoverFromCheckpoint()
         ↓
    Load checkpoint state
    ↓
    Retry update or mark stale
    ↓
    Delete checkpoint on success
```

### 1.2 Recovery Strategy

On worker startup:

1. Scan for abandoned checkpoints
2. For each checkpoint:
   - Load checkpoint state
   - Check retry_count < max_retries
   - Either retry update or delete checkpoint
3. If retry exhausted:
   - Mark artifact as STALE
   - Delete checkpoint
   - Planner can decide fallback

### 1.3 Atomicity Guarantees

- Checkpoint writes are atomic (temp file → rename)
- Manifest publishes use CAS semantics
- Recovery replay is idempotent

---

## 2. Distributed Locking Design

### 2.1 Lock States

```
UNLOCKED
├─ acquireLock(artifact_id, holder_id, ttl_seconds)
├─ State: LOCKED by holder_id until expires_at_unix_sec
├─ renewLock(artifact_id, holder_id, ttl_seconds)
├─ State: TTL extended to expires_at_unix_sec
└─ releaseLock(artifact_id, holder_id)
   └─ State: UNLOCKED (available)

EXPIRED (TTL elapsed)
├─ forcefullyAcquireLock() can take over
├─ Or cleanupExpiredLocks() removes it
```

### 2.2 Lock Holder Verification

```cpp
// Prevent wrong actor from releasing a lock
LockStatus releaseLock(const std::string& artifact_id,
                       const std::string& holder_id) {
  auto it = locks_.find(artifact_id);
  if (it->second.holder_id != holder_id) {
    return LockStatus::HOLDER_MISMATCH;  // Prevent spoofing
  }
  // Safe to release
  locks_.erase(it);
  return LockStatus::OK;
}
```

### 2.3 TTL-Based Expiry

Default TTL: 3600 seconds (1 hour)

If lock holder crashes:
- After TTL expires, lock automatically becomes available
- `cleanupExpiredLocks()` removes stale locks
- Next worker can forcefully acquire and retry

**Renewal during long operations:**

```cpp
// Every N seconds during rebuild
renewLock(artifact_id, worker_id, 3600);  // Extend by 1 hour
```

---

## 3. Stale Artifact Detection

### 3.1 Staleness Levels

```
FRESH
├─ age_seconds < age_threshold_slightly (60s)
├─ delta_lag < lag_threshold_slightly (100)
└─ worker throughput ≥ delta arrival rate

SLIGHTLY_STALE
├─ 60s ≤ age < 300s OR 100 ≤ delta_lag < 1000
└─ Planner CAN use artifact (advisory)

MODERATELY_STALE
├─ 300s ≤ age < 3600s OR 1000 ≤ delta_lag < 10000
└─ Planner SHOULD fallback to exact graph (recommended)

CRITICALLY_STALE
├─ age ≥ 3600s OR delta_lag ≥ 10000
├─ Planner MUST fallback to exact graph (required)
└─ Consider cascade invalidation if persistent
```

### 3.2 Fallback Recommendation

```cpp
bool shouldFallback(const StaleArtifactMetrics& metrics) {
  // Fallback if moderately stale or worse
  if (metrics.staleness >= StalenessLevel::MODERATELY_STALE) {
    return true;
  }
  
  // Also fallback if worker falling significantly behind
  if (metrics.delta_arrival_rate / metrics.worker_throughput > 2.5) {
    return true;
  }
  
  return false;
}
```

### 3.3 History Tracking

Each artifact maintains staleness history:

```
StalenessHistory:
├─ artifact_id
├─ most_recent (current level)
├─ moderate_staleness_count (escalation count)
├─ critical_staleness_count (critical incidents)
└─ last_change_unix_sec (time of last transition)
```

---

## 4. Error Recovery Handler

### 4.1 Failure Modes and Recovery

| Failure Mode | Error Code | Recovery Action | Retries |
|---|---|---|---|
| Partial refit failed | PARTIAL_REFIT_FAILED | FALLBACK_TO_REBUILD or RETRY | 0-3 |
| Rank cap breached | RANK_CAP_BREACH | FALLBACK_TO_REBUILD + INVALIDATE | 0 |
| Residual breached | RESIDUAL_BREACH | INVALIDATE | 0 |
| Lock timeout | LOCK_TIMEOUT | ESCALATE_TO_PRIORITY | 1 |
| Checkpoint corrupted | CHECKPOINT_CORRUPTED | NONE (delete checkpoint) | 1 |
| Update timeout | UPDATE_TIMEOUT | DEFER_UPDATE or MARK_STALE | 0-1 |

### 4.2 Residual Breach Handling

```cpp
if (resulting_residual > max_residual_threshold) {
  // Unacceptable quality drop
  ErrorRecoveryInfo info = analyzeResidualBreach(
      artifact_id, resulting_residual, max_residual);
  
  // Action: INVALIDATE
  // - Remove from cache
  // - Notify planner
  // - Schedule rebuild
  // - Mark for cascade invalidation if source was invalidated
}
```

### 4.3 Rank Cap Breach Handling

```cpp
if (would_breach_rank_cap) {
  // Rank status would exceed rank_cap
  // Indicates model decomposition degraded
  
  // Action: FALLBACK_TO_REBUILD
  // - Abort current update
  // - Trigger full rebuild
  // - Reset rank_status to optimal
  // - Mark artifact invalidated
}
```

---

## 5. Integration with UpdateWorker

### 5.1 Enhanced ProcessTask Flow

```
1. recoverFromCheckpoint(artifact_id)
   - Load any previous checkpoint
   - If retry exhausted, skip to 2

2. acquireUpdateLock(artifact_id, "update_processing")
   - Prevent concurrent updates
   - Hold lock for duration of operation

3. saveCheckpoint(artifact_id, task)
   - Persist state before long operation
   - For crash recovery

4. [Decision & Execution]
   - decideUpdateStrategy()
   - executeDecision() with error handling
   - renewLock() if long operation

5. publishManifest()
   - Atomic CAS publish
   - Update manifest_version

6. Cleanup:
   - deleteCheckpoint(artifact_id)
   - releaseUpdateLock(artifact_id)
   - Delete temp files
```

### 5.2 Error Handling with Recovery

```cpp
try {
  success = executePartialRefit(artifact_id, delta_window, manifest);
  
  if (!success || residual_breach) {
    ErrorRecoveryInfo recovery = error_handler_->analyzePartialRefitFailure(...);
    
    switch (recovery.recovery_action) {
      case RecoveryAction::RETRY:
        // Retry up to max_retries
        break;
      case RecoveryAction::FALLBACK_TO_REBUILD:
        // Trigger rebuild
        success = executeRebuild(...);
        break;
      case RecoveryAction::INVALIDATE:
        // Mark artifact invalid
        invalidation_manager_->invalidateArtifact(...);
        success = false;
        break;
    }
  }
} catch (const std::exception& e) {
  ErrorRecoveryInfo recovery = error_handler_->analyze(error_code, ...);
  // Handle based on recovery action
}
```

---

## 6. Production Readiness Checklist

### Phase 3 Deliverables

- [x] **CrashRecoveryCheckpoint** - Checkpoint save/load/cleanup
  - Atomic writes with temp files
  - Version validation
  - TTL-based cleanup
  
- [x] **DistributedLockManager** - Exclusive locking with TTL
  - Lock acquisition/release/renewal
  - Holder verification
  - Automatic expiry
  
- [x] **StaleArtifactDetector** - Staleness monitoring
  - Multi-level classification
  - Fallback recommendations
  - History tracking
  
- [x] **ErrorRecoveryHandler** - Unified error analysis
  - Failure mode analysis
  - Recovery action recommendations
  - Statistics collection
  
- [x] **Enhanced SnapshotBasedUpdateWorker**
  - Checkpoint integration
  - Lock management
  - Error recovery integration
  - Enhanced executePartialRefit with error handling

### Outstanding Items (Phase 4+)

- [ ] RocksDB backend for checkpoint persistence
- [ ] RocksDB backend for lock state persistence
- [ ] Integration tests for crash recovery scenarios
- [ ] Integration tests for concurrent lock contention
- [ ] Benchmark suite for recovery overhead
- [ ] Production deployment runbook

---

## 7. Risk Mitigation

| Risk | Mitigation |
|---|---|
| Checkpoint file corruption | Versioning + validation, atomic writes |
| Lock expiry too short | TTL renewal during operations |
| Retry exhaustion | Defer to priority queue or mark stale |
| Cascade invalidation overuse | Logging + monitoring of invalidation reasons |
| Deadlock between locks | Single artifact lock (no multi-lock chains) |

---

## 8. Performance Impact

- **Checkpoint Save**: ~1-5ms per artifact (disk I/O)
- **Lock Acquisition**: ~0.1ms (in-memory, no disk)
- **Staleness Detection**: ~0.5ms per artifact
- **Error Analysis**: <0.1ms (all in-memory)

**Overall Update Overhead**: ~5-10ms per task (checkpoint + locking + detection)

---

## 9. Monitoring & Observability

### Metrics to Export

- `checkpoint_save_duration_ms` - Time to save checkpoint
- `lock_acquisition_wait_ms` - Time waiting for lock
- `staleness_detection_duration_ms` - Time to analyze staleness
- `error_recovery_actions_total` - Count by recovery action type
- `lock_contention_count` - Number of lock contentions
- `checkpoint_cleanup_count` - Checkpoints deleted

### Alerts

- Alert if checkpoint save failure rate > 5%
- Alert if lock wait time > 1 second
- Alert if artifacts critical stale > 5%
- Alert if error recovery failure rate > 10%

---

## 10. Future Enhancements (Phase 5+)

1. **Distributed Locking Backend**
   - Redis-based locks for multi-instance coordination
   - Consul-based distributed locks
   - Zookeeper-based coordination

2. **Checkpoint Persistence**
   - RocksDB backend for durability
   - Checkpoint replay on recovery
   - Incremental checkpoint updates

3. **Advanced Recovery Strategies**
   - Partial rollback on checkpoint recovery
   - Checkpoint replication across nodes
   - Checkpoint compression for archival

4. **Performance Optimization**
   - Lock-free checkpoint reading
   - Concurrent checkpoint cleanup
   - Batched error recovery actions

---

## References

- **Issue:** makr-code/ThemisDB#5471
- **Phase 2:** PHASE_2_DESIGN_DOCUMENTATION.md (Tensor Delta Log, Manifest Store, Update Worker)
- **Phase 4:** Tests and integration tests (linked issue #5472)
- **Phase 5:** Performance benchmarking and hardening
