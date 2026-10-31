---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Operations Guide

## Purpose and supported use

`FaultInjector` and `ChaosScheduler` provide process-local fault-state simulation for test and integration consumers that explicitly query `isFaultActive()`. They do not perform real disk or network manipulation, do not persist state across process restarts, and are not a production control plane.

Server source contains a guarded admin handler and `/admin/chaos/*` route
dispatch. CMake defaults `THEMIS_CHAOS_ADMIN` to `OFF` and conditionally adds the
handler source, but the macro is defined only on `themis_server` while
`http_server.cpp` is compiled into `themis_core`; additionally, the server code
uses a no-argument `ChaosScheduler` constructor that is not declared by the
public API. **TODO:** build and fix the enabled integration before treating the
endpoint as usable. Although the guarded route contains an admin-access check,
this does not establish a live route or deployment configuration. CMake warns
not to ship the option in production.

## Operational signals

The core `FaultInjector`/`ChaosScheduler` implementation exports no Prometheus
metrics, traces, or alerts and does not emit operational logs. The optional
server handler contains logging calls, but its build integration is unresolved
(see above); do not rely on it as an operational signal. The supported core
inspection and control surfaces are:

| Signal/control | Meaning and action |
|---|---|
| `getActiveFaults()` / `activeFaultCount()` | Current unexpired fault registry entries; capture before recovery and require zero at test completion. |
| `isFaultActive(node_id, type)` | Verifies whether a specific simulated fault remains active. |
| `pendingCount()` / `isRunning()` | Scheduler backlog and worker state; clear pending work explicitly if it must not fire. |
| `EventCallback(FaultSpec, injected)` | Synchronous lifecycle notification for injection and individual recovery; consumers must log it if an audit trail is required. |

Alerts must be owned by the test harness or surrounding deployment. Alert on unexpected active faults or pending entries after the approved test window, and include target/type in the emitted event. Do not infer real node, disk, or network health from these simulator signals.

## Routine operating controls

- Require change approval and an isolated target before scheduling any fault.
- Prefer finite positive durations; document that zero means permanent until manually cleared.
- Validate the fault target, type, probability, trigger time, and expected recovery before starting.
- At completion, stop the scheduler, clear unwanted pending entries, recover active faults individually, and verify an empty registry.
- Keep callbacks short and non-reentrant. Recovery callbacks run synchronously under the injector mutex, and callback exceptions propagate to the caller.

## Limits relevant to operations

The v1 runtime validates empty target IDs and probability range on injection, but does not enforce the contract's recommended maximum durations, tick interval, pending-entry count, or callback count. Callers must bound these values. Stopping the scheduler prevents firing while stopped; queued entries remain process-local until explicitly cleared or the process exits.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/chaos/chaos_framework.h`
- `include/chaos/chaos_contract.h`
