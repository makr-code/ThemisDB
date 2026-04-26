> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Sharding Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
### Added
- **`HammingCoder`** — RAID-2 style shard-level error-correction coder (`include/sharding/redundancy_strategy.h`, `src/sharding/redundancy_strategy.cpp`)
  - Implements the generalised Hamming parity-bit assignment at block granularity using pure XOR (no Galois-Field arithmetic)
  - `HammingCoder::encode()`: systematic encoding; parity shard _p_ covers data shard _j_ when bit _p_ is set in the 1-based position (_j_+1)
  - `HammingCoder::decode()`: iterative XOR repair; recovers all shards whose parity coverage allows; throws `std::runtime_error` on unrecoverable failure sets
  - `HAMMING` added to `ErasureCodingAlgorithm` enum; factory `ErasureCoder::create(HAMMING)` returns `HammingCoder`
  - 16 focused tests in `tests/test_hamming_coder.cpp` (HC_01..HC_16): chunk invariants, single/multi-shard failure, canonical Hamming(7,4) parity-coverage verification, 1 MB round-trip, edge cases
  - `HammingCoderFocusedTests` CTest target registered


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
