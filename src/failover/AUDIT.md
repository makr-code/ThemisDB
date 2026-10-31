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

# Failover Module Source Audit

**Audit date:** 2026-10-31
**Scope:** failover headers, implementation, focused tests, benchmark registration, and module documentation.

## Current source findings

| Area | Verified in repository | Limitation / risk |
|---|---|---|
| Automatic orchestration | `AutoFailoverManager` monitors health, tracks failures, queues work, checks quorum/fencing, requests replica promotion, and exposes events/statistics. | Spare activation and metadata update do not perform the implied external actions. `target_promote_id` is stored in the queued task but not used to select the candidate. |
| Health timeout | `getClusterHealth()` is invoked asynchronously and the monitoring cycle waits up to `health_check_call_timeout_ms`. A timed-out future may continue running; another health cycle is skipped while it remains pending. | `stop()` calls `join()` directly. Its source comments calculate deadlines, but those deadlines do not limit the join. |
| Quorum log | Optional `QuorumLog` appends delimited records with CRC32 and skips malformed or CRC-invalid records during recovery. Append/flush failure blocks the relevant path. | Startup logs recovered data; the manager does not restore epoch/quorum state from it. CRC32 is not authentication. |
| Disaster recovery | Plan strings are validated; seven steps run in fixed order; failed steps stop later steps; named results are cached in memory; concurrent uncached execution is rejected. | Built-in restore/traffic shift do not invoke adapters. Catch-up tests quorum, verification tests health/quorum, and there is no compensation for prior steps. |
| Topology | Snapshots copy failure counts and sort node IDs; the detector retries once if its before/after node sets differ. | `has_topology_change()` compares node sets, not version values; this is not a general concurrent-topology stress guarantee. |
| Tests/build | `tests/failover/CMakeLists.txt` globs `test_failover_*.cpp` and registers focused targets with `failover;release_critical` labels. Benchmark CMake defines three benchmark targets behind `THEMIS_BUILD_BENCHMARKS`. | This documentation audit checked source/registration only; it did not build targets or execute tests/benchmarks. |
| Observability | Manager state, statistics, callbacks, and `spdlog` calls exist. | No named Prometheus exporter or trace-span implementation was found in the failover runtime. |

## Audit status and references

This report records repository inspection, not a CI result, production-readiness
certification, or human approval. For aggregate maturity, use root
[`../../ROADMAP.md`](../../ROADMAP.md); module behavior is documented in
[`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), and
[`IMPLEMENTATION.md`](IMPLEMENTATION.md).
