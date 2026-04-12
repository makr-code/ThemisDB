<!-- Status: current | validated: 2026-04-06 -->

# Updates Module — Public Header Roadmap

## Current Status

Production-ready.  v1.5.0 headers shipped 2026-03-12.  All 19 public headers are
stable under semantic versioning.

## Completed

- [x] Blue/green deployment API (`blue_green_deployment.h`) — v1.0.0
- [x] Cluster update orchestration (`cluster_update_manager.h`) — v1.0.0
- [x] Schema migration definitions and runner — v1.0.0
- [x] In-place schema migrator — v1.0.0
- [x] Updates configuration bag (`updates_config.h`) — v1.0.0
- [x] Canary rollout with percentage-gated routing — v1.1.0
- [x] Dependency resolver — v1.1.0
- [x] Manifest database and release manifest — v1.1.0
- [x] Hot reload engine — v1.2.0
- [x] Delta update engine — v1.2.0
- [x] Parallel downloader — v1.2.0
- [x] Update state machine FSM — v1.3.0
- [x] Update history logger — v1.3.0
- [x] Notification webhook — v1.3.0
- [x] Coordinated update manager — v1.4.0
- [x] Preflight health check — v1.4.0
- [x] Tenant update scheduler — v1.5.0
- [x] Schema migration tester — v1.5.0

## Planned Features

- [x] Rollback checkpoint API on `UpdateStateMachine` (Target: Q2 2026) — **Completed v1.8.0**
  - `createCheckpoint()`, `rollbackToCheckpoint(CheckpointId)`, `listCheckpoints()`, `clearCheckpoints()`, `setHistoryLogger()`
  - Integrated with `UpdateHistoryLogger` for auditability (checkpoint_created / checkpoint_rollback events)
  - 17 focused tests added to `tests/test_updates_production.cpp` (CheckpointTest suite)
- [ ] Progressive schema migration — multi-step expand/contract (Target: Q3 2026)
  - `SchemaMigration::addExpandStep()` / `addContractStep()`
  - Validated by `SchemaMigrationTester` in shadow mode
- [ ] Canary automatic promotion/abort based on metrics (Target: Q3 2026)
  - `CanaryConfig::setAutoPromoteThreshold()` and `setAutoAbortThreshold()`
  - Pluggable metric provider interface
- [ ] Dry-run mode for `ClusterUpdateManager` (Target: Q4 2026)
  - `simulate()` returns `UpdatePlan` without applying changes
- [ ] OCI image pull integration in `ParallelDownloader` (Target: Q4 2026)
  - Support for OCI registry authentication tokens in `ReleaseManifest`

## Implementation Phases

### Phase 1 — Design / API Contract
- Define `CheckpointId` type and checkpoint/rollback signatures.
- Specify expand/contract step model for `SchemaMigration`.
- Define metric provider interface for auto-promotion canary.

### Phase 2 — Core Implementation
- Checkpoint creation integrated into `UpdateStateMachine` state entry.
- Expand/contract multi-step runner in `MigrationRunner`.
- Metric-driven auto-promote/abort coroutine in `CanaryRollout`.

### Phase 3 — Error Handling & Edge Cases
- Checkpoint rollback under concurrent canary traffic.
- Partial expand/contract failure recovery.
- Metric provider unavailability graceful degradation.

### Phase 4 — Tests
- Unit tests: checkpoint create + rollback state machine round-trips.
- Integration tests: multi-step schema migration on staging cluster.
- Simulation tests: `ClusterUpdateManager::simulate()` diff validation.

### Phase 5 — Performance / Hardening
- Checkpoint creation overhead < 5 ms per node.
- Parallel downloader OCI pull throughput ≥ 200 MB/s on 10 Gbit link.
- Canary metric evaluation loop ≤ 1 s latency.

### Phase 6 — Documentation & Sign-off
- Update ROADMAP, ARCHITECTURE, AUDIT, CHANGELOG.
- Doxygen annotations on all new public symbols.
- Security review of OCI authentication token handling in `ReleaseManifest`.

## Production Readiness Checklist

- [x] All headers compile under C++17 with `-Wall -Wextra`
- [x] `CanaryConfig` percentage bounds validated at construction
- [x] `UpdateStateMachine` invalid transitions throw typed exceptions
- [x] `PreflightHealthCheck` result is strongly typed; cannot be silently ignored
- [x] `UpdateHistoryLogger` methods are `[[nodiscard]]`
- [x] Rollback checkpoint API finalised (v1.8.0)
- [ ] Progressive schema migration validated against production schemas
