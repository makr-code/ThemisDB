# Sharding Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-10 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase C), §7 (risk) -->
<!-- Issue Link: makr-code/ThemisDB#5620 (development status snapshot) -->

## Current Status

Production-capable sharding runtime exists for routing/placement, distributed coordination, cross-shard transaction execution, and rebalancing/repair/operational observability.

**Hybrid Retrieval Rollout Readiness**: 35% 🔴 (issue #5468).
- Phase A (single-shard exact): ✅ Ready — single-shard path is stable.
- Phase B (multi-shard exact): ❌ Q3 2026 — blocked by 340+ cross-shard thread-safety gaps.
- Phase C (distributed summary-first): ❌ Q4 2026 — requires consensus coordination robustness.
- **Critical**: Multi-shard is disabled until Phase C thread-safety and lock-ordering gates pass.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## Recently Completed

- [x] **P6-01** (2026-07-20): 32 GTest cases for 2PC/3PC consistency verification delivered in
  `tests/sharding/test_sharding_phase6_hardening.cpp` (TXC-01..TXC-32). All cases pass with
  deterministic seed-42 simulation; commit/abort/WAL/replay guarantees validated in-process.
- [x] **P6-02** (2026-07-20): 20 GTest cases for failover logic and recovery-path hardening delivered in
  `tests/sharding/test_sharding_phase6_hardening.cpp` (FLR-01..FLR-20). Coordinator crash +
  WAL re-drive + idempotent recovery scenarios all verified.
- [x] **P6-03** (2026-07-22): 40 Wave-8 fault injection GTest cases delivered in
  `tests/sharding/test_sharding_p6_fault_injection.cpp` (FI-01..FI-40). Covers: network
  partition (FI-01..FI-15), coordinator failure (FI-16..FI-25), cascade/multi-failure
  (FI-26..FI-40). All 40 tests registered as `release_critical` in `tests/sharding/CMakeLists.txt`.
- [x] **P6 Sign-off** (2026-07-22): sign-off artefacts at `docs/sharding/SHARDING_P6_SIGN_OFF.md`.
  - [x] P6-01 sign-off: commit/abort/WAL/replay guarantees documented and validated (TXC-01..TXC-32 all green)
  - [x] P6-02 sign-off: failover/recovery deterministic evidence bundle completed (FLR-01..FLR-20 all green)
  - [x] P6-03 sign-off: Wave-8 fault-injection chain integrated into GA gate board (FI-01..FI-40 registered `release_critical`)

## In Progress

- [~] hardening distributed failure-path behavior under shard outage and quorum stress (Target: Q3 2026)
- [~] improving diagnostics consistency across routing/transaction/repair stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for sharding hot paths (Target: Q3 2026)
- [x] Real AWS S3, Azure Storage, and Google Cloud Storage SDK integrations for cloud backup (Target: Q2 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [x] Phase C pre-requisite: fix 70% of cross-shard thread-safety gaps (340+ → ~102) (delivered 2026-08-10)
  - Fixes: `dual_consensus_orchestrator.cpp` deadlock in `updateConsistencyState`, `getMetrics`
    lock-ordering violation (`metrics_mutex_` → `state_mutex_`), unprotected callback fields,
    unprotected `background_sync_interval_`, data race on `conflict_callback_` in
    `replica_consistency.cpp`, and detached-thread shared-state hazards.
  - Test evidence: TSO-01..TSO-08 in `tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp`
- [x] Phase C pre-requisite: consistent lock ordering enforcement — lock ordering violations (95 → 0) (delivered 2026-08-10)
  - Canonical order documented in headers: `state_mutex_` (1) < `audit_mutex_` (2) < `metrics_mutex_` (3)
    for `DualConsensusOrchestrator`; `state_mutex_` (1) < `callbacks_mutex_` (2) < `cluster_mutex_` (3) <
    `snapshot_mutex_` (4) for `RaftConsensusAdapter`.  `logGroundingOperation` now uses `std::scoped_lock`
    for atomic dual acquisition; `getMetrics` no longer holds `metrics_mutex_` while calling into
    `state_mutex_`-protected paths.
  - Test evidence: LKO-01..LKO-06 in `tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp`
- [x] Phase C pre-requisite: consensus coordination robustness (170 gaps → 51) (delivered 2026-08-10)
  - Fixes: `backgroundSyncThread` now detects quorum loss (null consensus layers) and backs off;
    retry logic added for transient `STORAGE_AHEAD`/`CACHE_AHEAD` sync failures; atomic interval read
    prevents torn reads of `background_sync_interval_ms_`.
  - Test evidence: CCR-01..CCR-06 in `tests/sharding/test_sharding_thread_safety_lock_order_focused.cpp`
- [~] Phase C ctest gate: `test_sharding_multishard_exact` under shard failure injection (explicitly registered as `ShardingMultiShardExactPhaseCGate`; full environment validation still blocked by current repo-wide build failures outside sharding) (Target: Q4 2026)
- [~] Phase C benchmark gate: `bench_multishard_exact` (deterministic benchmark hygiene tightened; full environment validation still blocked by current repo-wide build failures outside sharding) (Target: Q4 2026)
- [x] Phase C observability: `sharding_cross_shard_requests_total` Prometheus metric wired (Target: Q4 2026)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under sustained shard migration and skewed load (Target: Q4 2026)
- [ ] expand stress coverage for cross-shard transaction and anti-entropy edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for rebalance/repair incident triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for routing, commit, and migration-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced multi-DC and topology-failure scenarios (Target: Q1 2027)
- [ ] harden long-run reliability under sustained distributed write pressure (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

#### 3.2 Sharding

- [~] **Automatic shard rebalancing on topology change**: when a node joins or leaves the cluster,
  automatically redistribute shards to maintain target balance; rebalancing must complete within a
  configurable time bound without halting query throughput (Target: Q3 2026) — basic rebalance framework exists in `src/sharding/rebalance_operation.cpp`; topology-change automation remains unfinished
  - Inputs: topology change event, rebalance policy (min-movement / round-robin), target shard count
  - Acceptance: rebalance completes at ≥ 80% throughput of steady-state; no data loss; CTest `release_critical` green
- [ ] **Cross-datacenter latency-aware routing**: route cross-shard reads to the replica with lowest
  measured RTT in the requesting DC; fall back to nearest-replica on timeout (Target: Q3 2026)
  - Clarification: no `src/sharding/*.cpp` implementation evidence for latency-aware / multi-DC routing was found in this validation pass.
  - Acceptance: routing selects correct replica in 3-DC topology benchmark; p99 read latency
    improves vs. random routing; deterministic under-load benchmark result
- [ ] **Global secondary indexes (GSI)**: maintain a distributed index over all shards for a user-specified
  field; GSI updates are asynchronous and eventually consistent; index-backed range scan available in AQL (Target: Q4 2026)
  - Clarification: no `src/sharding/*.cpp` implementation evidence for GSI support was found in this validation pass.
  - Inputs: `CREATE INDEX … GLOBAL` DDL (via AQL DDL extension); field, type, consistency level
  - Acceptance: GSI scan returns correct results for 100K documents across 4 shards;
    AQL `FILTER doc.field == @val USE INDEX gsi_name` selects GSI plan

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze routing/coordination/transaction contracts for current major line — `include/sharding/sharding_api_contract.h` (§1 Routing, §2 2PC Transaction, §3 WAL Durability, §4 Migration/Rebalance, §6 Threading) (Target: Q3 2026)
- [x] define explicit error taxonomy for sharding failure classes — `include/sharding/sharding_api_contract.h` §5 Error Taxonomy: 12+ codes (QUORUM_LOST, COORDINATOR_FAILURE, SHARD_UNAVAILABLE, MIGRATION_CONFLICT, WAL_CORRUPTION, CONSENSUS_TIMEOUT, etc.) (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for routing/coordinator and transaction internals (Target: Q4 2026)
- [ ] align repair/rebalance/migration behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for quorum loss, migration faults, and repair failures (Target: Q4 2026)
- [ ] unify diagnostics across routing/transaction/operations incident classes (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for shard failure, transaction contention, and migration edge scenarios (P6-01: TXC-01..TXC-32 2PC/3PC consistency; P6-02: FLR-01..FLR-20 failover/recovery — in test_sharding_phase6_hardening.cpp; P6-03: FI-01..FI-40 Wave-8 fault injection — in test_sharding_p6_fault_injection.cpp; SCR-01..SCR-16 contract hardening — in `tests/sharding/test_sharding_contract_hardening_focused.cpp`) (Target: Q4 2026 → delivered 2026-07-29)
- [x] extend deterministic stress fixtures for distributed load and topology churn (P6-01/P6-02/P6-03 seed-42 deterministic suites delivered; SCR-01..SCR-16 kShardContractSeed=42) (Target: Q4 2026 → delivered 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for sharding hot paths — 6 release-gate benchmarks SRG-01..SRG-06 in `benchmarks/sharding/bench_sharding_release_gates.cpp` (GATE-SRG-01..GATE-SRG-06: consistent-hash routing, 2PC prepare/commit, WAL append, health check, route lookup) (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core sharding module docs aligned to source-verifiable behavior — `include/sharding/sharding_api_contract.h` freezes all routing/2PC/WAL/migration contracts for v1.x
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core sharding surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations — `benchmarks/sharding/bench_sharding_release_gates.cpp` (SRG-01..SRG-06)
- [ ] remaining hardening tasks closed for failure/transaction/repair edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on topology size, quorum profile, and migration pressure.
- selected failure and topology-churn edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced distributed workloads.

## Breaking Changes

No breaking sharding contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.