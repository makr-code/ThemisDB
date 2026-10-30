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

# Access Model Data and Policy Model

## Purpose

This document defines the data vocabulary and decision rules implemented by
`access_model`. It is for developers changing policy or integrating a tier
adapter. The module models placement and transition requests; it does not
define application authorization rules.

## Tier and event data

`TierLevel` in `include/access_model/access_tier_interface.h` names six
locations: `L1_WORKING`, `L2_EPISODIC`, `L3_SEMANTIC`, `STORAGE_HOT`,
`STORAGE_WARM`, and `STORAGE_COLD`, plus `UNKNOWN`. `AccessTier` provides
string-keyed `get`, `put`, and `invalidate` operations and reports tier
capacity, access counts, age, health, and latency. `CacheTier` and `StorageTier`
add their respective eviction and hot-access notifications.

Coordinator input is represented by `EvictionEvent` and `AccessEvent`. Both
carry a key and tier; the first also carries eviction reason, size, access
count, and last-access age, while the second carries an access count and access
window. `PromotionResult`, `DemotionPlan`, `DemotionResult`, and
`AccessTransitionEvent` capture requested or reported outcomes. Correlation IDs
are strings and may be empty when an event is directly supplied by a caller.

## Policy parameters and rules

The defaults are part of `AgeBasedPolicy` in
`include/access_model/age_based_policy.h`. Inactivity thresholds are 1 day for
L1, 7 days for L2, 14 days for L3-to-storage, 14 days for hot storage, and
45 days for warm storage. Write-age thresholds are 30 days hot-to-warm and
90 days warm-to-cold. Access-count thresholds default to 10, 5, 2, and 3 for
L1, L2, L3, and storage.

The inline `shouldDemoteL1ToL2` and `shouldDemoteL2ToL3` helpers return false
when their configured day threshold is zero; otherwise they use a strict `>`
comparison. Hot-to-warm and warm-to-cold predicates return true when either
the zero-access age or write-age threshold is exceeded. Other predicates,
`classifyHotness`, and `recommendTierForData` are separately implemented in
`age_based_policy.cpp`; their behavior should be tested directly when changed.

`isValid()` requires nonzero access thresholds, strictly decreasing cache
promotion thresholds, strictly increasing L1/L2/L3 age thresholds, and
`hot_to_warm_days < warm_to_cold_days`. It does not validate every duration or
threshold field. The coordinator does not enforce this check when a policy is
set.

The coordinator's event decisions are narrower than this policy API. See
[`RESEARCH.md`](./RESEARCH.md) for the exact eviction and hot-access behavior.
Treat these as separate contracts rather than assuming every policy helper is
used by the event loop.

## State and evaluation boundaries

The coordinator retains a tier map, one policy value, a FIFO task queue,
pending demotion plans, aggregate counters/histograms, and transition records.
The queue has no bound. Event-handler paths trim their history after 1,000
records, but worker-completion records do not apply the same trimming rule; the
history therefore has no consistent cap. No per-key policy table or learned
state is defined.

Unit tests in `tests/access_model/` cover threshold boundaries, event handling,
concurrency, and observed outcomes. Tests using fake tiers establish coordinator
behavior, not durability or correctness of a production tier adapter.

For interface field definitions see [`API.md`](./API.md); for current worker
semantics and limitations see [`IMPLEMENTATION.md`](./IMPLEMENTATION.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
