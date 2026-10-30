---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Research Notes

## Purpose and scope

This note records the assumptions and evaluation evidence for `src/access_model`.
It is for developers deciding whether the module's tier-placement policy fits a
cache/storage integration. It is not a storage benchmark report or an embedding
quality study.

The module addresses coordination between three cache labels
(`L1_WORKING`, `L2_EPISODIC`, `L3_SEMANTIC`) and three storage labels
(`STORAGE_HOT`, `STORAGE_WARM`, `STORAGE_COLD`). Its policy is deterministic:
it uses access counts and elapsed time, not a trained model or workload
prediction.

## Assumptions and method

`AgeBasedPolicy` provides reusable predicates and a HOT/WARM/COOL/COLD
classification. Defaults in `include/access_model/age_based_policy.h` include
1/7/14 days for L1/L2/L3 inactivity thresholds, 14/30/45/90 days for storage
aging thresholds, and promotion counts of 10/5/2/3 for L1/L2/L3/storage.
These are code defaults, not deployment recommendations or measured optima.

The coordinator's event handlers implement their own narrower decisions:

- With no policy set, `onEviction` and `onHotAccess` count the event and return
  without producing a transition decision.
- With a policy set, an eviction is retained at or above the L1 threshold;
  otherwise it is queued for `STORAGE_WARM` only when its last-access age is
  greater than `hot_zero_access_days` and that tier is registered.
- A hot-access event at or above `storage_promotion_threshold` targets
  `L3_SEMANTIC`, except a cold-storage event targets `STORAGE_WARM` when that
  tier is registered.

These event-handler rules are distinct from the policy helper predicates. Do
not assume every predicate or threshold field is used by the coordinator
event path.

## Evaluation and reproducibility

Focused, policy, integration, concurrency, observability, and end-to-end tests
are under [`../../tests/access_model/`](../../tests/access_model/). The
performance expectations map six benchmark cases to
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) and
[`WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md`](./WAVE_B_BENCHMARK_EVIDENCE_BUNDLE.md).
Those documents define targets; the evidence bundle says representative
hardware baselines still need re-capture before Wave B GA promotion.

For repeatable evaluation, run the focused module tests and the release-profile
benchmark cases on the target hardware, retain the benchmark manifest, and
compare each mapped case to its recorded baseline. A target value alone is not
evidence that a run passed.

## Trade-offs and limits

- Count-and-age rules are explainable and inexpensive, but do not adapt to
  changing workloads or account for cost, tenant, or key-specific policy.
- `AgeBasedPolicy::isValid()` checks threshold relationships, but
  `AccessCoordinator::setAgePolicy()` does not call it.
- The current coordinator worker records a successful promotion result without
  invoking tier `get`/`put` operations. Its event and benchmark evidence must
  therefore not be interpreted as proof that bytes were moved between tiers.
- The coordinator queue has no configured depth limit in the implementation.

The canonical interface is in [`API.md`](./API.md); implementation limits and
validation commands are in [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
