> **Build:** `cmake --preset release && cmake --build build/release`

# include failover module

**Module Path:** `include/failover/`
**Implementation Overview:** `../../src/failover/README.md`

Public header surface for failover orchestration and disaster recovery.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `auto_failover_manager.h` | `AutoFailoverManager`, `AutoFailoverConfig`, `FailoverResult`, `FailoverEventType`, `FailoverOrchestratorState` | Automatic failover orchestration, queue management, event callbacks, retry and pressure telemetry |
| `disaster_recovery_manager.h` | `DisasterRecoveryManager`, `DisasterRecoveryConfig`, `DisasterRecoveryPlan`, `DisasterRecoveryResult`, `DisasterRecoveryStep` | Disaster-recovery plan validation, 7-step execution pipeline, step-hook injection, aggregate run statistics |

## Public API Behavior

### `AutoFailoverManager`

- Lifecycle: `start()`, `stop()`, `isRunning()`
- Manual control: `triggerManualFailover(failed_node_id, target_promote_id)`
- State/inspection: `getState()`, `isFailoverInProgress()`, `getFailingNodes()`, `getLastFailoverResult()`
- Runtime config: `updateConfig()` / `getConfig()`
- Observability: `getStatistics()` and `registerEventCallback()`

Important runtime semantics:

- `triggerManualFailover()` returns `false` if the manager is stopped or the queue is already full.
- Queue-pressure events are emitted when queue fill ratio reaches `queue_pressure_threshold`.
- `Statistics` include processed failovers, queue depth/drop counters, network-partition counts, and retry counters.

### `DisasterRecoveryManager`

- Execution: `executePlan(plan)`
- Validation: `validatePlan(plan, error)`
- Customisation/testing: `setStepHook(step, hook)`, `clearStepHooks()`
- State/inspection: `getState()`, `getStatistics()`

Important runtime semantics:

- Non-dry-run plans require a snapshot ID.
- Built-in execution order is `PRECHECKS → SNAPSHOT_VALIDATION → EPOCH_FENCING → RESTORE → REPLICA_CATCHUP → TRAFFIC_SHIFT → VERIFICATION`.
- Step hooks override the built-in implementation for the targeted step.

## Configuration Options

### `AutoFailoverConfig`

| Field | Effect |
|------|--------|
| `failure_detection_interval`, `health_check_interval` | Monitoring cadence for failure tracking |
| `failover_timeout`, `spare_activation_timeout`, `leader_election_timeout` | Time bounds for key failover phases |
| `consecutive_failures_before_action` | Node-failure threshold before orchestration triggers |
| `max_concurrent_failovers` | Queue capacity for pending failovers |
| `queue_pressure_threshold` | Emits `QUEUE_PRESSURE` once queue fill ratio reaches the configured fraction |
| `enable_automatic_failover`, `enable_spare_activation`, `enable_leader_election` | Enable/disable major orchestration behaviors |
| `enable_network_partition_detection`, `enable_split_brain_prevention` | Control partition detection and epoch-fencing safeguards |
| `enable_automatic_recovery`, `recovery_retry_interval`, `max_recovery_attempts` | Control best-effort post-failover recovery retries |

### `DisasterRecoveryConfig`

| Field | Effect |
|------|--------|
| `precheck_timeout`, `catchup_timeout`, `verification_timeout` | Upper bounds for prechecks, replica catchup, and verification windows |
| `max_verification_retries` | Max health/quorum verification attempts before failure |
| `require_quorum` | Requires cluster quorum during prechecks/catchup/verification |
| `enforce_epoch_fencing` | Requires epoch fencing before restore / traffic cutover |
| `allow_dry_run_without_managers` | Declared compatibility flag; current implementation already permits dry-run execution without external managers regardless of this field |

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

```cpp
#include "failover/auto_failover_manager.h"

themis::failover::AutoFailoverConfig cfg;
cfg.max_concurrent_failovers = 4;
cfg.queue_pressure_threshold = 0.5f;

themis::failover::AutoFailoverManager mgr(cfg, replication_mgr, health_monitor, spare_mgr, fencing_mgr);
mgr.start();
mgr.triggerManualFailover("node-a");
```

```cpp
#include "failover/disaster_recovery_manager.h"

themis::failover::DisasterRecoveryPlan plan;
plan.plan_id = "dr-plan-1";
plan.primary_site = "dc-a";
plan.recovery_site = "dc-b";
plan.snapshot_id = "snapshot-42";

themis::failover::DisasterRecoveryManager mgr(cfg, replication_mgr, fencing_mgr);
auto result = mgr.executePlan(plan);
```

## Troubleshooting

- `start()` returns `false`: the manager is already running or thread startup failed.
- `triggerManualFailover()` returns `false`: start the manager first, or increase `max_concurrent_failovers` if the queue is saturated.
- DR validation fails: ensure `plan_id`, `primary_site`, `recovery_site`, and (for non-dry-run) `snapshot_id` are set.
- DR execution fails on fencing/quorum: pass the required managers or align `require_quorum` / `enforce_epoch_fencing` with the intended environment.

## Related Docs

- Implementation overview: [`../../src/failover/README.md`](../../src/failover/README.md)
- Architecture: [`../../src/failover/ARCHITECTURE.md`](../../src/failover/ARCHITECTURE.md)
- Roadmap: [`../../src/failover/ROADMAP.md`](../../src/failover/ROADMAP.md)
- Future enhancements: [`../../src/failover/FUTURE_ENHANCEMENTS.md`](../../src/failover/FUTURE_ENHANCEMENTS.md)
- German module overview: [`../../docs/de/failover/README.md`](../../docs/de/failover/README.md)
- English secondary overview: [`../../docs/en/failover/README.md`](../../docs/en/failover/README.md)
