### Context

This issue implements the roadmap item 'Changefeed Sequence Counter: RocksDB Merge Operator' for the cdc domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Changefeed Sequence Counter: RocksDB Merge Operator

### Goal

Deliver the scoped changes for Changefeed Sequence Counter: RocksDB Merge Operator in src/cdc/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Changefeed Sequence Counter: RocksDB Merge Operator
**Priority:** Medium
**Target Version:** v1.8.0

`Changefeed::nextSequence()` in `changefeed.cpp` (line 146, marked with an explicit `// TODO: Consider using RocksDB merge operator for better performance`) uses a mutex + Read-Modify-Write (`Get` then `Put`) round-trip to the RocksDB column family on every change event. Under high write throughput each event serializes through `sequence_mutex_` and issues two synchronous RocksDB operations.

**Implementation Notes:**
- `[ ]` Implement a `SequenceIncrementOperator` (subclass of `rocksdb::AssociativeMergeOperator`) that atomically increments a little-endian uint64 stored under `SEQUENCE_KEY`.
- `[ ]` Register the merge operator on the changefeed column family via `ColumnFamilyOptions::merge_operator` at DB open time in `Changefeed::open()`.
- `[ ]` Replace the `Get` + `Put` in `nextSequence()` with a single `Merge()` call; remove `sequence_mutex_`.
- `[ ]` The returned sequence is still required; fetch via `GetForUpdate` with snapshot isolation, or maintain an in-process atomic counter with periodic RocksDB persistence for crash recovery.

**Performance Targets:**
- Sequence generation throughput: ≥ 200 K/s (from ~50 K/s with mutex+Get+Put) under 8 writer threads.

---


**Priority:** Medium
**Target Version:** v1.8.0

The change log grows unboundedly. Implement size-based and TTL-based retention policies managed by `CDCAdmin`, exposed via both the admin REST API and a background compaction thread.

**Implementation Notes:**
- `[x]` Add `RetentionPolicy` struct to `cdc_admin.h`: `max_age_seconds`, `max_bytes`, `max_entries`; load from `config/data_management/cdc_retention.yaml`.
- `[x]` Background compaction thread in `changefeed.cpp` (similar to L3 eviction thread in `adaptive_query_cache.cpp`): runs every `compaction_interval_s` (default 300 s).
- `[x]` Compaction deletes events older than `max_age_seconds` using `CDCAdmin::purgeOlderThan(timestamp)` (new method).
- `[x]` Size-based trigger: if change log RocksDB column family exceeds `max_bytes`, compact oldest entries until under 80% of limit.
- `[x]` `CDCAdmin::getRetentionStatus()` returns current log size, oldest event age, and next scheduled compaction time.
- `[x]` Admin endpoint: `GET /v1/admin/cdc/retention` and `PUT /v1/admin/cdc/retention` to read/update policy at runtime.

**Performance Targets:**
- Compaction of 1M expired events completes in < 30 s (background, no query impact).
- Compaction I/O bandwidth capped at 50 MB/s to avoid starving foreground writes (configurable).

---

### Acceptance Criteria

- [ ] Implement a `SequenceIncrementOperator` (subclass of `rocksdb::AssociativeMergeOperator`) that atomically increments a little-endian uint64 stored under `SEQUENCE_KEY`.
- [ ] Register the merge operator on the changefeed column family via `ColumnFamilyOptions::merge_operator` at DB open time in `Changefeed::open()`.
- [ ] Replace the `Get` + `Put` in `nextSequence()` with a single `Merge()` call; remove `sequence_mutex_`.
- [ ] The returned sequence is still required; fetch via `GetForUpdate` with snapshot isolation, or maintain an in-process atomic counter with periodic RocksDB persistence for crash recovery.
- [ ] Sequence generation throughput: ≥ 200 K/s (from ~50 K/s with mutex+Get+Put) under 8 writer threads.
- [ ] Add `RetentionPolicy` struct to `cdc_admin.h`: `max_age_seconds`, `max_bytes`, `max_entries`; load from `config/data_management/cdc_retention.yaml`.
- [ ] Background compaction thread in `changefeed.cpp` (similar to L3 eviction thread in `adaptive_query_cache.cpp`): runs every `compaction_interval_s` (default 300 s).
- [ ] Compaction deletes events older than `max_age_seconds` using `CDCAdmin::purgeOlderThan(timestamp)` (new method).
- [ ] Size-based trigger: if change log RocksDB column family exceeds `max_bytes`, compact oldest entries until under 80% of limit.
- [ ] `CDCAdmin::getRetentionStatus()` returns current log size, oldest event age, and next scheduled compaction time.
- [ ] Admin endpoint: `GET /v1/admin/cdc/retention` and `PUT /v1/admin/cdc/retention` to read/update policy at runtime.
- [ ] Compaction of 1M expired events completes in < 30 s (background, no query impact).
- [ ] Compaction I/O bandwidth capped at 50 MB/s to avoid starving foreground writes (configurable).

### Relationships

- Roadmap row: #160 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cdc/FUTURE_ENHANCEMENTS.md#changefeed-sequence-counter-rocksdb-merge-operator
- Source key: roadmap:160:cdc:v1.8.0:changefeed-sequence-counter-rocksdb-merge-operator

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:160:cdc:v1.8.0:changefeed-sequence-counter-rocksdb-merge-operator -->
<!-- roadmap-ref: row=160;module=cdc;target=v1.8.0 -->
<!-- roadmap-detail: src/cdc/FUTURE_ENHANCEMENTS.md#changefeed-sequence-counter-rocksdb-merge-operator -->
