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

# Chaos Module Embedding Boundaries

## Purpose

This page clarifies what it means to embed or integrate `src/chaos` into a
test harness. The module has no vector, text, or semantic embedding interface;
it must not be confused with ThemisDB's embedding or retrieval subsystems.

## Supported integration

An in-process consumer may own a `FaultInjector`, optionally share it with a
`ChaosScheduler`, and query the registry from test logic. The consumer defines
the actual behavior associated with a descriptor. The module itself does not
wrap network clients, storage engines, process management, or production
traffic. Its public surface is documented in [API.md](API.md), and an example
integration sequence is in [MODULE_GUIDE.md](MODULE_GUIDE.md).

## Constraints for harness authors

- Keep the injector and scheduler lifetime within the test or controlled
  integration process.
- Use isolated test/staging targets and explicit, finite durations. A zero or
  negative duration is permanent until recovery.
- Start the scheduler only when the harness is ready to accept due entries.
  Stopping it leaves queued entries pending; clear them explicitly if they
  must not be replayed after restart.
- The consumer must implement and verify the effect and recovery. A stored
  `NETWORK_PARTITION` or `DISK_FAILURE` label is not an actual I/O fault.
- Record callback events in the harness if an audit trail is needed; the
  module does not persist event history.

There is no model download, vector index, embedding generation, or external
service dependency in this integration path. See
[TRUST.md](TRUST.md) for the corresponding trust boundary and
[OPERATIONS.md](OPERATIONS.md) for cleanup signals.
