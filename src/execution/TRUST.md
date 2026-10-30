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

# Execution Trust Model

## Purpose

This page describes trust boundaries and residual risks for callers and
maintainers of the execution primitives. The canonical module threat inventory
and security controls remain in [SECURITY.md](SECURITY.md); repository-wide
vulnerability reporting follows the root `SECURITY.md` process.

## Trust boundaries

| Boundary | Trust assumption | Consequence |
|---|---|---|
| Caller to scheduler/pool | The caller is authorized by its host to submit a callable. | The module performs no authentication or authorization. Enforce caller policy before submission. |
| Submitted callable to worker | The callable is trusted in-process code. | It runs with the process's privileges; the pool provides no sandbox or privilege separation. |
| Queue and timeout configuration | The host supplies limits appropriate to its workload. | Limits are per primitive instance, and poorly chosen capacities/timeouts can still exhaust host resources or propagate overload. |
| Metrics to operator | The host interprets snapshots and exports them if needed. | Metrics are in memory only; there is no built-in durable audit trail or exporter. |
| Shutdown boundary | The host coordinates producers, consumers, and object lifetimes. | Shutdown rejects new work but does not cancel an executing callable or guarantee a timed-out drain completes all queued work. |

## Controls present in source

- Scheduler and pool queues are bounded by configuration.
- Admission and submission wait only up to the caller timeout.
- LOW scheduler work can be shed at the configured threshold.
- Both primitives reject new work after shutdown begins.
- Task exceptions are caught at the worker boundary and counted so one throwing
  callable does not directly terminate that worker loop.

These controls reduce unbounded queue growth and exception propagation; they do
not establish tenant isolation, prevent abusive work already accepted, or
protect unrelated process state from a malicious callable.

## Residual risks and handling

- Validate tenant, authorization, workload class, and callable provenance in
  the host before submission.
- Bound task runtime and resource use at the application layer; the module has
  no task cancellation or deadline enforcement.
- Avoid reference captures whose objects may be destroyed before task
  completion.
- Investigate spikes in shed/rejection/failure counters at the host layer. The
  pool does not retain exception details.
- Treat unexpected execution or a suspected malicious task as a host-process
  incident. Preserve host logs and telemetry, stop further submissions using
  the host's controls, and follow the repository security incident process.

No credential or key store exists in this module. Callables can nevertheless
access whatever secrets the process can access; callers must not pass secrets
into diagnostic task names or otherwise expose them through host logging.

See [SECURITY.md](SECURITY.md) for the module security controls,
[RUNBOOK.md](RUNBOOK.md) for operational response, and
[IMPLEMENTATION.md](IMPLEMENTATION.md) for the current failure semantics.

Exact model backend not verifiable from final document text alone.
