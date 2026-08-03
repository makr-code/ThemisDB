# Access Model Module

**Version:** 1.0.0  
**Status:** 🟡 ALPHA (Phase 1 API Definition)  
**Validated:** 2026-08-03

---

## Overview

The `access_model` module provides unified access tier abstraction and coordination between cache and storage layers in ThemisDB.

### Key Components

| Component | File | Purpose |
|-----------|------|---------|
| **AccessTier** | `access_tier_interface.h` | Abstract tier interface (base contract) |
| **AccessCoordinator** | `access_coordinator.h` | Broker for promotion/demotion |
| **Promotion/Demotion** | `promotion_demotion.h` | Data structures for tier transitions |
| **AgeBasedPolicy** | `age_based_policy.h` | Unified aging policy |
| **AccessMetrics** | `access_metrics.h` | Observability metrics |

---

## Design Principles

1. **Single Responsibility:** Cache manages memory; Storage manages persistence; Coordinator manages transitions
2. **Dependency Inversion:** Both layers use abstract `AccessTier` interface
3. **Backward Compatible:** Opt-in via feature flag; existing APIs unchanged
4. **Observable:** All transitions logged with correlation IDs
5. **Testable:** Mock coordinator for unit tests; deterministic integration tests

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│  AccessCoordinator (Broker)                     │
│  • Listens to cache eviction signals            │
│  • Listens to storage access patterns           │
│  • Orchestrates promotion/demotion workers      │
│  • Applies unified age-based policies           │
│  • Tracks metrics & correlation IDs             │
└────────┬──────────────────────┬─────────────────┘
         │                      │
    ┌────▼──────┐          ┌────▼─────────┐
    │Cache Tiers│          │StorageTiers  │
    │L1/L2/L3   │          │Hot/Warm/Cold │
    │Implement  │          │Implement     │
    │AccessTier │          │AccessTier    │
    └───────────┘          └──────────────┘
```

---

## Usage

### 1. Initialize the Coordinator

```cpp
#include "access_model/access_coordinator.h"

auto coordinator = themis::access_model::createAccessCoordinator(4);  // 4 workers

std::map<TierLevel, std::shared_ptr<AccessTier>> tiers = {
    {TierLevel::L1_WORKING, cache_l1},
    {TierLevel::L2_EPISODIC, cache_l2},
    {TierLevel::L3_SEMANTIC, cache_l3},
    {TierLevel::STORAGE_HOT, storage_hot},
    {TierLevel::STORAGE_WARM, storage_warm},
    {TierLevel::STORAGE_COLD, storage_cold},
};

coordinator->initialize(tiers);
```

### 2. Set Unified Policy

```cpp
#include "access_model/age_based_policy.h"

AgeBasedPolicy policy;
policy.hot_to_warm_days = 30;
policy.warm_to_cold_days = 90;
policy.l1_promotion_threshold = 10;

coordinator->setAgePolicy(policy);
```

### 3. Register Listeners

```cpp
// Cache tier notifies on eviction
cache_l1->registerEvictionListener(coordinator.get());

// Storage tier notifies on hot access
storage_hot->registerPromotionListener(coordinator.get());
```

### 4. Initiate Promotion (Optional)

```cpp
auto result = coordinator->promoteAsync("key_abc", 
                                       TierLevel::STORAGE_COLD,
                                       TierLevel::L3_SEMANTIC,
                                       {.max_wait_ms = 100ms});
```

---

## Observability

### Metrics

```cpp
// Get key-specific metrics
auto key_metrics = coordinator->getKeyMetrics("key_abc");
// → current_tier: L2_EPISODIC
// → total_accesses: 42
// → promotion_path: [COLD, WARM, L3, L2]

// Get tier-specific metrics
auto tier_metrics = coordinator->getTierMetrics(TierLevel::L1_WORKING);
// → hit_rate: 0.92
// → avg_get_latency: 500ns
// → current_size_bytes: 50MB

// Get coordinator-level metrics
auto model_metrics = coordinator->getAccessModelMetrics();
// → promotions_succeeded: 1520
// → coordinator_overhead_percent: 2.3
```

### Structured Logging

All transitions logged with correlation ID:

```json
{
  "timestamp": "2026-08-03T09:18:54Z",
  "correlation_id": "acm-promo-a1b2c3d4",
  "event": "promotion_complete",
  "key": "query_result_x",
  "from_tier": "STORAGE_COLD",
  "to_tier": "L3_SEMANTIC",
  "latency_ms": 45,
  "reason": "storage_hot_access"
}
```

---

## Configuration

### CMake Feature Flag

```cmake
option(THEMISDB_ACCESS_COORDINATOR_ENABLED 
       "Enable unified access model coordinator" ON)
```

### Runtime Config

```ini
[access_model]
enabled = true
coordinator_thread_pool_size = 4

[age_based_policy]
hot_to_warm_days = 30
warm_to_cold_days = 90
l1_promotion_threshold = 10
```

---

## Testing

### Unit Tests

```bash
ctest -R "module_access_model_.*_focused" -V
```

Labels: `access_model`, `tier`, `promotion`, `demotion`

### Integration Tests

```bash
ctest -R "test_access_model_e2e" -V
```

---

## Dependencies

- **cache module** — For cache tier implementations
- **storage module** — For storage tier implementations
- **core/logger** — For structured logging
- **core/metrics** — For telemetry collection

---

## See Also

- [`docs/architecture/UNIFIED_ACCESS_MODEL.md`](../../docs/architecture/UNIFIED_ACCESS_MODEL.md) — Architectural overview
- [`docs/architecture/CACHE_STORAGE_INTEGRATION.md`](../../docs/architecture/CACHE_STORAGE_INTEGRATION.md) — Integration patterns
- [`src/cache/ROADMAP.md`](../cache/ROADMAP.md) — Cache module roadmap
- [`src/storage/ROADMAP.md`](../storage/ROADMAP.md) — Storage module roadmap

---

## Roadmap

- **Phase 1:** Architecture & interfaces (DONE)
- **Phase 2:** Core coordinator implementation (IN PROGRESS)
- **Phase 3:** Cache module integration (PLANNED)
- **Phase 4:** Storage module integration (PLANNED)
- **Phase 5:** Observability & diagnostics (PLANNED)
- **Phase 6:** Tests & release gates (PLANNED)

---

## Maintainers

- Copilot (Initial implementation)
- Community (Contributions welcome)

