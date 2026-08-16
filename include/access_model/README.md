> **Build:** `cmake --preset release && cmake --build build/release`

# Access Model Module — Public Headers

**Module Path:** `include/access_model/`
**Implementation:** `../../src/access_model/`

## Purpose

Public interfaces and declarations for ThemisDB's unified access tier coordination subsystem, providing abstract tier interfaces and promotion/demotion orchestration.

## Canonical Module Documentation

`include/access_model/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/access_model/`:

- [`../../src/access_model/README.md`](../../src/access_model/README.md)
- [`../../src/access_model/ARCHITECTURE.md`](../../src/access_model/ARCHITECTURE.md)
- [`../../src/access_model/ROADMAP.md`](../../src/access_model/ROADMAP.md)
- [`../../src/access_model/FUTURE_ENHANCEMENTS.md`](../../src/access_model/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `access_tier_interface.h` | `AccessTier` — abstract tier interface |
| `access_coordinator.h` | `AccessCoordinator` — broker for promotion/demotion |
| `promotion_demotion.h` | `PromotionRequest`, `DemotionRequest` — tier transition data structures |
| `age_based_policy.h` | `AgeBasedPolicy` — unified aging policy configuration |
| `access_metrics.h` | `AccessMetrics` — observability metrics and telemetry |

## Usage

```cpp
#include "access_model/access_coordinator.h"

auto coordinator = themis::access_model::createAccessCoordinator(4);  // 4 workers

std::map<TierLevel, std::shared_ptr<AccessTier>> tiers = {
    {TierLevel::L1_WORKING, cache_l1},
    {TierLevel::L2_EPISODIC, cache_l2},
    {TierLevel::STORAGE_COLD, storage_cold},
};

coordinator->initialize(tiers);
```

For full runtime usage examples (promotion/demotion, metrics collection), see [`../../src/access_model/README.md`](../../src/access_model/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `access_coordinator.h` (worker pool size, enable/disable)
- `age_based_policy.h` (`AgeBasedPolicy::Config` for transition thresholds)
- `access_metrics.h` (metrics collection and query interfaces)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-access_model
```

## See Also

- [`../../src/access_model/README.md`](../../src/access_model/README.md) — implementation details
- [`../../src/cache/README.md`](../../src/cache/README.md) — cache tier integration
- [`../../src/storage/README.md`](../../src/storage/README.md) — storage tier integration

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
