# EPIC 2.4 Approximation Governance

<!-- Status: current | Phases 1-7 complete | validated: 2026-07-17 -->
<!-- Closes: #5440 -->
<!-- Links: docs/adr/adr-e2-004-approximation-governance-rules.md -->
<!-- Links: src/evaluation/include/approximation_rules.h -->
<!-- Links: src/evaluation/src/approximation_rules.cc -->
<!-- Links: tests/epic2_evaluation/approximation_rules_test.cc -->

## Summary

Approximation boundaries and governance rules for layered retrieval.  This document
defines the canonical zone mapping, the policy invariants, and the integration contract
for the `ApproximationRuleEngine` used by the hybrid query planner.

## Boundary / Policy Matrix

| Layer            | Default Zone | Truth-bearing | GPU-eligible | Fail-closed | Kernel max |
|------------------|:------------:|:-------------:|:------------:|:-----------:|:----------:|
| Ann              | Approximate  | No            | Cat A / B    | No          | A          |
| TensorSummary    | Bounded      | No (advisory) | Cat A / B    | No          | B          |
| ExactGraph       | Exact        | Yes           | **Never**    | **Yes**     | C          |
| DistributedShard | Bounded†     | Depends       | Cat A / B    | No          | B          |

† DistributedShard zone is dynamic: escalates to Exact per shard when shard-summary
confidence falls below `ApproximationPolicy::min_confidence_bounded`.

## Governance Invariants

1. **Category C fail-closed** — ACL enforcement, provenance-chain construction, and
   transaction consistency operations are always `ApproximationZone::Exact`.  Routing
   them to Approximate or Bounded is rejected immediately
   (`GovernanceDecision::Deny`).
2. **GPU ban on ExactGraph** — GPU dispatch to the ExactGraph layer is never permitted.
   This is enforced by `ApproximationBoundary::gpu_eligible = false` and checked via
   `checkBoundary()`.
3. **Zone strictness** — the requested zone must be ≥ the canonical minimum for the
   layer.  Requesting a looser zone triggers `GovernanceDecision::EscalateToExact`
   (or Deny for fail-closed layers).
4. **Confidence thresholds** — each zone has a policy-controlled minimum confidence
   score.  Bounded defaults to 0.80; Exact defaults to 1.0.  Below-threshold requests
   escalate or are denied accordingly.
5. **Policy provenance** — every `BoundaryCheckResult` carries the `policy_version`
   string for audit tracing (ADR E2-005).
6. **Bypass** — available only when `ApproximationPolicy::allow_bypass == true`.
   Every bypass decision is logged with the policy version.

## Repository Surfaces

| File | Role |
|------|------|
| `src/evaluation/include/approximation_rules.h` | Public API: types, interfaces, factory |
| `src/evaluation/src/approximation_rules.cc`    | Production implementation |
| `tests/epic2_evaluation/approximation_rules_test.cc` | 42 GTest cases (APB/GRV/POL/ESC/VPP/EDG) |

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Governance vocabulary defined: `ApproximationZone`, `RetrievalLayer`,
      `GovernanceDecision`, `ExactnessViolation`
- [x] Contract reviewed and stable before Phase 2 implementation

### Phase 2: Core implementation
- [x] `DefaultApproximationRuleEngine::checkBoundary()` — 7-rule priority chain
- [x] `DefaultApproximationRuleEngine::validatePlannedPath()` — all five ExecutionPaths
- [x] `DefaultApproximationRuleEngine::canonicalBoundary()` — per-layer lookup

### Phase 3: Error handling and edge cases
- [x] Category C fail-closed on any non-Exact zone or non-ExactGraph layer
- [x] ExactGraph zone-mismatch → EscalateToExact
- [x] Confidence below threshold → EscalateToExact (Approximate/Bounded) or Deny (Exact)
- [x] Unknown layer → Deny with `ExactnessViolation::UnknownLayer`
- [x] Unknown layer `canonicalBoundary()` returns safe default (fail-closed Exact)
- [x] Dynamic policy bypass path with explicit audit note

### Phase 4: Tests
- [x] APB-01..08: canonical boundary descriptors per layer
- [x] GRV-01..08: governance rule violations
- [x] POL-01..06: dynamic policy overrides and confidence thresholds
- [x] ESC-01..06: EscalateToExact triggers
- [x] VPP-01..08: `validatePlannedPath` for all five `ExecutionPath` values
- [x] EDG-01..06: edge cases (unknown layer, boundary confidence, `isAllowed()`)

### Phase 5: Performance and hardening
- [x] All check methods are `noexcept`; zero heap allocation for check paths
- [x] Canonical boundary table is `constexpr` array — O(1) lookup, no branching on layer
- [x] `isZoneStrictEnough()` is a `constexpr` predicate — inlined at call site
- [x] Bypass and escalation strategies documented in code and this governance doc

### Phase 6: Documentation and acceptance
- [x] Boundary/Policy matrix documented (above)
- [x] Seven-phase roadmap complete and marked `[x]`
- [x] ADR E2-004 updated to Accepted status
- [x] Repository surfaces stable and linked

### Phase 7: Integration
- [x] `src/evaluation/CMakeLists.txt` — `epic2_approximation_lib` registered
- [x] `tests/epic2_evaluation/CMakeLists.txt` — `approximation_rules_test` registered
- [x] `ApproximationRuleEngine::validatePlannedPath()` accepts `PlannerDecision` directly,
      enabling planner pipeline integration without type adapters

## Acceptance Signals

- [x] All 42 GTest cases pass (`ApproximationBoundaryTest`, `GovernanceRuleViolationTest`,
      `PolicyOverrideTest`, `EscalateToExactTest`, `ValidatePlannedPathTest`, `EdgeCaseTest`)
- [x] Repository surfaces stable and linked from CMake
- [x] Policy version is propagated on every `BoundaryCheckResult`
- [x] Category C fail-closed invariant verified by GRV-01..04

## References

- `docs/adr/adr-e2-004-approximation-governance-rules.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/adr/adr-e2-003-query-planner-routing-model.md`
- `docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md`
- `src/evaluation/include/query_planner.h`
