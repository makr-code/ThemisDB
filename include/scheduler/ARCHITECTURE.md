---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: api-contract
Canonical references: include/scheduler/*.h; src/scheduler/README.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler public-header architecture

`include/scheduler/` declares API types and component interfaces. It does not
make every integration enabled by default, nor does the presence of a header
establish ABI stability. For runtime behavior and known limitations, see the
[implementation architecture](../../src/scheduler/ARCHITECTURE.md).

## Header map

| Header | Declared surface | Namespace |
|---|---|---|
| `task_scheduler.h` | `ScheduledTask`, `TaskScheduler`, task config, execution results, and stats | `themis` |
| `distributed_task_coordinator.h` | Coordinator that uses a `TaskScheduler` and `sharding::DistributedCoordinator` | `themis` |
| `event_trigger.h` | `CDCTriggerConfig`, `EventTrigger`, `EventTriggerManager` | `themis` |
| `hybrid_retention_manager.h` | Retention config/stats and `HybridRetentionManager` | `themis` |
| `scheduler_api_contract.h` | `SchedulerError`, constants, and `isSchedulerFailClosed` | `themis::scheduler` (with `SchedulerError` re-exported by `themis`) |
| `external_scheduler_adapter.h` | External platform/config types and `ExternalSchedulerAdapter` | `themis::scheduler` |
| `task_audit_event.h` | Audit/security event and metrics types | `themis::scheduler` |
| `task_audit_manager.h` | Audit config/query/export types and `TaskAuditManager` | `themis::scheduler` |
| `task_result_store.h` | `TaskExecutionResult` and `TaskResultStore` | `themis::scheduler` |
| `task_anomaly_detector.h` | Detector config/statistics and `TaskAnomalyDetector` | `themis::scheduler` |

## Contract cautions

- Read each declaration and its implementation before treating comments in
  another module document as a guarantee.
- `SchedulerError` is not the return type for all scheduler failures.
- `TaskScheduler` checks a shared distributed-execution gate on immediate,
  scheduled, and DAG dispatch. Standalone schedulers have no attached gate.
- `isSchedulerFailClosed()` identifies only `kCoordinationError` and
  `kInternalError` as fail-closed. It does not encode every operation-specific
  rejection policy.
- Request-context permission helpers deny when the context is unset. Callers
  must establish a narrowly scoped context; `ScopedRequestContext` restores the
  prior context at scope exit.

The canonical implementation-level discussion is
[src/scheduler/ARCHITECTURE.md](../../src/scheduler/ARCHITECTURE.md).
