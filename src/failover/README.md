> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# failover module

<!-- Status: current | validated: 2026-05-12 -->
<!-- Links: ../../include/failover/README.md · ./ARCHITECTURE.md · ./ROADMAP.md · ./FUTURE_ENHANCEMENTS.md -->

Status: production-ready failover orchestration and disaster recovery workflows.

## Current Implementation Layout

| Component | Implementation Location | Runtime Role |
|----------|--------------------------|--------------|
| Automatic failover orchestration | `src/failover/auto_failover_manager.cpp` | Runs monitoring + worker threads, queues failover requests, emits lifecycle/pressure events, updates retry/queue telemetry |
| Disaster-recovery execution | `src/failover/disaster_recovery_manager.cpp` | Validates plans and executes the 7-step DR pipeline with per-step outcome reporting |
| Public interfaces | `include/failover/auto_failover_manager.h`, `include/failover/disaster_recovery_manager.h` | Expose manager APIs, config structs, runtime state enums, result/statistics contracts |
| Design notes | `src/failover/ARCHITECTURE.md` | Captures concurrency model, recovery phases, and integration boundaries |

## Runtime Behavior, Errors, and Limits

### `AutoFailoverManager`

- Starts two background threads: a monitoring loop and a failover worker loop.
- Health checks update per-node failure tracking; nodes crossing the configured failure threshold are queued for failover.
- Manual failovers are rejected when the manager is not running or when the failover queue is already at `max_concurrent_failovers`.
- Queue-pressure telemetry is emitted once the queue fill ratio reaches `queue_pressure_threshold`; statistics track current/max depth and dropped tasks.
- When enabled, network-partition detection emits `NETWORK_PARTITION_DETECTED` and can trigger split-brain prevention / graceful read-only behavior.
- Successful and failed failovers update `FailoverResult` and `Statistics`; optional automatic recovery emits `RECOVERY_STARTED` / `RECOVERY_COMPLETED`.

### `DisasterRecoveryManager`

- Validates the plan before mutating state; non-dry-run plans require `plan_id`, `primary_site`, `recovery_site`, and `snapshot_id`.
- Executes the recovery pipeline in this order: prechecks, snapshot validation, epoch fencing, restore, replica catchup, traffic shift, verification.
- Dry-run plans short-circuit the operational steps with explicit `"dry-run ..."` step messages.
- Missing replication/fencing managers cause failures when the corresponding config requires quorum checks or epoch fencing.
- Verification retries are bounded by `max_verification_retries`; catchup and verification both honor timeout-based limits.
- Step hooks (`setStepHook`) can override built-in step execution for tests or controlled integrations.

## Usage Notes

- Use [`../../include/failover/README.md`](../../include/failover/README.md) for the public API, config fields, and code snippets.
- Review [`./ARCHITECTURE.md`](./ARCHITECTURE.md) before changing concurrency-sensitive logic.
- Open roadmap items and longer-term work are tracked in [`./ROADMAP.md`](./ROADMAP.md) and [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md).

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Troubleshooting

- `triggerManualFailover(...)` returns `false`: confirm `start()` succeeded and the queue is not already full.
- Frequent `QUEUE_PRESSURE` events: raise `max_concurrent_failovers`, lower enqueue rate, or investigate why workers are not draining.
- DR plan fails with `snapshot_id must not be empty`: provide a snapshot for non-dry-run recovery.
- DR plan fails with quorum/fencing errors: either provide the required managers or explicitly relax `require_quorum` / `enforce_epoch_fencing` for the intended scenario.

## Related Docs

- Public headers: [`../../include/failover/README.md`](../../include/failover/README.md)
- Architecture: [`./ARCHITECTURE.md`](./ARCHITECTURE.md)
- Roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- German module overview: [`../../docs/de/failover/README.md`](../../docs/de/failover/README.md)
- English secondary overview: [`../../docs/en/failover/README.md`](../../docs/en/failover/README.md)
