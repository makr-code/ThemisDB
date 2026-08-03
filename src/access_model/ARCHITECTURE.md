# Architecture - Access Model Module

**Version:** 1.0.0  
**Status:** 🟡 ALPHA (Phase 1 API Definition)  
**Validated:** 2026-08-03  
**Links:** [`README.md`](./README.md) · [`ROADMAP.md`](./ROADMAP.md) · [`../../docs/architecture/UNIFIED_ACCESS_MODEL.md`](../../docs/architecture/UNIFIED_ACCESS_MODEL.md)

---

## Overview

The `access_model` module bridges cache and storage tiers through the `AccessCoordinator` broker. It manages tier transitions (promotion/demotion) with clean separation of concerns.

---

## Main Execution Planes

### 1. **Tier Abstraction Plane**
- `AccessTier` interface — abstract contract for all tiers
- `CacheTier` specialization — cache-specific behavior
- `StorageTier` specialization — storage-specific behavior
- Enables coordinator to treat cache and storage uniformly

### 2. **Coordination Plane**
- `AccessCoordinator` — central broker for all transitions
- Eviction signal processing (cache → storage feedback)
- Promotion pattern detection (storage → cache feedback)
- Background worker pool for async operations

### 3. **Policy Plane**
- `AgeBasedPolicy` — unified aging thresholds
- Cache: L1→L2→L3 demotion based on age
- Storage: hot→warm→cold migration based on age
- Single source of truth for "hotness" definition

### 4. **Observability Plane**
- `AccessMetrics` — comprehensive statistics
- Correlation IDs for end-to-end tracing
- Latency histograms, hit rates, cross-tier analytics
- Structured logging for all transitions

---

## Core Contracts

| Contract | Behavior |
|----------|----------|
| **AccessTier Interface** | Unified get/put/invalidate/promote/demote API |
| **Eviction Listener** | Cache notifies coordinator on LRU/LFU/TTL eviction |
| **Promotion Listener** | Storage notifies coordinator on hot access patterns |
| **AgeBasedPolicy** | Unified migration thresholds for all tiers |
| **AccessMetrics** | Uniform observability surface |

---

## Key Design Decisions

### 1. Why Abstract `AccessTier` Interface?

**Decision:** Both cache and storage tiers implement a common interface.

**Benefits:**
- Coordinator has no direct cache↔storage dependencies
- Tier substitution possible (mock for testing)
- Uniform metrics collection across tiers
- Easy to add new tier types (GPU memory, etc.)

### 2. Why Separate Coordinator?

**Decision:** Promotion/demotion orchestration happens in dedicated broker.

**Benefits:**
- Single responsibility: Cache manages memory, Storage manages persistence, Coordinator manages transitions
- Reduced coupling between cache and storage modules
- Easier to test: mock coordinator for cache/storage unit tests
- Flexible policies: coordinator applies policies uniformly

### 3. Why Unified `AgeBasedPolicy`?

**Decision:** Cache and storage tiers use same aging thresholds.

**Benefits:**
- No conflicting "hotness" definitions across layers
- Operators have single configuration source
- Consistent demotion behavior (predictable)
- Simpler to reason about data lifecycle

### 4. Why Correlation IDs?

**Decision:** Every transition gets a unique correlation ID.

**Benefits:**
- End-to-end tracing across cache→coordinator→storage
- Production debugging: link operator logs to specific transitions
- Performance analysis: latency histograms per correlation family

---

## Backward Compatibility

### For Cache Module
- Existing `AdaptiveQueryCache`, `BoundedLRUCache` APIs **unchanged**
- `AccessCoordinator` is **opt-in** (feature flag)
- Default: cache operates independently (no coordinator)

### For Storage Module
- Existing `TieredStorageManager` APIs **unchanged**
- Migration policies (hot→warm→cold) **continue independently**
- `AccessCoordinator` callbacks are **optional listeners**

### Migration Path
1. Enable coordinator at startup (feature flag: `THEMISDB_ACCESS_COORDINATOR_ENABLED`)
2. Register cache/storage tier implementations
3. Set unified `AgeBasedPolicy`
4. Observe metrics; adjust thresholds if needed

---

## Sourcecode Verification (Phase 1 API Definition)

**Files:**
- `include/access_model/access_tier_interface.h` — Abstract tier contract
- `include/access_model/access_coordinator.h` — Coordinator interface
- `include/access_model/promotion_demotion.h` — Data structures
- `include/access_model/age_based_policy.h` — Unified policy
- `include/access_model/access_metrics.h` — Observability

**Verified Claims:**
- ✅ Unified tier abstraction with explicit contracts
- ✅ Coordinator broker with clear responsibilities
- ✅ Backward-compatible opt-in design
- ✅ Observable transitions with correlation IDs

**Pending (Phase 2+):**
- Implementation: `access_coordinator.cpp`
- Integration hooks: cache → coordinator → storage
- Tests: ACM-01..ACM-12
- Release gates: GATE-ACM-01..06

---

## See Also

- [`docs/architecture/UNIFIED_ACCESS_MODEL.md`](../../docs/architecture/UNIFIED_ACCESS_MODEL.md) — Unified model overview
- [`docs/architecture/CACHE_STORAGE_INTEGRATION.md`](../../docs/architecture/CACHE_STORAGE_INTEGRATION.md) — Integration patterns
- [`src/cache/ARCHITECTURE.md`](../cache/ARCHITECTURE.md) — Cache module
- [`src/storage/ARCHITECTURE.md`](../storage/ARCHITECTURE.md) — Storage module

