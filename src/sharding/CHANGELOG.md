> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Sharding Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.9.0] — 2026-03-24
### Added
- **Phase 4.1 — Epoch-based Fencing + Lease Management** (`epoch_fencing.h` / `epoch_fencing.cpp`)
  - `EpochFencingManager`: monotonically-increasing epoch counter; `bumpEpoch()` / `checkToken()` / `makeToken()`
  - `EpochToken`: immutable carry-along fencing credential validated on every write RPC
  - `FencingResult` enum: ALLOWED / STALE_EPOCH / INVALID_TOKEN / STONITH_ISSUED / STONITH_FAILED
  - `IStonithProvider` abstract interface + `NullStonithProvider` (in-memory, for tests)
  - `EpochFencingConfig` with auto-STONITH flag and configurable timeout
  - `LeaseManager`: distributed time-bounded exclusive leases with acquire / renew / release / eviction
  - `LeaseRecord` + `LeaseState` (AVAILABLE / HELD / EXPIRED / REVOKING) + `LeaseConfig`
  - WAL-backed lease persistence (`wal_path`) for crash-safe restart recovery
  - Per-manager `Metrics` structs for both `EpochFencingManager` and `LeaseManager`
  - 28 unit tests in `tests/test_epoch_fencing.cpp`; `EpochFencingFocusedTests` CMake target
  - CI: `.github/workflows/06-infrastructure_distributed_epoch-fencing-ci.yml`


### Added
- Raft-based consensus for shard coordination and leader election
- Shard repair engine with automatic rebalancing
- Cross-shard query routing with scatter-gather execution
- Consistent hashing ring with virtual nodes
- Shard split and merge operations with zero-downtime
- Per-shard metrics and health monitoring via MetricsCollector

## [1.0.0] — 2024-01-01
### Added
- Horizontal sharding with range and hash partitioning
- Shard metadata registry
- Cross-shard transaction coordination
