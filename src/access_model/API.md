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

# Access Model API Contract

## Purpose

This reference describes the C++ interfaces owned by `include/access_model/`.
It is for cache/storage adapter authors and developers integrating the
coordinator. The headers are the API source of truth; this page records important
runtime behavior and edge cases as implemented in `src/access_model/`.

## Tier contract

`TierLevel` identifies `L1_WORKING`, `L2_EPISODIC`, `L3_SEMANTIC`,
`STORAGE_HOT`, `STORAGE_WARM`, `STORAGE_COLD`, and `UNKNOWN`.
`AccessTier` is the common interface. Implementations provide string-keyed
`get`, `put`, and `invalidate`, tier identity/capacity/access statistics, and
`initialize`, `shutdown`, and `isHealthy`. `CacheTier` adds `notifyEviction`;
`StorageTier` adds `notifyHotAccess`. Promotion/demotion capability and
estimated latency are optional virtual behaviors; the base implementation says
unsupported and returns `-1 ms` for an unknown estimate.

`TierAccessOptions` carries correlation ID, optional target tier, maximum wait,
an opaque caller context pointer, and a completion callback. `TierGetResult`,
`TierPutResult`, and `TierPromotionResult` carry status and tier/latency fields.
The tier interface does not define persistence, encoding, or vector semantics.

## Coordinator contract

`createAccessCoordinator(std::size_t thread_pool_size = 4)` returns a shared
coordinator. Its lifecycle methods are `initialize(tiers)`, `start()`,
`shutdown()`, and `isRunning()`. `setAgePolicy()` assigns the policy and
activates event-handler decisions. `setPromotionThresholds()` only changes the
two count thresholds; by itself it does not mark a policy as configured.

`EvictionListener::onCacheEvicted` carries key, source tier, size, access count,
age since access, and reason. `PromotionListener::onStorageAccess` carries key,
source tier, access count, and access window. `AccessCoordinator` adapts these
callbacks to `EvictionEvent` and `AccessEvent`.

Other operations are `promoteAsync`, `planDemotion`, `executeDemotion`,
`getKeyMetrics`, `getTierMetrics`, `getAccessModelMetrics`, and
`getRecentTransitions`. A demotion plan is identified by its `plan_id`; an
unknown plan or an unregistered source/destination tier returns `std::nullopt`.

## Important edge behavior

- The current `initialize()` implementation copies the map and returns true; it
  does not validate its contents or initialize tier objects.
- `setAgePolicy()` does not reject a policy for which `AgeBasedPolicy::isValid()`
  is false. Callers that need that check must perform it themselves.
- `promoteAsync()` returns a failed result immediately if the coordinator is not
  running. When running, current worker code does not invoke tier operations and
  stores only a weak pointer to the completion promise; future completion is
  therefore not a reliable data-transfer signal.
- Current demotion execution checks plan/tier registration but performs no
  tier-to-tier data movement.
- `getKeyMetrics()` and `getTierMetrics()` return empty/default metrics.
- `AccessModelMetrics` declares latency histograms, but coordinator event/task
  processing does not currently record samples into them.
- `getRecentTransitions()` is insertion-ordered when the full history fits
  within the requested limit and newest-first when truncation is needed.
- `UNKNOWN` is a sentinel; `classifyTier(UNKNOWN)` currently returns
  `TierClassification::STORAGE`.

Consumers must treat success counters/results as coordinator bookkeeping, not
proof that a value was moved. See [`IMPLEMENTATION.md`](./IMPLEMENTATION.md)
before relying on asynchronous operations.

## Compatibility and validation

The public declarations are in `access_coordinator.h`,
`access_tier_interface.h`, `age_based_policy.h`, `promotion_demotion.h`, and
`access_metrics.h`; logging and correlation context are declared in
`access_model_logging.h` and `access_model_trace.h`. When changing a signature,
preserve the header's documented v1.x compatibility boundary and update focused
tests plus this contract. See [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for
integration ordering.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
