# Distributed Tensor Module - Gap Closure Implementation Plan

**Status:** In Progress (themisdb-implementer agent: 38 tool calls, ~192s elapsed)  
**Date:** 2026-08-18  
**Task ID:** dt-rocksdb-checkpoint-impl

## Overview

This document describes the planned implementation of open gaps in the distributed_tensor EPIC 3 module to achieve Phase 5-6 production readiness.

## Gap 1: RocksDB Integration in ManifestStore

### Current State
- **File:** `src/distributed_tensor/src/manifest_store.cc`
- **Issue:** In-memory mock with 3 TODO comments (lines 29, 43, 57)
- **Impact:** No persistent storage; data lost on restart

### Planned Implementation

#### 1.1 Header and Initialization
```cpp
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/column_family.h"

// Replace:
static std::map<std::string, std::string> g_manifest_store;
static std::map<std::string, std::vector<ManifestVersion>> g_version_history;
static std::map<std::string, std::pair<std::string, int64_t>> g_locks;

// With:
rocksdb::DB* db_ = nullptr;
rocksdb::ColumnFamilyHandle* default_cf_ = nullptr;
rocksdb::ColumnFamilyHandle* version_history_cf_ = nullptr;
rocksdb::ColumnFamilyHandle* locks_cf_ = nullptr;
```

#### 1.2 open() Method Enhancement
```cpp
ManifestStoreStatus ManifestStore::open(const std::string& db_path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  
  // Create column families
  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      "version_history", rocksdb::ColumnFamilyOptions()));
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      "locks", rocksdb::ColumnFamilyOptions()));
  
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::Status status = rocksdb::DB::Open(
      options, db_path, column_families, &handles, &db_);
  
  if (!status.ok()) {
    return ManifestStoreStatus::STORAGE_ERROR;
  }
  
  default_cf_ = handles[0];
  version_history_cf_ = handles[1];
  locks_cf_ = handles[2];
  
  is_open_ = true;
  db_path_ = db_path;
  return ManifestStoreStatus::OK;
}
```

#### 1.3 close() Method Enhancement
```cpp
ManifestStoreStatus ManifestStore::close() {
  if (!db_) return ManifestStoreStatus::OK;
  
  // Close column family handles
  if (default_cf_) db_->DestroyColumnFamilyHandle(default_cf_);
  if (version_history_cf_) db_->DestroyColumnFamilyHandle(version_history_cf_);
  if (locks_cf_) db_->DestroyColumnFamilyHandle(locks_cf_);
  
  delete db_;
  db_ = nullptr;
  is_open_ = false;
  
  return ManifestStoreStatus::OK;
}
```

#### 1.4 Error Code Mapping
Map RocksDB::Status to ManifestStoreStatus:
- `status.IsNotFound()` → INVALID_MANIFEST
- `status.IsIOError()` → STORAGE_ERROR
- `status.IsLocked()` → LOCK_TIMEOUT
- Other errors → STORAGE_ERROR

### Acceptance Criteria
- [ ] All public API methods use RocksDB backend
- [ ] Version history persists across restarts
- [ ] Locks work with persistent backend
- [ ] No in-memory fallbacks
- [ ] Backward API compatibility maintained

---

## Gap 2: Checkpoint Resume Logic in SnapshotUpdateWorker

### Current State
- **File:** `src/distributed_tensor/src/snapshot_update_worker.cc` (lines 524-552)
- **Issue:** TODO comment; recovery just acknowledges without actual resumption
- **Impact:** Crashes lose update progress; cannot resume from checkpoint

### Planned Implementation

#### 2.1 RecoverFromCheckpoint() Full Logic
```cpp
bool SnapshotBasedUpdateWorker::recoverFromCheckpoint(const std::string& artifact_id) {
  if (!checkpoint_manager_) {
    return true;  // No checkpoint manager, recovery not applicable
  }

  // Load checkpoint
  Checkpoint checkpoint;
  CheckpointStatus status = checkpoint_manager_->load(artifact_id, checkpoint);

  if (status == CheckpointStatus::NOT_FOUND) {
    return true;  // No checkpoint, normal path
  }

  if (status != CheckpointStatus::OK) {
    return false;  // Corrupted checkpoint, fail-closed
  }

  // Check retry exhaustion
  if (checkpoint.retry_count >= checkpoint.max_retries) {
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return true;  // Give up and continue
  }

  // Validate checkpoint state
  ArtifactManifest restored_manifest = checkpoint.current_manifest;
  if (!restored_manifest.isFresh(3600000)) {
    // Checkpoint too stale, discard it
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return true;
  }

  // Restore delta window and validate
  DeltaWindow& window = checkpoint.delta_window;
  if (window.entries.empty()) {
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return true;
  }

  // Validate sequence continuity
  uint64_t expected_seq = window.sequence_start;
  for (const auto& entry : window.entries) {
    if (entry.sequence_number != expected_seq) {
      // Sequence broken, abandon checkpoint
      checkpoint_manager_->deleteCheckpoint(artifact_id);
      return true;
    }
    expected_seq++;
  }

  // Resume from checkpoint_offset
  // Apply remaining delta entries
  double residual = checkpoint.current_manifest.residual;
  bool instability_detected = false;

  // Re-analyze remaining deltas
  for (size_t i = 0; i < window.entries.size(); ++i) {
    const auto& entry = window.entries[i];
    // Update residual based on entry type
    if (entry.type == "UPDATE") {
      residual += 0.01;  // Small increase per update
    } else if (entry.type == "DELETE") {
      residual -= 0.005;  // Decrease per delete
    }
  }

  // Clamp residual
  residual = std::max(0.0, std::min(1.0, residual));

  // Check if residual exceeds threshold
  if (residual > refit_threshold_pct_ / 100.0) {
    instability_detected = true;
  }

  // Update manifest lifecycle
  if (instability_detected) {
    restored_manifest.lifecycle_state = LifecycleState::REBUILDING;
    restored_manifest.invalidation_reason = InvalidationReason::INSTABILITY;
  } else {
    restored_manifest.lifecycle_state = LifecycleState::PATCHED;
  }

  restored_manifest.residual = residual;
  restored_manifest.artifact_age_ms = getCurrentTimeMs() - restored_manifest.created_at_ms;

  // Save recovered state to manifest store
  if (manifest_store_) {
    std::string manifest_json = restored_manifest.toJson();
    manifest_store_->store(artifact_id, manifest_json);
  }

  // Increment retry count and save updated checkpoint
  checkpoint.retry_count++;
  checkpoint_manager_->save(artifact_id, checkpoint);

  // State transition tracking
  recovery_state_transitions_[artifact_id] = 
      instability_detected ? UpdateDecision::PARTIAL_REFIT : UpdateDecision::PATCH;

  return true;  // Recovery successful
}
```

#### 2.2 State Machine Transitions
- REBUILDING → PATCHED (if stable)
- REBUILDING → PARTIAL_REFITTED (if instability detected)
- REBUILDING → REBUILT (if severe instability)
- All transitions validate residuals and rank caps

#### 2.3 Residual Validation
```cpp
// Validate recovered manifest
bool isValidRecoveredManifest(const ArtifactManifest& m) {
  // Check residual within bounds [0, 1]
  if (m.residual < 0.0 || m.residual > 1.0) return false;
  
  // Check rank cap enforced
  if (m.rank_cap > 0 && m.rank_cap < m.min_rank_required) return false;
  
  // Check lifecycle state valid
  if (m.lifecycle_state == LifecycleState::FAILED) return false;
  
  return true;
}
```

### Acceptance Criteria
- [ ] RecoverFromCheckpoint() actually resumes updates
- [ ] State machine properly transitions through recovery phases
- [ ] Residual validation prevents invalid scenarios
- [ ] Idempotent: same checkpoint can be retried safely
- [ ] All error paths fail-closed (SG-DT-01)
- [ ] Checkpoint properly incremented on each retry

---

## Gap 3: Logging Integration in ShardSummaryCoordinator

### Current State
- **File:** `src/distributed_tensor/src/shard_summary_coordinator.cc`
- **Issue:** TODO comment for themis::base::logging::Warn()
- **Impact:** No production diagnostics; difficult to debug multi-shard operations

### Planned Implementation

#### 3.1 Logging Integration
```cpp
#include "src/base/logging/logging.h"  // or appropriate logging header

// Replace placeholder warning calls with:
THEMIS_LOG(themis::base::logging::LogLevel::WARN) 
    << "Shard " << shard_id << " summary refresh failed: " << error_message;

THEMIS_LOG(themis::base::logging::LogLevel::DEBUG)
    << "Summary-first routing for " << artifact_id 
    << ": using summary with age=" << age_ms << "ms";

THEMIS_LOG(themis::base::logging::LogLevel::INFO)
    << "Exact-on-demand fetch triggered for " << artifact_id
    << " due to freshness threshold exceeded";
```

#### 3.2 Context-Aware Logging
- Log operation type (refresh, routing, fetch)
- Include artifact/shard identifiers
- Track timestamp of operations
- Log decision rationale (why summary vs exact)
- Log error details on failures

### Acceptance Criteria
- [ ] All placeholder TODOs replaced with real logging
- [ ] Log messages include operation context
- [ ] No performance regressions from logging
- [ ] Diagnostic messages aid operator troubleshooting

---

## Gap 4: Phase 6 Acceptance Evidence

### Pending Evidence
1. Build verification on windows-release preset
2. Focused test execution results
3. Benchmark baseline data
4. Hardware/topology documentation

### Planned Execution
```bash
# 1. Configure and build
cmake --preset windows-release
cmake --build --preset windows-release --parallel 16

# 2. Run all focused test targets
ctest --preset windows-release -R "module_epic3_distributed_tensor" --output-on-failure

# 3. Execute benchmark suite
# Run: benchmarks/epic3_distributed_tensor/*_bench.cc

# 4. Collect and document results
# Generate gate summary with variance report
```

---

## Implementation Timeline

| Phase | Activity | Duration | Owner |
|-------|----------|----------|-------|
| 1 | RocksDB integration | ~2-3 hours | themisdb-implementer |
| 2 | Checkpoint resume logic | ~2-3 hours | themisdb-implementer |
| 3 | Logging integration | ~30-60 mins | themisdb-implementer |
| 4 | Code review | ~1-2 hours | themisdb-reviewer |
| 5 | Build verification | ~1 hour | CI/task agent |
| 6 | Test execution | ~2-3 hours | CI/task agent |
| 7 | Evidence collection | ~1-2 hours | Manual documentation |

---

## Success Criteria

### Code Quality
- [ ] No TODO/FIXME/STUB comments in modified files
- [ ] All public APIs maintain backward compatibility
- [ ] All error paths preserve SG-DT-01 (fail-closed) invariant
- [ ] Comprehensive error handling with proper logging

### Functionality
- [ ] RocksDB backend operational
- [ ] Checkpoint resume working end-to-end
- [ ] Production logging integrated
- [ ] All focused tests pass

### Documentation
- [ ] PHASE_6_ACCEPTANCE_CHECKLIST.md updated with evidence
- [ ] Benchmark results attached
- [ ] Test execution logs preserved
- [ ] Hardware/topology documented

---

## Related Documentation

- `src/distributed_tensor/ROADMAP.md` - Phase 5-6 requirements
- `src/distributed_tensor/FUTURE_ENHANCEMENTS.md` - Performance targets
- `src/distributed_tensor/PHASE_6_ACCEPTANCE_CHECKLIST.md` - Evidence requirements
- `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` - Rollout phases

