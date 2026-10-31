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
  step hooks and in-memory caching for named plans.
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
- Spare activation, metadata writes, DR restore, and DR traffic shift do not
  perform their implied external operations.
- DR catch-up is a quorum check, not replication-position synchronization;
  verification checks cluster health/quorum, not recovered contents/site identity.
- Current failover runtime does not export named Prometheus metrics or trace spans.
- Existing benchmark source includes mock/simulated workloads and declares
  thresholds; source presence does not establish measured latency.

## Follow-up documentation and implementation questions

- Decide whether `stop()` requires a genuinely bounded shutdown contract; if so,
  implement and test it before documenting a maximum wait.
- Integrate and verify external adapters for restore, traffic, metadata, spare
  activation, and recovered-state validation before treating results as evidence
  of those effects.
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
