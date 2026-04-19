> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Timeseries Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.9.0] — 2026-03-24
### Added
- `ContinuousAggStatus` enum (`ACTIVE`, `STALE`, `INACTIVE`) — aggregate lifecycle state
- `ContinuousAggDefinition` struct — named continuous aggregate descriptor carrying `config` (AggConfig), `auto_refresh`, `status`, and auto-populated `agg_id` (watermark key)
- `ContinuousAggMaterializationStatus` struct — per-aggregate status snapshot (`name`, `derived_metric`, `watermark_ms`, `status`, `windows_written`)
- `ContinuousAggMaterializationEngine` — TimescaleDB-style materialization engine with:
  - `createAggregate(def)` — register a named aggregate (rejects duplicates, auto-derives `agg_id`)
  - `dropAggregate(name)` — deregister and delete persisted watermark
  - `listAggregates()` — returns names in stable insertion order
  - `getAggregate(name)` — returns definition or `nullopt`
  - `refreshAggregate(name, to_ms)` — incremental watermark-driven refresh; skips INACTIVE aggregates
  - `refreshAll(to_ms)` — refresh all ACTIVE aggregates with `auto_refresh=true` in insertion order
  - `queryMaterialized(name, from_ms, to_ms)` — read derived metric data points without triggering refresh
  - `getAggregateStatus(name)` / `getAllStatus()` — watermark and lifecycle observability
- 27 focused tests in `tests/timeseries/test_continuous_agg_materialization.cpp` covering registry CRUD, refresh lifecycle, watermark advancement, query routing, incremental refresh correctness, and status reporting
- `ContinuousAggMaterializationFocusedTests` CMake target registered in `tests/CMakeLists.txt`
### Changed
- `include/timeseries/continuous_agg.h` — added `ContinuousAggStatus`, `ContinuousAggDefinition`, `ContinuousAggMaterializationStatus`, and `ContinuousAggMaterializationEngine` declarations; added `#include <unordered_map>`

## [1.5.0] — 2026-03-12
### Added
- Gorilla compression algorithm for floating-point time series values
- Continuous aggregation with configurable windows (tumbling, sliding, session)
- Retention policies with automatic TTL-based eviction
- Auto-batching write buffer for high-frequency single-point inserts
- Downsampling pipelines (LTTB, min/max/avg)
- Gap-fill interpolation for sparse time series

## [1.0.0] — 2024-01-01
### Added
- Time-ordered key-value storage with nanosecond precision timestamps
- Range queries by time interval
- Basic aggregation (count, sum, avg, min, max)
