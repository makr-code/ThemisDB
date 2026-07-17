# ADR E2-004: Approximation governance rules

<!-- Status: Accepted | validated: 2026-07-17 -->
<!-- Closes: #5440 -->

## Status

Accepted

## Context

The hybrid query planner (EPIC 2.5) routes requests across four retrieval layers
(ANN, TensorSummary, ExactGraph, DistributedShard), each with different accuracy
and cost characteristics.  Without an explicit governance contract, approximate
results could silently propagate to truth-bearing decisions (ACL enforcement,
provenance construction, transaction consistency).

Issue #5440 requires that every layer carry a canonical approximation zone, and
that the planner enforce per-layer and per-policy rules at request time with
machine-readable violation codes.

## Decision

### 1. Canonical zone mapping (design-time invariants)

| Layer            | Zone        | Truth-bearing | GPU       | Fail-closed |
|------------------|-------------|:-------------:|-----------|:-----------:|
| Ann              | Approximate | No            | Cat A / B | No          |
| TensorSummary    | Bounded     | No (advisory) | Cat A / B | No          |
| ExactGraph       | Exact       | Yes           | **Never** | **Yes**     |
| DistributedShard | Bounded†    | Depends       | Cat A / B | No          |

† DistributedShard escalates to Exact per shard when shard-summary confidence
falls below the policy threshold.

### 2. Rule priority chain (enforced in `checkBoundary()`)

Rules are evaluated in order; the first match terminates evaluation:

1. Category C fail-closed — any Category C kernel on a non-Exact zone is `Deny`.
2. Category C on non-ExactGraph layer — `Deny`.
3. ExactGraph zone mismatch — any zone ≠ Exact on ExactGraph → `EscalateToExact`.
4. Policy ACL / provenance / transaction override — mirrors Rule 1/2 per policy flag.
5. Zone strictness — requested zone < canonical minimum → `EscalateToExact` (or
   `Deny` for fail-closed layers).
6. Confidence threshold — below minimum for requested zone → `EscalateToExact`
   (Approximate/Bounded) or `Deny` (Exact).
7. Truth-bearing / advisory gate — truth-bearing layer with non-Exact zone and no
   bypass → `Deny`; with bypass → `Bypass` (audited).

### 3. Policy provenance

Every `BoundaryCheckResult` carries the `policy_version` string from
`ApproximationPolicy`.  This satisfies the confidence-contract tracing requirement
from ADR E2-005.

### 4. Bypass mechanism

Bypass is opt-in (`ApproximationPolicy::allow_bypass = false` by default).
Every bypass decision produces a `GovernanceDecision::Bypass` result with an
explicit audit note in the explanation string.

### 5. Integration point

`ApproximationRuleEngine::validatePlannedPath()` accepts a `PlannerDecision`
directly and maps execution paths to their implied layers and zones, enabling
lightweight post-hoc validation without modifying the planner core.

## Consequences

### Expected benefits

- Approximate results can never silently reach truth-bearing decisions when
  the rule engine is invoked.
- Fail-closed enforcement for Category C / ExactGraph is type-safe and tested
  by 42 GTest cases.
- Policy changes (confidence thresholds, bypass) require only `ApproximationPolicy`
  updates, not code changes.
- Machine-readable violation codes enable dashboards and alerting.

### Trade-offs and rejected options

**Rejected: in-planner inline checks** — embedding zone checks inside
`query_planner.cc` would have coupled governance policy to planner logic,
making policy updates a planner code change.  A separate engine decouples the
two concerns.

**Rejected: runtime policy reload** — dynamic policy reload was out of scope for
this phase; `ApproximationPolicy` is a value type passed at call time by the
caller, which can load from any policy store.

### Compatibility

No existing planner behavior is changed; `ApproximationRuleEngine` is additive.
The `validatePlannedPath()` integration hook can be inserted into the planner
pipeline at any point without altering the planner's path-selection logic.

## Follow-up

- [ ] Wire `validatePlannedPath()` into `DefaultQueryPlanner::selectPath()` for
      automatic post-decision governance (Target: Q4 2026).
- [ ] Add `PlannerObserver` callback for governance violations (Target: Q4 2026).
- [ ] Extend `ExactnessViolation` codes for shard-level exactness escalation audit
      trail (Target: Q1 2027).

## References

- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/src/approximation_rules.cc`
- `tests/epic2_evaluation/approximation_rules_test.cc`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- ADR E2-003 — query-planner routing model
- ADR E2-005 — cross-layer fallback confidence policy
