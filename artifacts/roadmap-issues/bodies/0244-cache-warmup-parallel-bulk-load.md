### Context

This issue implements the roadmap item 'Warmup: Parallel Bulk Load' for the cache domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: Warmup: Parallel Bulk Load

### Goal

Deliver the scoped changes for Warmup: Parallel Bulk Load in src/cache/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Warmup: Parallel Bulk Load
**Priority:** Low
**Target Version:** v1.8.0

`warmup.cpp` (`warmupFromLog`) processes warmup entries sequentially — one line at a time — which limits warmup throughput on startup when the log file has millions of entries.

**Implementation Notes:**
- `[x]` Partition the NDJSON warmup log into N chunks (one per CPU core) and spawn N `std::async` tasks to parse and insert in parallel; use per-shard L1 insertion to avoid contention.
- `[x]` Add `WarmupConfig::max_parallel_workers` (default: `std::thread::hardware_concurrency()`).
- `[x]` Report `warmup_duration_ms` and `warmup_entries_per_second` in the warmup result JSON.

**Performance Targets:**
- Warmup throughput: ≥ 500 K entries/s on a 4-core machine for a 5 M entry log.

---

### Acceptance Criteria

- [x] Partition the NDJSON warmup log into N chunks (one per CPU core) and spawn N `std::async` tasks to parse and insert in parallel; use per-shard L1 insertion to avoid contention.
- [x] Add `WarmupConfig::max_parallel_workers` (default: `std::thread::hardware_concurrency()`).
- [x] Report `warmup_duration_ms` and `warmup_entries_per_second` in the warmup result JSON.
- [x] Warmup throughput: ≥ 500 K entries/s on a 4-core machine for a 5 M entry log.

### Relationships

- Roadmap row: #244 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cache/FUTURE_ENHANCEMENTS.md#warmup-parallel-bulk-load
- Source key: roadmap:244:cache:v1.8.0:warmup-parallel-bulk-load

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:244:cache:v1.8.0:warmup-parallel-bulk-load -->
<!-- roadmap-ref: row=244;module=cache;target=v1.8.0 -->
<!-- roadmap-detail: src/cache/FUTURE_ENHANCEMENTS.md#warmup-parallel-bulk-load -->
