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

## [1.5.0] — 2026-02-01
### Added
- Raft-based consensus for shard coordination and leader election
- `ShardRepairEngine` – anti-entropy background scan + repair queue
- Vandermonde-based Reed-Solomon decoder – full multi-chunk recovery
- Prometheus metrics integration for repair health
- Admin API repair endpoints (`POST /admin/repair`, `/admin/repair/scan`, `GET /admin/repair/{id}`)

## [1.4.0] — 2025-12-01
### Added
- Pluggable consensus framework (Raft, Gossip, Paxos) via `ConsensusFactory`
- Cross-shard transaction coordinator (2PC, 3PC, SAGA, Percolator)
- Distributed deadlock detection (wait-for-graph cycle detection)
- Metadata sharding design and documentation
- Shard repair engine with automatic rebalancing
- Cross-shard query routing with scatter-gather execution
- Consistent hashing ring with virtual nodes
- Per-shard metrics and health monitoring via MetricsCollector

## [1.0.0] — 2024-01-01
### Added
- Horizontal sharding with range and hash partitioning
- Shard metadata registry
- Cross-shard transaction coordination
