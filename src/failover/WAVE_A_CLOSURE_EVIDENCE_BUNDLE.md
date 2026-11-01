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

# Wave A Failover Evidence — Source-Checked Status

> Historical evidence index, rechecked against the current source on 2026-10-31.
> This page is not a CI execution report or release sign-off. Earlier PASS,
> bounded-time, and persistence claims are not accepted without current test or
> runtime evidence.

## Source-visible features

| Topic | Current source behavior | Test source |
|---|---|---|
| Health-call timeout | Waits for `health_check_call_timeout_ms`; reports a diagnostic on timeout. The async operation may continue, and later cycles are skipped while it remains pending. | `tests/failover/test_failover_wave_a_health_timeout.cpp` |
| Epoch fencing | `preventSplitBrain()` rejects missing manager/epoch 0 when enabled. Candidate selection separately requests a fencing epoch when a fencing manager exists. | `tests/failover/test_failover_wave_a_fencing.cpp` |
| Quorum log | Appends records, validates CRC on recovery, skips invalid records; manager startup logs recovered state but does not restore state from it. | `tests/failover/test_failover_wave_a_quorum_persistence.cpp` |
| Topology snapshot | Captures sorted IDs and copied failure counts; the detector compares node sets before/after and retries once on membership change. | `tests/failover/test_failover_wave_a_topology_versioning.cpp` |

The quorum persistence test's `Integration` case exercises `QuorumLog` directly;
its source explicitly does not instantiate the manager integration. The current
test source establishes test intent and assertions, not a passing execution.

## Open evidence limits

- The computed deadlines in `AutoFailoverManager::stop()` do not bound `join()`.
- Quorum-log records do not restore manager epoch/quorum state after restart.
- The topology snapshot predicate does not classify version-only differences as
  topology changes.
- No test or benchmark was run as part of this documentation audit.

See [`AUDIT.md`](AUDIT.md), [`IMPLEMENTATION.md`](IMPLEMENTATION.md), and the
linked test sources for current evidence boundaries.
