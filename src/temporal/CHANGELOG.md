> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Temporal Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.9.0] — 2026-03-24
### Added
- `TemporalClause` enum (`AS_OF`, `FROM_TO`, `BETWEEN_AND`, `CONTAINED_IN`, `ALL`) — structured SQL:2011 §7.6 clause types
- `TemporalQuerySpec` struct with static factory methods (`asOf`, `fromTo`, `betweenAnd`, `containedIn`, `all`) and `include_deleted` flag; portable replacement for ad-hoc timestamp arguments
- `TemporalQueryEngine::executeTemporalQuery(SystemVersionedTable, TemporalQuerySpec, filters)` — single entry point that dispatches to `queryAsOf`, `queryFromTo`, `queryBetween`, `queryWithSemantics` (NON_SEQUENCED), or CONTAINED_IN filtering; suppresses logically-deleted rows when `include_deleted=false`
- `TemporalQueryEngine::executeTemporalQuery(BiTemporalTable, TemporalQuerySpec, filters)` — application-time variant dispatching to `queryApplicationTime`, `queryApplicationTimeRange`, CONTAINED_IN valid-time filtering, and ALL (full valid-time range)
- 24 new focused tests in `tests/temporal/test_temporal_query_engine.cpp` covering all 5 clause types on both table variants, filter interaction, deleted-row suppression, and factory methods
- `TemporalQueryEngineFocusedTests` CMake target registered in `tests/CMakeLists.txt`
### Changed
- `include/temporal/temporal_query_engine.h` — `TemporalQuerySpec` and `TemporalClause` placed before the class declaration; `executeTemporalQuery()` overloads added as public static methods

## [1.6.0] — 2026-03-20
### Added
- `IntervalTreeIndex`: augmented BST with per-node max-end tracking; O(log n) insert/remove, O(log n + k) point and overlap queries for valid-time period predicates
- `TemporalCompressor`: compresses historical version payloads using DELTA (JSON field-level diff), ZSTD (LZ-family), Gorilla (XOR-delta for numeric time-series), and DICTIONARY (value-table encoding); `compressHistory()` API with grace-window skipping and `CompressionStats` observability
- `TemporalCDC`: in-process version-aware change data capture engine; typed `ChangeEvent` (INSERT/UPDATE/DELETE/VERSION_CREATED) with before/after payloads; named pub/sub subscriptions; bounded ring-buffer event log (default 65 536 events); `replayChanges()` for point-in-time backfill
### Changed
- `src/temporal/ARCHITECTURE.md` updated to reflect Phase 4 components
- `src/temporal/ROADMAP.md` Phase 4 items marked `[x]`; status updated to `Partial ⚙️`
### Fixed
- Known Limitations: history compression and CDC now documented as implemented

## [1.5.0] — 2026-03-12
### Added
- Bitemporal data model: transaction time + valid time tracking
- `AS OF`, `FROM...TO`, `BETWEEN...AND` time-travel query syntax
- HLC-based conflict resolution for concurrent temporal writes
- Bitemporal joins across transaction-time and valid-time dimensions
- SEQUENCED and NON-SEQUENCED update semantics
- Temporal aggregations (min/max/avg over time intervals)
- Automated retention policies with configurable age-based eviction

## [1.0.0] — 2024-01-01
### Added
- Transaction-time tracking for all storage mutations
- Basic time-travel queries
