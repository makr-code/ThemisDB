# Tensor Artifact Recovery Procedures

**Status:** Engineering Guidance Document  
**Date:** 2026-07-05  
**Related Issue:** makr-code/ThemisDB#5433

---

## Purpose

This document provides implementation guidance for recovery, rebuild, and crash-safe procedures for distributed tensor artifacts. It complements the high-level policy defined in `EPIC3_RECOVERY_STRATEGY.md`.

---

## 1. Recovery Manager Architecture

### 1.1 High-Level Components

```
┌─────────────────────────────────────────────────────────────┐
│            Query Planner / LLM Stack                        │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│            Recovery Status Query API                        │
├──────────────────────────────────────────────────────────────┤
│  - GetArtifactStatus()                                       │
│  - GetRecoveryMetrics()                                      │
│  - GetFreshnessDebt()                                        │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│          RecoveryManager (Central Orchestration)            │
├──────────────────────────────────────────────────────────────┤
│  - Route failure → recovery action                           │
│  - Decide rebuild mode (patch/refit/snapshot/invalidate)    │
│  - Priority queue & scheduling                              │
│  - Freshness debt tracking                                  │
│  - Crash recovery coordination                              │
└──────────────────────┬──────────────────────────────────────┘
          ┌────────────┼────────────┐
          │            │            │
    ┌─────▼──────┐ ┌──▼─────────┐ ┌─▼─────────────┐
    │ RebuildMgr │ │ PatchMgr   │ │CrashRecovery │
    │            │ │            │ │              │
    │ - Snapshot │ │ - Delta    │ │ - WAL Replay │
    │ - Refit    │ │ - Apply    │ │ - Lock mgmt  │
    └──────┬──────┘ └──┬────────┘ └─┬────────────┘
           │           │           │
    ┌──────▼───────────▼───────────▼──────────────┐
    │     Artifact Manifest (Manifest Store)      │
    │     (Version tracking, locking, receipts)   │
    └─────────────────────────────────────────────┘
           │           │           │
    ┌──────▼───────────▼───────────▼──────────────┐
    │         Distributed Storage Layer           │
    │  (Replicas, shards, lineage sources)        │
    └─────────────────────────────────────────────┘
```

### 1.2 Core Abstractions

#### RecoveryStatus
```cpp
enum class RecoveryStatus {
  HEALTHY,           // Artifact available, replicated, fresh
  DEGRADED,          // Available but with reduced redundancy
  RECOVERING,        // Active recovery in progress
  FAILED,            // Recovery failed; unable to proceed
  UNAVAILABLE,       // Artifact permanently lost or inaccessible
};

struct RecoveryMetrics {
  RecoveryStatus status;
  RecoveryMode active_mode;
  Duration elapsed_time;
  double progress_percent;  // 0.0–100.0
  int replica_count;
  bool is_fresh;
  Duration staleness_age;
  vector<string> failure_reasons;
};
```

#### RebuildPriority
```cpp
enum class RebuildPriority {
  CRITICAL,   // Artifact required for active query
  HIGH,       // Artifact affects query performance
  NORMAL,     // Standard maintenance
  LOW,        // Background / optional
};

struct PriorityQueueEntry {
  ArtifactId artifact_id;
  RebuildPriority priority;
  Duration staleness;
  int dependency_count;  // Artifacts that depend on this
  RecoveryMode preferred_mode;
};
```

---

## 2. Recovery Decision Tree

### 2.1 Failure Detection and Classification

Upon detecting artifact unavailability:

```cpp
Future<RecoveryDecision> ClassifyFailure(ArtifactId id) {
  auto artifact = co_await manifest_.Get(id);
  auto replicas = co_await shard_cluster_.ProbeReplicas(id);
  
  if (replicas.healthy_count == 0) {
    // Complete loss
    if (artifact.has_lineage) {
      return RecoveryDecision{
        .mode = RebuildMode::SNAPSHOT,
        .priority = Priority::CRITICAL,
        .fallback = Fallback::GRAPH_VALIDATION,
      };
    } else {
      return RecoveryDecision{
        .mode = RebuildMode::NONE,
        .priority = Priority::CRITICAL,
        .fallback = Fallback::GRAPH_VALIDATION,
      };
    }
  } else if (replicas.corrupted_count > 0) {
    // Partial corruption
    auto fresh_count = replicas.healthy_count - replicas.corrupted_count;
    if (fresh_count > 0) {
      return RecoveryDecision{
        .mode = RebuildMode::PATCH,  // Promote from replica
        .priority = Priority::HIGH,
        .fallback = Fallback::REPLICA_FALLBACK,
      };
    } else {
      return RecoveryDecision{
        .mode = RebuildMode::SNAPSHOT,
        .priority = Priority::CRITICAL,
        .fallback = Fallback::GRAPH_VALIDATION,
      };
    }
  } else {
    // Transient unavailability or stale
    auto staleness = Clock::now() - artifact.last_updated;
    if (staleness > max_staleness_) {
      return RecoveryDecision{
        .mode = RebuildMode::INVALIDATE,
        .priority = Priority::HIGH,
        .fallback = Fallback::GRAPH_VALIDATION,
      };
    } else {
      // Transient; will retry
      return RecoveryDecision{
        .mode = RebuildMode::NONE,
        .priority = Priority::LOW,
        .fallback = Fallback::WAIT_AND_RETRY,
      };
    }
  }
}
```

### 2.2 Mode Selection

After classification, select rebuild mode:

```cpp
RebuildMode SelectRebuildMode(
    const RecoveryDecision& decision,
    const RebuildPolicy& policy) {
  
  // 1. Check policy preference for this artifact class
  auto class_pref = policy.mode_prefs[artifact.class_];
  
  // 2. Check if preconditions are met
  if (!CheckModePreconditions(RebuildMode::PATCH)) {
    // Try next mode
  }
  
  // 3. Estimate cost vs. benefit
  auto patch_cost = EstimatePatc Cost(artifact);
  auto snapshot_cost = EstimateSnapshotCost(artifact);
  
  // 4. Choose based on cost and latency requirement
  if (decision.priority == Priority::CRITICAL) {
    if (patch_cost < snapshot_cost) {
      return RebuildMode::PATCH;
    } else {
      return RebuildMode::SNAPSHOT;
    }
  } else {
    // Non-critical: prefer lazy invalidation
    return RebuildMode::INVALIDATE;
  }
}
```

---

## 3. Patch Recovery (Mode A)

### 3.1 Delta Application

Delta patches are incremental updates suitable for frequent modifications:

```cpp
Future<void> ApplyDelta(
    ArtifactId id,
    const ArtifactDelta& delta,
    const WriteOptions& opts = {}) {
  
  co_await manifest_.LockArtifact(id);
  auto guard = co_await manifest_.LockGuard(id);
  
  try {
    // 1. Verify base artifact
    auto base = co_await shard_cluster_.LoadArtifact(id);
    if (!VerifyIntegrity(base, manifest_.Get(id).hash)) {
      throw ArtifactCorruptedException("Base artifact corrupted");
    }
    
    // 2. Validate delta applicability
    if (delta.base_version != base.version) {
      throw VersionMismatchException("Delta base version mismatch");
    }
    
    // 3. Apply delta incrementally
    auto patched = co_await ApplyDeltaIncrementally(base, delta);
    
    // 4. Verify patched artifact
    auto patched_hash = ComputeHash(patched);
    if (patched_hash != delta.expected_hash) {
      throw IntegrityFailedException("Patched artifact hash mismatch");
    }
    
    // 5. Write-ahead log
    auto wal_entry = CreateWALEntry(id, delta, patched_hash);
    co_await wal_.Append(wal_entry);
    
    // 6. Distribute patched artifact
    co_await shard_cluster_.StoreArtifact(id, patched, opts);
    
    // 7. Update manifest (atomic)
    co_await manifest_.Update(id, {
        .version = base.version + 1,
        .hash = patched_hash,
        .last_updated = Clock::now(),
        .patched = true,
    });
    
    // 8. Clear WAL entry
    co_await wal_.Commit(wal_entry.id);
    
  } catch (const std::exception& e) {
    // 9. Rollback on error
    co_await manifest_.Unlock(id);
    throw;
  }
  
  co_await manifest_.Unlock(id);
}
```

### 3.2 Delta Coalescing

To reduce patching overhead, coalesce multiple small deltas:

```cpp
struct DeltaCoalescePolicy {
  int max_pending_deltas = 10;
  Duration max_delta_age = 100ms;
  size_t max_total_size_bytes = 10MB;
};

Future<ArtifactDelta> CoalescePendingDeltas(
    ArtifactId id,
    const DeltaCoalescePolicy& policy) {
  
  auto pending = co_await delta_queue_.Peek(id, policy.max_pending_deltas);
  
  if (pending.size() == 0) {
    return std::nullopt;
  }
  
  // Check if coalescing conditions are met
  auto oldest_age = Clock::now() - pending.front().timestamp;
  auto total_size = std::accumulate(
      pending.begin(), pending.end(), 0,
      [](auto sum, const auto& d) { return sum + d.size_bytes; });
  
  if (oldest_age < policy.max_delta_age &&
      total_size < policy.max_total_size_bytes &&
      pending.size() < policy.max_pending_deltas) {
    // Not yet time to coalesce
    return std::nullopt;
  }
  
  // Merge all deltas into single composite delta
  return co_await MergeDeltas(pending);
}
```

---

## 4. Partial Refit (Mode B)

### 4.1 Factorization-Aware Refit

For tensor artifacts with factor structure (TT cores, HT subtrees):

```cpp
struct FactorRefitPlan {
  vector<FactorId> factors_to_refit;
  vector<FactorId> factors_to_keep;
  ArtifactDelta refitting_delta;
  vector<CheckpointId> checkpoints;  // Resume points
};

Future<FactorRefitPlan> PlanPartialRefit(
    ArtifactId tensor_id,
    const RefitRequest& request) {
  
  auto manifest = co_await manifest_.Get(tensor_id);
  auto factors = co_await shard_cluster_.ListFactors(tensor_id);
  
  // 1. Determine which factors need refitting
  vector<FactorId> to_refit, to_keep;
  for (const auto& factor : factors) {
    if (request.requires_refit(factor.id)) {
      to_refit.push_back(factor.id);
    } else if (factor.is_healthy && !factor.is_stale) {
      to_keep.push_back(factor.id);
    }
  }
  
  // 2. Plan refitting stages
  auto refitting_delta = request.BuildDelta(to_refit);
  
  // 3. Plan checkpoints for resumability
  int checkpoint_interval = to_refit.size() / 4;  // 4 checkpoints
  vector<CheckpointId> checkpoints;
  for (int i = 0; i < to_refit.size(); i += checkpoint_interval) {
    checkpoints.push_back(CreateCheckpoint(i));
  }
  
  return FactorRefitPlan{
      .factors_to_refit = to_refit,
      .factors_to_keep = to_keep,
      .refitting_delta = refitting_delta,
      .checkpoints = checkpoints,
  };
}

Future<void> ExecutePartialRefit(
    ArtifactId tensor_id,
    const FactorRefitPlan& plan) {
  
  co_await manifest_.LockArtifact(tensor_id);
  auto guard = co_await manifest_.LockGuard(tensor_id);
  
  try {
    // 1. Load factors to keep (verify integrity)
    auto kept_factors = co_await shard_cluster_.LoadFactors(
        tensor_id, plan.factors_to_keep);
    for (const auto& factor : kept_factors) {
      if (!VerifyIntegrity(factor)) {
        throw IntegrityFailedException("Factor corrupted: " + factor.id);
      }
    }
    
    // 2. Refit factors in stages
    vector<Factor> refitted;
    for (int i = 0; i < plan.factors_to_refit.size(); ++i) {
      auto factor_id = plan.factors_to_refit[i];
      
      // Load old factor if exists
      auto old_factor = co_await shard_cluster_.LoadFactor(factor_id);
      
      // Refit using delta
      auto new_factor = co_await ApplyRefitDelta(
          old_factor, plan.refitting_delta);
      refitted.push_back(new_factor);
      
      // Checkpoint periodically
      if (i % plan.checkpoints.size() == 0) {
        co_await CreateCheckpoint(plan.checkpoints[i / plan.checkpoints.size()]);
      }
    }
    
    // 3. Assemble refitted tensor
    auto refitted_tensor = co_await AssembleFactors(
        kept_factors, refitted);
    
    // 4. Verify composite tensor
    auto new_hash = ComputeHash(refitted_tensor);
    if (new_hash != plan.refitting_delta.expected_hash) {
      throw IntegrityFailedException("Refitted tensor hash mismatch");
    }
    
    // 5. Distribute refitted tensor and factors
    co_await shard_cluster_.StoreArtifact(tensor_id, refitted_tensor);
    for (const auto& factor : refitted) {
      co_await shard_cluster_.StoreFactor(tensor_id, factor);
    }
    
    // 6. Update manifest
    co_await manifest_.Update(tensor_id, {
        .version = old_manifest.version + 1,
        .hash = new_hash,
        .last_updated = Clock::now(),
        .refit_factors = plan.factors_to_refit.size(),
    });
    
  } catch (const std::exception& e) {
    // Rollback
    co_await manifest_.Unlock(tensor_id);
    throw;
  }
  
  co_await manifest_.Unlock(tensor_id);
}
```

---

## 5. Snapshot Rebuild (Mode C)

### 5.1 Lineage-Based Reconstruction

For complete artifacts requiring full reconstruction from source:

```cpp
struct RebuildSourceInfo {
  enum class SourceType { PACKAGE_LINEAGE, EXACT_ARTIFACT, REPLICATION };
  
  SourceType type;
  PackageId source_package;  // If PACKAGE_LINEAGE
  ArtifactId source_artifact;  // If EXACT_ARTIFACT
  ShardId replica_shard;  // If REPLICATION
  vector<string> required_operations;
  Duration estimated_time;
};

Future<RebuildSourceInfo> DetermineRebuildSource(ArtifactId id) {
  auto manifest = co_await manifest_.Get(id);
  
  // 1. Try replica first (fastest)
  auto replicas = co_await shard_cluster_.ProbeReplicas(id);
  if (replicas.healthy_count > 0) {
    auto replica_shard = replicas.healthy_shards.front();
    return RebuildSourceInfo{
        .type = SourceType::REPLICATION,
        .replica_shard = replica_shard,
        .estimated_time = Duration::milliseconds(100),
    };
  }
  
  // 2. Check for exact artifact source
  if (manifest.source_artifact_id != "") {
    auto source = co_await manifest_.Get(manifest.source_artifact_id);
    if (source.is_available) {
      return RebuildSourceInfo{
          .type = SourceType::EXACT_ARTIFACT,
          .source_artifact = manifest.source_artifact_id,
          .estimated_time = Duration::seconds(5),
      };
    }
  }
  
  // 3. Fall back to package lineage
  if (manifest.package_lineage.size() > 0) {
    return RebuildSourceInfo{
        .type = SourceType::PACKAGE_LINEAGE,
        .source_package = manifest.package_lineage.front(),
        .required_operations = manifest.rebuild_operations,
        .estimated_time = Duration::seconds(60),
    };
  }
  
  throw RebuildSourceUnavailableException(
      "No rebuild source available for artifact: " + id.str());
}

Future<void> RebuildFromLineage(
    ArtifactId id,
    const PackageId& source_package) {
  
  co_await manifest_.LockArtifact(id);
  auto guard = co_await manifest_.LockGuard(id);
  
  try {
    auto manifest = co_await manifest_.Get(id);
    
    // 1. Load source package
    auto package = co_await artifact_store_.LoadPackage(source_package);
    if (!package.is_complete) {
      throw PackageIncompleteException("Source package incomplete");
    }
    
    // 2. Execute rebuild operations
    auto intermediate = package.source_data;
    for (const auto& op : manifest.rebuild_operations) {
      // Execute transformation (e.g., factorization, quantization)
      intermediate = co_await ExecuteRebuildOp(op, intermediate);
      
      // Create checkpoint
      co_await CreateCheckpoint({
          .operation = op,
          .intermediate_size = intermediate.size_bytes,
      });
    }
    
    // 3. Verify rebuilt artifact
    auto rebuilt_hash = ComputeHash(intermediate);
    if (rebuilt_hash != manifest.expected_hash) {
      throw IntegrityFailedException(
          "Rebuilt artifact hash mismatch: " +
          rebuilt_hash.str() + " vs " + manifest.expected_hash.str());
    }
    
    // 4. Write-ahead log
    auto wal_entry = CreateWALEntry(id, "rebuild", rebuilt_hash);
    co_await wal_.Append(wal_entry);
    
    // 5. Distribute rebuilt artifact
    co_await shard_cluster_.StoreArtifact(id, intermediate);
    
    // 6. Update manifest
    co_await manifest_.Update(id, {
        .version = manifest.version + 1,
        .hash = rebuilt_hash,
        .last_updated = Clock::now(),
        .rebuilt_from_lineage = true,
    });
    
    // 7. Commit WAL
    co_await wal_.Commit(wal_entry.id);
    
  } catch (const std::exception& e) {
    co_await manifest_.Unlock(id);
    throw;
  }
  
  co_await manifest_.Unlock(id);
}
```

---

## 6. Crash Recovery (Idempotent Resume)

### 6.1 Write-Ahead Log Structure

```cpp
struct WALEntry {
  using EntryId = std::uint64_t;
  
  EntryId id;
  std::chrono::system_clock::time_point timestamp;
  
  enum class OpType { UPDATE, REBUILD, GC, INVALIDATE };
  OpType op_type;
  
  ArtifactId artifact_id;
  std::string manifest_version_before;
  std::string manifest_version_after;
  
  // Operation-specific checkpoint data
  std::string checkpoint_data;  // Serialized state
  
  bool is_committed = false;
};
```

### 6.2 Recovery Replay

Upon worker startup:

```cpp
Future<void> ReplayFromCrash(const std::vector<WALEntry>& log) {
  
  for (const auto& entry : log) {
    if (entry.is_committed) {
      // Already committed; skip
      continue;
    }
    
    try {
      switch (entry.op_type) {
        case OpType::UPDATE:
          co_await ReplayUpdate(entry);
          break;
        case OpType::REBUILD:
          co_await ReplayRebuild(entry);
          break;
        case OpType::GC:
          co_await ReplayGC(entry);
          break;
        case OpType::INVALIDATE:
          co_await ReplayInvalidate(entry);
          break;
      }
    } catch (const std::exception& e) {
      // Log error and abort this entry; continue with next
      logger_.error("Failed to replay WAL entry {}: {}", entry.id, e.what());
      continue;
    }
  }
}

Future<void> ReplayUpdate(const WALEntry& entry) {
  auto artifact = co_await manifest_.Get(entry.artifact_id);
  
  // Check if update already applied
  if (artifact.version > std::stoi(entry.manifest_version_before)) {
    // Update already applied; skip
    return;
  }
  
  // Resume update from checkpoint
  auto checkpoint = DeserializeCheckpoint(entry.checkpoint_data);
  co_await ResumeDeltaApplication(entry.artifact_id, checkpoint);
}

Future<void> ReplayRebuild(const WALEntry& entry) {
  auto artifact = co_await manifest_.Get(entry.artifact_id);
  
  // Check if rebuild already completed
  if (artifact.version > std::stoi(entry.manifest_version_before)) {
    // Rebuild already completed; skip
    return;
  }
  
  // Resume rebuild from checkpoint
  auto checkpoint = DeserializeCheckpoint(entry.checkpoint_data);
  co_await ResumeRebuild(entry.artifact_id, checkpoint);
}
```

### 6.3 Idempotency Guarantees

For crash safety, recovery operations must be idempotent:

1. **Manifest version**: Always checked before applying operation
2. **Hash verification**: Rebuilt/patched artifact verified before promoting
3. **Atomic promotion**: Move to live location only after verification
4. **Duplicate detection**: WAL prevents double-application via version check

---

## 7. Freshness Debt Tracking

### 7.1 Debt Accumulation

```cpp
struct FreshnessDebtEntry {
  ArtifactId artifact_id;
  std::chrono::system_clock::time_point stale_since;
  double impact_factor;  // Query-impact weight (0.0–1.0)
};

struct FreshnessDebtState {
  std::vector<FreshnessDebtEntry> stale_artifacts;
  double total_debt = 0.0;  // Sum of (age × impact) for all stale artifacts
  std::chrono::duration<double> max_staleness;
  
  // Thresholds
  double yellow_threshold = 0.05;  // 5%
  double red_threshold = 0.20;     // 20%
};

Future<void> TrackFreshnessDebt() {
  while (is_running_) {
    co_await Sleep(Duration::seconds(1));
    
    auto now = Clock::now();
    double cumulative_debt = 0.0;
    
    for (auto& entry : debt_state_.stale_artifacts) {
      auto age = now - entry.stale_since;
      auto age_seconds = age.count() / 1000.0;  // milliseconds → seconds
      
      // Debt increases linearly with staleness age
      cumulative_debt += age_seconds * entry.impact_factor;
    }
    
    debt_state_.total_debt = cumulative_debt;
    
    // Check thresholds
    double debt_ratio = cumulative_debt / max_debt_capacity_;
    if (debt_ratio > debt_state_.red_threshold) {
      co_await HandleRedThreshold();
    } else if (debt_ratio > debt_state_.yellow_threshold) {
      logger_.warn("Freshness debt at yellow threshold: {:.1%}", debt_ratio);
    }
  }
}

Future<void> HandleRedThreshold() {
  logger_.error("Freshness debt exceeded red threshold!");
  
  // 1. Prioritize invalidation of oldest stale artifacts
  std::sort(debt_state_.stale_artifacts.begin(),
            debt_state_.stale_artifacts.end(),
            [](const auto& a, const auto& b) {
              return a.stale_since < b.stale_since;
            });
  
  // 2. Invalidate to trigger re-materialization
  for (const auto& entry : debt_state_.stale_artifacts) {
    co_await InvalidateArtifact(entry.artifact_id);
  }
  
  // 3. Alert operator
  metrics_.debt_red_threshold_violations++;
  notifications_.Alert(AlertLevel::ERROR,
      "Freshness debt exceeded; artifact invalidation triggered");
}
```

---

## 8. Observability and Monitoring

### 8.1 Key Metrics

```cpp
struct RecoveryMetrics {
  // Latency metrics (milliseconds)
  prometheus::Summary recovery_latency_ms;    // p50, p99, p999
  prometheus::Summary rebuild_latency_ms;
  prometheus::Summary crash_recovery_time_ms;
  
  // Cost metrics
  prometheus::Counter bytes_recovered_total;
  prometheus::Counter bytes_transferred_total;
  prometheus::Counter rebuild_operations_total;
  
  // Freshness metrics
  prometheus::Gauge freshness_debt_current;
  prometheus::Counter freshness_debt_red_breaches;
  prometheus::Histogram artifact_staleness_hours;
  
  // Replica consistency
  prometheus::Gauge replica_lag_ms;
  prometheus::Counter replica_sync_failures;
  
  // Crash recovery
  prometheus::Counter crash_recoveries_total;
  prometheus::Counter crash_recovery_failures;
  prometheus::Histogram wal_replay_time_ms;
};
```

### 8.2 Example Observability Integration

```cpp
// Register metrics with Prometheus
recovery_metrics_.recovery_latency_ms = prometheus_registry_.Add(
    prometheus::Summary{
        .help = "Time to recover artifact (ms)",
        .name = "tensor_recovery_latency_ms",
        .labels = {"artifact_class", "failure_type", "recovery_mode"},
    });

// Record metric during recovery
auto start_time = Clock::now();
co_await RecoverArtifact(artifact_id, mode);
auto duration_ms = (Clock::now() - start_time).count() / 1000.0;

recovery_metrics_.recovery_latency_ms.observe(duration_ms,
    {artifact.class_str(), failure_reason, mode_str()});
```

---

## 9. Checklist for Implementation

- [ ] `recovery_manager.h`: Define `RecoveryManager` class and core APIs
- [ ] `rebuild_policy.h`: Define rebuild mode selection logic
- [ ] `crash_recovery.h`: Define WAL replay and idempotent resume
- [ ] `recovery_manager.cc`: Implement recovery orchestration
- [ ] `rebuild_policy.cc`: Implement mode selection algorithms
- [ ] `crash_recovery.cc`: Implement WAL replay and recovery
- [ ] Unit tests for all three modules
- [ ] Integration tests with fault injection
- [ ] Benchmark suite for recovery latency and cost
- [ ] Observability integration with Prometheus
- [ ] Operational runbooks for common recovery scenarios

---

## References

- `EPIC3_RECOVERY_STRATEGY.md` (High-level policy)
- `DISTRIBUTED_TENSOR_SHARDING.md`
- `EPIC3_MANIFEST_SCHEMA.md`
- `EPIC3_ARTIFACT_CLASSES.md`
