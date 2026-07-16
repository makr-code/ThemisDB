### Context

This issue implements the roadmap item '`IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch' for the analytics domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: 6 · `IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch

### Goal

Deliver the scoped changes for `IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch in src/analytics/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### 6 · `IncrementalView::applyChanges()` — Exclusive Lock for Entire Batch
**Priority:** High
**Target Version:** v1.8.0
**Files:** `src/analytics/incremental_view.cpp` lines 325–400

`applyChanges(const std::vector<ChangeRecord>& changes)` acquires `unique_lock lk(rw_mutex_)`
at line 325 and holds it for the entire iteration over `changes`, which may contain
thousands of records.  Concurrent readers (`query()` at line 371 uses `shared_lock`) are
blocked for the full batch duration, violating the 50 ms IVM constraint when batches exceed
a few hundred rows under load.

`applyChange()` (single-record path, line 284) exhibits the same pattern: the unique lock
spans `passesBaseFilters()`, `applyRow()`, and `pruneEmptyGroup()`, all of which involve
`unordered_map` lookups and string parsing.

**Implementation Notes:**
- `[ ]` In `applyChanges()`, process changes in micro-batches of ≤ 256 rows: acquire `unique_lock`, apply micro-batch, release, yield with `std::this_thread::yield()`, repeat — readers can slip in between micro-batches
- `[ ]` Pre-compute `passesBaseFilters()` outside the write lock using a read-only snapshot of `def_` (immutable after construction); only `applyRow()` and `pruneEmptyGroup()` need the exclusive lock
- `[ ]` Add a read-latency regression test: background writer calls `applyChanges(10 000 rows)` while a reader thread calls `query()` in a tight loop; assert reader P99 ≤ 10 ms

**Performance Targets:**
- Reader P99 latency during a 10 000-row batch apply: ≤ 10 ms
- `applyChanges()` throughput: ≥ 200 000 rows/s

---

### Acceptance Criteria

- [ ] In `applyChanges()`, process changes in micro-batches of ≤ 256 rows: acquire `unique_lock`, apply micro-batch, release, yield with `std::this_thread::yield()`, repeat — readers can slip in between micro-batches
- [ ] Pre-compute `passesBaseFilters()` outside the write lock using a read-only snapshot of `def_` (immutable after construction); only `applyRow()` and `pruneEmptyGroup()` need the exclusive lock
- [ ] Add a read-latency regression test: background writer calls `applyChanges(10 000 rows)` while a reader thread calls `query()` in a tight loop; assert reader P99 ≤ 10 ms
- [ ] Reader P99 latency during a 10 000-row batch apply: ≤ 10 ms
- [ ] `applyChanges()` throughput: ≥ 200 000 rows/s

### Relationships

- Roadmap row: #45 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/analytics/FUTURE_ENHANCEMENTS.md#6--incrementalviewapplychanges--exclusive-lock-for-entire-batch
- Source key: roadmap:45:analytics:v1.8.0:6-incrementalviewapplychanges-exclusive-lock-for-entire-batch

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:45:analytics:v1.8.0:6-incrementalviewapplychanges-exclusive-lock-for-entire-batch -->
<!-- roadmap-ref: row=45;module=analytics;target=v1.8.0 -->
<!-- roadmap-detail: src/analytics/FUTURE_ENHANCEMENTS.md#6--incrementalviewapplychanges--exclusive-lock-for-entire-batch -->
