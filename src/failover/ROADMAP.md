---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Failover Module Roadmap

> Root [`../../ROADMAP.md`](../../ROADMAP.md) is authoritative for aggregate
> release maturity and readiness. This page summarizes source-visible module
> scope and identifies follow-up work; historical wave labels are not test or
> production sign-off.

## Current implementation

- Automatic health/failure tracking, bounded request queue, quorum/fencing
  checks, replica promotion requests, state/results, events, and statistics.
- Optional local quorum log and deterministic topology snapshot helpers.
- Disaster-recovery plan validation and fixed seven-step coordination with
  adapter acknowledgements, independent proof hooks, a local checksummed
  completion log, and plan-ID replay across manager restart.
- Focused test sources are registered through `tests/failover/CMakeLists.txt`
  with `failover` and `release_critical` labels. Three Google Benchmark
  executables are registered when benchmarks are enabled.

This inventory confirms code and CMake declarations only; it does not claim
tests or benchmarks passed. See [`AUDIT.md`](AUDIT.md) for audit limitations.

## Known implementation limits

- `stop()` uses direct `join()` calls. Source comments mention deadlines, but no
  timeout is enforced on either join.
- A timed-out health-check future can continue running; later monitoring cycles
  are skipped until that future completes.
- `target_promote_id` is queued but ignored when selecting a promotion candidate.
- Quorum-log recovery is logged, not applied as restored manager state.
- Spare activation requires a runtime hook and independently observed `ACTIVE`
  state; failure blocks completion and records the degraded state/event/counter.
- Non-dry-run DR requires operation and verification hooks, quorum, fencing, and
  a writable local completion log. The manager checks checksum, LSN/term, and
  recovery-site evidence before accepting each step.
- DR metadata is local-file persistence, not shared multi-process storage;
  concurrent multi-process writer safety and crash/power-loss durability are
  not established.
- Shard write-side epoch-token enforcement and multi-process failover/recovery
  E2E evidence remain unimplemented release blockers.
- Current failover runtime does not export named Prometheus metrics or trace spans.
- Existing benchmark source includes mock/simulated workloads and declares
  thresholds; source presence does not establish measured latency.

## Follow-up documentation and implementation questions

- Decide whether `stop()` requires a genuinely bounded shutdown contract; if so,
  implement and test it before documenting a maximum wait.
- Integrate shared durable metadata, shard-write fencing, and multi-process
  failover/recovery E2E evidence before treating the DR rollout as ready.
- Establish whether quorum-log replay should restore any manager state and define
  the persistence/consistency contract before making durability claims.
- Validate performance targets on representative hardware and distinguish
  workload simulations from end-to-end measurements.

## References

- [`API.md`](API.md) · [`ARCHITECTURE.md`](ARCHITECTURE.md) ·
  [`IMPLEMENTATION.md`](IMPLEMENTATION.md)
- [`tests/failover/CMakeLists.txt`](../../tests/failover/CMakeLists.txt)
- [`benchmarks/failover/CMakeLists.txt`](../../benchmarks/failover/CMakeLists.txt)
- Public contract: `include/failover/failover_api_contract.h`
