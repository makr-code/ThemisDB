---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Failover Module Architecture

## Scope and ownership

The `failover` module coordinates automatic failover requests and disaster
recovery plans. Public declarations live in `include/failover/`; implementation
is in `src/failover/`. It relies on replication, health-monitor, spare, and
epoch-fencing managers supplied by its caller. It does not own authentication,
the storage restore implementation, or traffic-management infrastructure.

## Components and runtime flow

1. `AutoFailoverManager` observes health through its configured dependencies,
   tracks consecutive failures, and accepts manual requests into a bounded
   queue. A worker checks quorum and fencing before selecting a healthy
   non-witness replica and asking the replication manager to trigger failover.
2. The manager reports lifecycle events, state, statistics, and the latest
   result through in-process API surfaces. Queue operations and state/statistics
   use internal synchronization; callers must not treat this as an externally
   exported metrics contract.
3. `DisasterRecoveryManager` validates required plan fields, rejects concurrent
   uncached executions, and runs its fixed ordered step pipeline. A failed step
   stops later steps; the manager does not compensate earlier steps.
4. `QuorumLog` optionally appends local pipe-delimited records with CRC32.
   `TopologySnapshot` copies failure counts and sorts node IDs for stable
   comparisons.

## Contracts and failure domains

| Boundary | Contract | Failure result |
|---|---|---|
| Request queue | Capacity is bounded by configuration | Submission while stopped or at capacity returns `false`; a full request is counted as dropped |
| Replication manager | Supplies quorum, replica health/roles, and the promotion request | Missing dependency, unavailable quorum, or failed promotion blocks or fails the operation |
| Epoch fencing manager | Supplies a non-zero epoch before guarded promotion | Missing manager when required or epoch 0 fails closed |
| DR plan | Required IDs are checked before step execution | Validation or step failure returns a failed result; completed steps are not rolled back |
| Optional quorum log | Appends decisions when configured | Append failure blocks the corresponding quorum/promotion path; CRC32 is not authentication |

## Integration boundary and incomplete actions

The current implementation does not execute every action implied by the
orchestration names. Spare activation emits `SPARE_ACTIVATION_FAILED` when a
spare manager is present, and its failure result is not used to abort the
overall failover. Metadata update logs the old/new leader but performs no
metadata write. Built-in disaster-recovery snapshot validation checks only ID
presence; restore and traffic shift report success without invoking external
adapters. Catch-up tests quorum, and verification checks health/quorum rather
than restored contents or recovery-site identity. A successful result therefore
does not prove those external effects occurred. See
[IMPLEMENTATION.md](IMPLEMENTATION.md).

## Observability boundary

The current `auto_failover_manager.cpp` and
`disaster_recovery_manager.cpp` use `spdlog`, callbacks, state, and statistics.
They do not publish named Prometheus metrics or create trace spans. Deployment
monitoring must consume an explicitly configured admin/export surface; do not
configure alerts against metric names or spans that this module does not emit.

## Related documentation

- [API.md](API.md) — public interface contract
- [OPERATIONS.md](OPERATIONS.md) — in-process signals and operator safeguards
- [TRUST.md](TRUST.md) — dependency trust boundaries