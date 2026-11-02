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

Source level: 1
SOT domain: module-behavior
Canonical references: `include/sharding/epoch_fencing.h`, `include/sharding/two_phase_commit_participant.h`, `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_client_adapter.h`, `include/sharding/distributed_transaction.h`, `include/sharding/cross_shard_transaction.h`, `src/sharding/epoch_fencing.cpp`, `src/sharding/two_phase_commit_participant.cpp`, `src/sharding/two_phase_commit_coordinator.cpp`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/distributed_transaction.cpp`, `src/sharding/cross_shard_transaction.cpp`, `tests/test_epoch_fencing.cpp`
Related milestone: DOC-WEEKLY-2026-W45

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
- Shard transaction write fencing: exact current-epoch and target-shard checks;
  guarded storage apply; 2PC PREPARE credential serialization and participant
  WAL retention; atomic credential revalidation before COMMIT apply. Missing or
  invalid providers/tokens fail closed on distributed and cross-shard
  PREPARE/SAGA write paths. Phase 2 failures remain unresolved until participant
  acknowledgement; remote abort adapters return actual ACK status.
- Focused test sources are registered through `tests/failover/CMakeLists.txt`
  with `failover` and `release_critical` labels. Three Google Benchmark
  executables are registered when benchmarks are enabled. The Wave A fencing
  test source covers real hot-spare activation followed by independent `ACTIVE`
  state read-back, and rejects adapter acknowledgement without that state.

This inventory confirms code and CMake declarations only; it does not claim
tests or benchmarks passed. See [`AUDIT.md`](AUDIT.md) for audit limitations.

**Evidence note (2026-11-02):** source-based check coverage was added for the
write-fencing synchronization but was not executed. A diagnostic CMake configure
was attempted and could not complete because `fmt` is unavailable; apt
installation was denied. This is not test-pass evidence and does not establish
M2 readiness.

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
  recovery-site evidence before accepting each step. An existing unreadable or
  malformed/checksum-invalid completion log blocks execution before adapters.
- DR metadata is local-file persistence, not shared multi-process storage;
  concurrent multi-process writer safety and crash/power-loss durability are
  not established.
- Remote gRPC PREPARE binds the coordinator ID to the authenticated X.509 peer
  identity before checking the fencing-token issuer; standalone token signatures
  are not implemented, and direct in-process calls remain within the trusted
  process boundary. The binding test is source-level coverage until executed.
- Shared durable metadata for DR and genuine multi-process failover/recovery E2E
  evidence remain open release blockers.
- Current failover runtime does not export named Prometheus metrics or trace spans.
- Existing benchmark source includes mock/simulated workloads and declares
  thresholds; source presence does not establish measured latency.

## Follow-up documentation and implementation questions

- Decide whether `stop()` requires a genuinely bounded shutdown contract; if so,
  implement and test it before documenting a maximum wait.
- Integrate shared durable metadata and genuine multi-process failover/recovery
  E2E evidence before treating the DR rollout as ready. Shard transaction
  fencing is implemented on the documented paths, but does not close these
  blockers or constitute M2 readiness.
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
