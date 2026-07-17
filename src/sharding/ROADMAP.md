# Sharding Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase C), §7 (risk) -->

## Current Status

Production-capable sharding runtime exists for routing/placement, distributed coordination, cross-shard transaction execution, and rebalancing/repair/operational observability.

**Hybrid Retrieval Rollout Readiness**: 35% 🔴 (issue #5468).
- Phase A (single-shard exact): ✅ Ready — single-shard path is stable.
- Phase B (multi-shard exact): ❌ Q3 2026 — blocked by 340+ cross-shard thread-safety gaps.
- Phase C (distributed summary-first): ❌ Q4 2026 — requires consensus coordination robustness.
- **Critical**: Multi-shard is disabled until Phase C thread-safety and lock-ordering gates pass.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] hardening distributed failure-path behavior under shard outage and quorum stress (Target: Q3 2026)
- [~] improving diagnostics consistency across routing/transaction/repair stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for sharding hot paths (Target: Q3 2026)
- [x] Real AWS S3, Azure Storage, and Google Cloud Storage SDK integrations for cloud backup (Target: Q2 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [ ] Phase C pre-requisite: fix 70% of cross-shard thread-safety gaps (340+ → ~102) (Target: Q3 2026)
- [ ] Phase C pre-requisite: consistent lock ordering enforcement — lock ordering violations (95 → 0) (Target: Q3 2026)
- [ ] Phase C pre-requisite: consensus coordination robustness (170 gaps → 51) (Target: Q4 2026)
- [~] Phase C ctest gate: `test_sharding_multishard_exact` under shard failure injection (implemented; environment validation pending) (Target: Q4 2026)
- [~] Phase C benchmark gate: `bench_multishard_exact` (implemented; environment validation pending) (Target: Q4 2026)
- [ ] Phase C observability: `sharding_cross_shard_requests_total` Prometheus metric wired (Target: Q4 2026)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under sustained shard migration and skewed load (Target: Q4 2026)
- [ ] expand stress coverage for cross-shard transaction and anti-entropy edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for rebalance/repair incident triage (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for routing, commit, and migration-sensitive paths (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced multi-DC and topology-failure scenarios (Target: Q1 2027)
- [ ] harden long-run reliability under sustained distributed write pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze routing/coordination/transaction contracts for current major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for sharding failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for routing/coordinator and transaction internals (Target: Q4 2026)
- [ ] align repair/rebalance/migration behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for quorum loss, migration faults, and repair failures (Target: Q4 2026)
- [ ] unify diagnostics across routing/transaction/operations incident classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for shard failure, transaction contention, and migration edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for distributed load and topology churn (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for sharding hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core sharding module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core sharding surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for failure/transaction/repair edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on topology size, quorum profile, and migration pressure.
- selected failure and topology-churn edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced distributed workloads.

## Breaking Changes

No breaking sharding contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.