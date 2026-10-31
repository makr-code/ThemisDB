---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset release && cmake --build build/release`

# include failover module

**Module Path:** `include/failover/`
**Implementation Overview:** `../../src/failover/README.md`

Public header surface for failover orchestration and disaster recovery.

## Header Entry-Points

| Header | Primary API | Runtime Role |
|--------|-------------|--------------|
| `auto_failover_manager.h` | `AutoFailoverManager`, `AutoFailoverConfig`, `FailoverResult`, event/state/statistics types | Health/failure monitoring, bounded queue, quorum/fencing gates, replica-promotion request, callbacks and in-process statistics |
| `disaster_recovery_manager.h` | `DisasterRecoveryManager`, config/plan/result/step types | DR plan validation, ordered seven-step coordination, step hooks and run statistics |
| `failover_api_contract.h` | `NodeRole`, `FailoverErrorCode`, timing/epoch/quorum/retry helpers | Shared contract declarations |
| `quorum_log.h` | `QuorumLog`, `QuorumEntry`, `QuorumState` | Optional local append/recovery log |
| `topology_snapshot.h` | `TopologySnapshot` | Failure-map snapshot and node-set comparison |

## Public API Behavior

### `AutoFailoverManager`

- Lifecycle: `start()`, `stop()`, `isRunning()`
- Manual control: `triggerManualFailover(failed_node_id, target_promote_id)`
- State/inspection: `getState()`, `isFailoverInProgress()`, `getFailingNodes()`, `getLastFailoverResult()`
- Runtime config: `updateConfig()` / `getConfig()`
- Observability: `getStatistics()` and `registerEventCallback()`

Important runtime semantics:

- `triggerManualFailover()` returns `false` if the manager is stopped or the queue is already full.
- A `QUEUE_PRESSURE` event is emitted for an enqueue whose queue fill ratio meets or exceeds `queue_pressure_threshold`.
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
| `health_check_interval` | Initial monitoring interval; adaptive mode may subsequently change it |
| `consecutive_failures_before_action` | Node-failure threshold before orchestration triggers |
| `max_concurrent_failovers` | Maximum number of pending requests in the queue |
| `queue_pressure_threshold` | Emits `QUEUE_PRESSURE` on enqueues at or above the configured fill fraction |
| `enable_spare_activation`, `enable_network_partition_detection`, `enable_split_brain_prevention` | Control the implemented spare, partition-detection, and fencing paths |
| `enable_automatic_recovery`, `recovery_retry_interval`, `max_recovery_attempts` | Control best-effort post-failover recovery retries |
| `health_check_call_timeout_ms`, `quorum_timeout_ms` | Bound the monitoring thread's health-call wait and quorum-check wait, respectively; a timed-out async health call may continue |
| `quorum_log_path` | Enables the optional local quorum log |
| `adaptive_check_interval`, `adaptive_check_samples`, `adaptive_check_interval_min`, `adaptive_check_interval_max` | Enable adaptive monitoring cadence and set its sample window and bounds |
| `gc_grace_failure_count`, `gc_grace_window`, `gc_grace_period` | Configure the failure-count window and grace period |
| `deterministic_tie_breaking` | Controls deterministic candidate tie-breaking |

The following fields are declared in `AutoFailoverConfig` but are not read by
the current runtime implementation: `failure_detection_interval`,
`failover_timeout`, `spare_activation_timeout`, `leader_election_timeout`,
`enable_automatic_failover`, `enable_leader_election`, and
`max_heartbeats_per_second`. Do not rely on them to change runtime behavior.

### `DisasterRecoveryConfig`

| Field | Effect |
|------|--------|
| `precheck_timeout` | Declared in the configuration but not read by the current runtime |
| `catchup_timeout` | Deadline for quorum polling; time spent inside a blocking dependency call is not bounded by it |
| `verification_timeout` | Divided across retry sleeps; it does not bound dependency-call duration or guarantee a strict overall deadline |
| `max_verification_retries` | Max health/quorum verification attempts before failure |
| `require_quorum` | Requires cluster quorum during prechecks/catchup/verification |
| `enforce_epoch_fencing` | Requires epoch fencing before restore / traffic cutover |
| `allow_dry_run_without_managers` | Currently has no effect; built-in dry-run steps bypass their manager checks regardless of this field |

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
