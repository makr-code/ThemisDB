<!-- Status: current | validated: 2026-04-06 -->

# Roadmap — include/temporal/

> This roadmap covers planned evolution of the **public temporal header API**.  
> Implementation work is tracked in [`../../src/temporal/`](../../src/temporal/).

---

## Current Status

| Field | Value |
|-------|-------|
| **Version** | v1.6.0 |
| **Release Date** | 2026-03-20 |
| **API Stability** | Stable (soname `.6`) |
| **Open Findings** | 2 minor (see AUDIT.md) |

---

## Completed

- [x] `temporal_types.h` — nanosecond-precision `TimePoint` (`v1.6.0`)
- [x] `bi_temporal.h` — valid-time + transaction-time bi-temporal model (`v1.0.0`)
- [x] `bitemporal_join.h` — OVERLAP and CONTAINED_IN join strategies (`v1.6.0`)
- [x] `interval_tree_index.h` — augmented-BST; O(log n + k) overlap query; `bulkLoad()` (`v1.6.0`)
- [x] `temporal_index.h` — index factory interface (`v1.4.0`)
- [x] `temporal_aggregator.h` — sliding / tumbling / session window aggregation (`v1.5.0`)
- [x] `temporal_query_engine.h` — AS-OF, BETWEEN, OVERLAPS, CONTAINED IN queries (`v1.2.0`)
- [x] `temporal_cdc.h` — pub/sub ring-buffer 65 536; `replayChanges()` (`v1.6.0`)
- [x] `temporal_compressor.h` — DELTA, ZSTD, Gorilla, DICTIONARY strategies (`v1.6.0`)
- [x] `temporal_conflict_resolver.h` — LWW and custom resolver (`v1.6.0`)
- [x] `retention_manager.h` — ARCHIVE retention tier (`v1.5.0`)
- [x] `snapshot_manager.h` — RAII snapshot handle + catalog (`v1.5.0`)
- [x] `system_versioned_table.h` — SQL:2011 system-versioned tables (`v1.5.0`)

---

## Planned Features

- [x] `temporal_cdc.h` — document ring-buffer overflow semantics in header doxygen (Implemented: 2026-04-17)
- [x] `retention_manager.h` — add `operator==` / `operator<` to `RetentionRule` (Implemented: 2026-04-17)
- [x] `temporal_conflict_resolver.h` — `MergeResolver` strategy for CRDT-style merges (Implemented: 2026-04-17)
  - Inputs: two `BiTemporalRecord<T>` with same `ValidTime`
  - Output: merged record; conflict log entry
  - Constraints: merge function must be commutative and idempotent
  - Tests: unit tests for commutativity + idempotency properties
- [x] `temporal_query_engine.h` — `SEQUENCED DISTINCT` query primitive (SQL:2011 §13.4) (Implemented: 2026-04-17)
- [x] `interval_tree_index.h` — `erase(key)` with rebalancing in O(log n) (Implemented: 2026-04-17)
- [x] `temporal_aggregator.h` — `FIRST_VALUE` / `LAST_VALUE` ordered temporal analytic functions (Implemented: 2026-04-17)
- [x] `temporal_compressor.h` — `LZ4Strategy` for high-throughput low-latency paths (Implemented: 2026-04-12)
- [x] `temporal_cdc.h` — persistent CDC log backed by append-only WAL segment (Implemented: 2026-04-17)
  - Design: `CDCPersistentLog` implementing `CDCListener`; WAL segment rotation at 64 MB
  - Errors: disk-full handling, segment corruption detection (CRC-32)
- [x] `snapshot_manager.h` — incremental snapshot diffing (`Snapshot::diff(other)`) (Implemented: 2026-04-17)
- [x] `bi_temporal.h` — `BiTemporalTable::merge(other)` for cross-node LWW reconciliation (Implemented: 2026-04-17)
- [ ] Remove deprecated `LegacyTemporalIndex` shim from `temporal_index.h` (Target: v1.8.0) — ✅ Completed 2026-04-17: shim was deprecated in v1.6.0 and is confirmed absent from source in v1.8.0

---

## Implementation Phases

### Phase 1 — Design / API Contract (current sprint)
- [x] Finalise `MergeResolver` interface signature and callback contract
- [x] Draft `SEQUENCED DISTINCT` query plan representation in `TemporalQueryPlan`
- [x] Specify WAL segment format for persistent CDC log

### Phase 2 — Core Implementation
- [x] Implement `MergeResolver` in planned temporal conflict-resolver implementation
- [x] Implement `SEQUENCED DISTINCT` path in `TemporalQueryEngine`
- [x] Implement `LZ4Strategy` in planned temporal compressor implementation
- [x] Implement `erase()` with tree rebalancing in `IntervalTreeIndex`

### Phase 3 — Error Handling & Edge Cases
- [x] CDC overflow: document and implement configurable policy (OVERWRITE / BLOCK / DROP)
- [x] WAL segment: CRC-32 validation on open; truncation recovery
- [x] `erase()`: concurrent reader invalidation handled via `std::unique_lock<std::shared_mutex>`; doxygen documents safe usage contract for callers holding query result copies (Implemented: 2026-04-17)

### Phase 4 — Tests
- [x] Unit tests: `MergeResolver` commutativity and idempotency (MCR-01..07)
- [x] Unit tests: `SEQUENCED DISTINCT` correctness against SQL:2011 examples
- [ ] Fuzz tests: `LZ4Strategy` round-trip via `libFuzzer` harness
- [x] Benchmark: `IntervalTreeIndex::erase()` vs. rebuild baseline (`benchmarks/bench_interval_tree_erase.cpp`) (Implemented: 2026-04-17)

### Phase 5 — Performance / Hardening
- [x] Profile `LZ4Strategy` — target ≥ 2 GB/s on timestamp columns
- [x] Validate `SEQUENCED DISTINCT` query plan avoids full-scan on indexed stores
- [ ] Stress-test persistent CDC log under 100k events/s sustained write rate

### Phase 6 — Documentation & Acceptance
- [x] Update all doxygen blocks for new symbols (Implemented: 2026-04-17)
- [ ] Update `ARCHITECTURE.md` interface inventory table
- [x] Update `CHANGELOG.md` under `[Unreleased]` (Implemented: 2026-04-17)
- [ ] Peer review of public-header API changes

---

## Production Readiness Checklist

- [x] All current headers have `[[nodiscard]]` on resource/error-returning functions
- [x] Virtual destructors on all polymorphic bases
- [x] No `using namespace` in public headers
- [x] RAII wrappers for all ownership transfers
- [x] ABI soname policy documented
- [x] CDC overflow semantics documented in header
- [x] `RetentionRule` equality operators
- [x] `LegacyTemporalIndex` removal scheduled for v1.8.0 (Completed: 2026-04-17; shim was documented in CHANGELOG v1.6.0 as deprecated; confirmed absent from source in v1.8.0)
