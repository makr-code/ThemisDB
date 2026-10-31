---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Updates Module State and Rollout Model

This document defines the control-state model for contributors changing update
orchestration. The implementation contract is
[`update_state_machine.h`](../../include/updates/update_state_machine.h) and
[`update_state_machine.cpp`](update_state_machine.cpp).

## Update state machine

The state machine serializes transitions among:

| State | Meaning |
|---|---|
| `IDLE` | No update is active; a new download may begin. |
| `DOWNLOADING` | Release files are being acquired. |
| `VERIFYING` | Release content is being checked before apply. |
| `APPLYING` | The update is being applied. |
| `ROLLING_BACK` | A rollback is in progress. |
| `FAILED` | An unrecoverable failure; only explicit `reset()` returns the machine to its initial state. |

Allowed edges are `IDLE → DOWNLOADING`; `DOWNLOADING → VERIFYING`,
`ROLLING_BACK`, or `FAILED`; `VERIFYING → APPLYING`, `ROLLING_BACK`, or
`FAILED`; `APPLYING → IDLE`, `ROLLING_BACK`, or `FAILED`; and
`ROLLING_BACK → IDLE` or `FAILED`. A rejected edge returns `false` and leaves
the state unchanged. Reaching `IDLE` clears the active version and in-flight
flag.

Accepted transitions append transaction entries and invoke callbacks after the
state lock is released. Callback exceptions are contained by the state machine.
These properties are important for callers that inspect the state from a
callback or coordinate work from multiple threads.

## Journal and checkpoint model

The optional transaction log is JSON-lines. The constructor reads valid entries,
skips blank or malformed lines, and marks a non-`IDLE`, non-`FAILED` final
entry as an in-flight update. This is a detection signal; it is not an
automatic recovery protocol. Checkpoints are assigned increasing IDs and may
be recorded in a separate checkpoint log; the API describes them as
in-memory snapshots for the state machine's lifetime. Do not treat a checkpoint
as a durable copy of installed files.

Rollback APIs include explicit checkpoint selection, latest-checkpoint
rollback, safety validation, and fallback strategies. `DEFER` records a
rollback for later retry; a caller must provide the retry policy and inspect
the outcome. Repeated rollback to the last checkpoint is idempotent according
to the header contract.

## Rollout models

- **Canary:** percentages are cumulative and node membership is derived from
  the version and stable node ID. The last configured stage must reach `1.0`.
  The controller has no cross-node coordination; orchestration belongs to its
  caller.
- **Blue/green:** one slot is active and one standby. Deploying to standby does
  not promote it; promotion requires a deployed standby, and a rollback uses
  the rollback ID from the underlying hot reload.
- **Coordination:** cluster and coordinated managers own multi-node status,
  quorum/ordering outcomes, and rollback policy. Per-node canary or blue/green
  state must not be mistaken for a cluster-wide transaction.

## Validation and edge cases

Tests should cover invalid edges, concurrent transition attempts, malformed
history, absent checkpoint IDs, repeated rollback, partial node failure, and
quorum/partition conditions. Relevant suites are
[`test_updates_determinism_edge_cases_focused.cpp`](../../tests/updates/test_updates_determinism_edge_cases_focused.cpp),
[`test_updates_rollback_hardening_focused.cpp`](../../tests/updates/test_updates_rollback_hardening_focused.cpp),
and [`test_updates_cluster_scheduling_stress_focused.cpp`](../../tests/updates/test_updates_cluster_scheduling_stress_focused.cpp).

> Exact model backend not verifiable from final document text alone.
