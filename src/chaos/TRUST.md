---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Module Trust Boundaries

## Purpose and threat boundary

This document is for maintainers and operators assessing whether chaos
simulation is safe to enable. The trust boundary is the process containing
`FaultInjector` and `ChaosScheduler`. The implementation maintains in-memory
descriptors and a scheduler queue; it does not perform I/O, cross-process
communication, authentication, authorization, or persistent audit logging.
See the process-local guarantees in
[`include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h).

## Risks and controls

| Risk | Current control or required caller action |
|---|---|
| A test assumes a descriptor causes a real node, network, or disk failure | Treat `FaultType` as a label only. The test consumer owns the simulated effect and must verify it independently. |
| Unapproved or overly broad scenarios affect a shared target | Run only in explicitly approved, isolated test/staging processes. The module has no authorization gate; enforce approval and target allowlisting in the harness. |
| Permanent or stale work fires unexpectedly | Use finite positive durations; inspect active and pending state; call `clearPending()` and recover active entries. `stop()` alone retains queued entries. |
| Invalid or excessive caller input consumes resources | Runtime injection rejects empty IDs and out-of-range probabilities only. Callers must bound tick intervals, durations, queue size, identifier/description size, and callback count. |
| Callback behavior destabilizes control flow | Register callbacks before concurrent use; keep them bounded and non-throwing. Recovery callbacks must not re-enter the same injector; scheduler-thread callback exceptions are not caught. |
| State is mistaken for durable evidence | The registry and queue are process-local. Persist any required event history in the test harness; do not infer state after restart. |

The current module exposes no remote chaos endpoint. A planned or separately
declared admin integration must not be treated as live, authenticated, or
authorized unless its server wiring and access controls are independently
verified.

## Incident handling

If a scenario has an uncertain target or blast radius, stop the test, stop the
scheduler, clear unwanted pending entries, and recover known active descriptors
individually. Investigate real subsystem behavior independently; simulator
state is not proof of a real infrastructure fault. Escalate callback
exceptions, scheduler lifecycle failures, or unreconciled shared-environment
effects to the test owner. Follow [RUNBOOK.md](RUNBOOK.md) for diagnosis and
[OPERATIONS.md](OPERATIONS.md) for supported signals.
