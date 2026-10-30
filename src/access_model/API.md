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
coordinator with a 10,000-task queue limit. The overload
`createAccessCoordinator(thread_pool_size, max_queue_depth)` configures that
limit; both values must be non-zero. `initialize(tiers)` rejects empty or
cache-only/storage-only registries, null/unhealthy adapters, `UNKNOWN` entries,
and mismatched tier identities. It validates but does not initialize adapters.
Call `start()` only after successful initialization. `shutdown()` rejects new
queued work, drains accepted work, and joins workers; a blocked adapter operation
can therefore delay shutdown.

`setAgePolicy()` throws `std::invalid_argument` and leaves the current policy
unchanged when `AgeBasedPolicy::isValid()` fails. `setPromotionThresholds()`
also throws if the resulting policy is invalid; by itself it does not activate
event-handler decisions.

`EvictionListener::onCacheEvicted` carries key, source tier, size, access count,
age since access, and reason. `PromotionListener::onStorageAccess` carries key,
source tier, access count, and access window. `AccessCoordinator` adapts these
callbacks to `EvictionEvent` and `AccessEvent`.

Other operations are `promoteAsync`, `planDemotion`, `executeDemotion`,
`getKeyMetrics`, `getTierMetrics`, `getAccessModelMetrics`, and
`getRecentTransitions`. A demotion plan is identified by its `plan_id`; an
unknown/consumed plan returns `std::nullopt`. A plan with a known identifier
returns a `DemotionResult`, including a failed result if a tier operation fails.

## Important edge behavior

- `promoteAsync()` copies the fetched value with source `get()` and destination
  `put()`; it retains the source. Its future reports queue-full, missing-key,
  unhealthy-tier, adapter, and placement errors.
- Demotion writes to the destination before invalidating the source. If
  invalidation fails, the result reports failure and both copies may remain.
  The operation is not an atomic transaction across tier backends.
- Cache eviction/hot-access callbacks are `void`. If a qualifying event cannot
  be queued (stopped coordinator or full queue), the coordinator records a failed
  transition/counter; inspect metrics rather than assuming callback acceptance.
- Per-key metrics retain up to 10,000 recently observed keys; an evicted key
  returns default metrics. Per-tier metrics include observed access/eviction and
  transition counts.
- `AccessModelMetrics` records event, policy, promotion, and demotion latency
  samples. Its counters also report current and configured pending queue depth.
- Transition history retains at most 1,000 records and
  `getRecentTransitions(limit)` always returns newest-first.
- `UNKNOWN` is a sentinel; `classifyTier(UNKNOWN)` currently returns
  `TierClassification::STORAGE`.

The tier operations are sequential and do not provide backend rollback,
cross-tier atomicity, retry, or deduplication. Applications must verify adapter
semantics and reconcile the reported partial-demotion case.

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
