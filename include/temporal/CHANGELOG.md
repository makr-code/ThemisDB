<!-- Status: current | validated: 2026-04-06 -->

# Changelog — include/temporal/

All notable changes to the **public temporal headers** are documented here.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).  
For implementation-level changes see [`../../src/temporal/CHANGELOG.md`](../../src/temporal/CHANGELOG.md).

---

## [Unreleased]

### Planned
- Document `CDCRingBuffer` overflow semantics in `temporal_cdc.h`
- Add `operator==` / `operator<` to `RetentionRule` in `retention_manager.h`

---

## [1.9.0] — 2026-04-17

### Added
- `snapshot_manager.h` — `SnapshotDiff` struct (`added`, `modified`, `removed`
  per table) and `TemporalSnapshotManager::diff(base, other)` for incremental
  snapshot diffing; O(R log R); serialisable via `toJson()`.
- `bi_temporal.h` — `BiTemporalTable::merge(other)`: cross-node Last-Writer-Wins
  reconciliation keyed on `sys_time.start`; returns `MergeResult{rows_inserted,
  rows_skipped, conflicts_lww}`; idempotent.

---

## [1.8.0] — 2026-04-17

### Added
- `temporal_cdc.h` — `CDCPersistentLog`: WAL-backed persistent CDC log
  implementing `CDCListener`.  Append-only segment format with 8-byte magic
  (`0x54444357`), version, and per-record CRC-32/ISO-HDLC.  Automatic segment
  rotation at 64 MiB.  Truncation recovery on `open()`: corrupt tail records
  are silently discarded.

### Removed
- `temporal_index.h` — `LegacyTemporalIndex` shim (deprecated since v1.6.0).
  The class was absent from source at this version boundary; this entry
  formally closes the deprecation cycle.

---

## [1.7.0] — 2026-04-17

### Added
- `interval_tree_index.h` — `erase(key)` STL-style alias for `removeKey()`;
  exclusive-write lock ensures no concurrent reader invalidation; doxygen
  caller-contract added.
- `temporal_aggregator.h` — `AggregateFunc::FIRST_VALUE` and
  `AggregateFunc::LAST_VALUE` ordered temporal analytic functions; implemented
  in all four compute paths (TRANSACTION, VALID, BITEMPORAL, SNAPSHOT).
- `temporal_query_engine.h` — `sequencedDistinct(table)` and
  `sequencedDistinctForKey(table, key)` implementing SQL:2011 §13.4 SEQUENCED
  DISTINCT; adjacent-period coalescing.

---

## [1.6.1] — 2026-04-17

### Added
- `retention_manager.h` — `RetentionRule` is now a proper value type with
  `operator==`, `operator!=`, and `operator<` to support sorted containers and
  equality assertions.
- `temporal_cdc.h` — `OverflowPolicy` enum (`OVERWRITE`, `BLOCK`, `DROP`) for
  the `CDCRingBuffer`; `overflowCount()` accessor; full Doxygen on ring-buffer
  overflow semantics.
- `temporal_conflict_resolver.h` — abstract `MergeResolver` strategy interface
  with `LWWFieldMergeResolver`, `UnionMergeResolver`, and `CustomMergeResolver`
  concrete implementations; `setMergeResolver` / `getMergeResolver` on
  `TemporalConflictResolver`; `resolveCRDT()` delegates to the injected
  resolver.

---

## [1.6.0] — 2026-03-20

### Added
- `temporal_conflict_resolver.h` — new header exposing `ConflictResolver` base,
  `LastWriteWinsResolver`, and `CustomResolver` with stable callback signature
- `temporal_compressor.h` — `GorillaStrategy` (XOR-delta float encoding) and
  `DictionaryStrategy` added alongside existing `DeltaStrategy` and `ZstdStrategy`
- `temporal_cdc.h` — `replayChanges(from, to)` method on `CDCRingBuffer`; ring
  buffer capacity increased from 32 768 → 65 536
- `interval_tree_index.h` — `bulkLoad()` method for O(n log n) batch construction
- `bitemporal_join.h` — `JoinStrategy::OVERLAP` and `JoinStrategy::CONTAINED_IN`
  enum values

### Changed
- `temporal_types.h` — `TimePoint` now typedef'd to `std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>`; previously microseconds
- `temporal_query_engine.h` — `execute()` marked `[[nodiscard]]`

### Deprecated
- `temporal_index.h` — `LegacyTemporalIndex` shim; will be removed in v1.8.0

### Fixed
- `bi_temporal.h` — `BiTemporalStore::insert()` no longer silently discards
  records with `valid_start == valid_end`; now throws `std::invalid_argument`

---

## [1.5.2] — 2026-01-14

### Fixed
- `snapshot_manager.h` — `Snapshot` move constructor was inadvertently deleted;
  restored

---

## [1.5.0] — 2025-11-30

### Added
- `system_versioned_table.h` — SQL:2011 system-versioned table interface
- `temporal_aggregator.h` — session-window support via `WindowSpec::Session`
- `retention_manager.h` — `RetentionPolicy::ARCHIVE` tier

### Changed
- `temporal_cdc.h` — `CDCListener` interface now receives `CDCEvent` by const-ref
  instead of by value (ABI-breaking; soname bumped to `.5`)

---

## [1.4.0] — 2025-08-15

### Added
- `interval_tree_index.h` — initial public release of `IntervalTreeIndex<K,V>`
- `temporal_index.h` — `TemporalIndexFactory` with `create(IndexType)` factory
  method

### Changed
- `bi_temporal.h` — split monolithic `BiTemporal.h` into `bi_temporal.h` +
  `bitemporal_join.h`

---

## [1.3.0] — 2025-05-01

### Added
- `temporal_compressor.h` — `DeltaStrategy` and `ZstdStrategy` initial release
- `snapshot_manager.h` — `SnapshotCatalog` listing interface

---

## [1.2.0] — 2025-02-10

### Added
- `temporal_query_engine.h` — initial `AS-OF`, `BETWEEN` query support
- `temporal_cdc.h` — initial pub/sub CDC ring-buffer (capacity 32 768)

---

## [1.0.0] — 2024-09-01

### Added
- `temporal_types.h`, `bi_temporal.h`, `retention_manager.h` — initial public release
