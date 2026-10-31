---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Execution Capabilities

## Problem and module role

The execution module provides bounded, in-process queueing primitives for
components that need explicit admission and task-dispatch limits. It is a library
component, not a distributed scheduler or a request-execution service.

## Capabilities

- `QueryScheduler` admits work to a bounded queue and dequeues entries by earliest
  absolute deadline, breaking equal-deadline ties by query id.
- Low-priority entries can be rejected at a configured shed threshold.
- `WorkStealingThreadPool` runs submitted callables on a fixed worker set from a
  bounded central dispatch queue.
- Both primitives expose in-memory metrics and reject new work after shutdown.

## Operational value and limitations

Bounded admission and explicit rejection outcomes let a host apply its own
backpressure policy. The module does not currently implement per-worker stealing,
elastic worker scaling, task cancellation, automatic expiry, tenant isolation, or
a metrics exporter. `HttpServer` conditionally constructs and tears down the
primitives, but does not currently route request work through them.

## Developer references

- [README.md](README.md) — scope, quickstart, interfaces, and known limitations
- [ARCHITECTURE.md](ARCHITECTURE.md) — components, runtime flow, and failure paths
- [ROADMAP.md](ROADMAP.md) — current status and remaining work
- [SECURITY.md](SECURITY.md) — trust boundaries and controls

Exact model backend not verifiable from final document text alone.
