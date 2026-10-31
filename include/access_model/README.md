---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Module — Public Headers

`include/access_model/` defines the public C++ contracts for coordinating
transitions between cache and storage tiers. The host application owns adapter
construction, initialization, lifetime, and listener registration. The module
does not parse runtime configuration or provide a feature-flag interface.

## Canonical module documentation

Detailed behavior, architecture, and operational limits are documented under
[`../../src/access_model/`](../../src/access_model/README.md):

- [Module overview and build/test quickstart](../../src/access_model/README.md)
- [Architecture and data flow](../../src/access_model/ARCHITECTURE.md)
- [API contract and edge cases](../../src/access_model/API.md)
- [Implementation limits](../../src/access_model/IMPLEMENTATION.md)

## Public headers

| Header | Public contract |
|---|---|
| `access_tier_interface.h` | `AccessTier`, `CacheTier`, `StorageTier`, tier identity, and adapter results |
| `access_coordinator.h` | Listener/event types, `AccessCoordinator`, and coordinator factories |
| `promotion_demotion.h` | `DemotionPlan` and `DemotionResult` |
| `age_based_policy.h` | `AgeBasedPolicy` thresholds and policy predicates |
| `access_metrics.h` | `AccessMetrics`, `AccessModelMetrics`, counters, and latency histograms |
| `access_model_logging.h` | Structured diagnostic record types and logger interface |
| `access_model_trace.h` | Thread-local `TraceContext` and scoped context management |

## Usage: Minimal integration example

The tier adapters in this example are application-owned implementations of
`AccessTier`; they must be initialized and kept alive by the host. A valid
registry must include at least one cache tier and one storage tier.

```cpp
#include "access_model/access_coordinator.h"

#include <map>
#include <memory>

namespace access_model = themis::access_model;

auto coordinator = access_model::createAccessCoordinator(4, 10'000);
std::map<access_model::TierLevel, std::shared_ptr<access_model::AccessTier>> tiers = {
    {access_model::TierLevel::L1_WORKING, cache_l1},
    {access_model::TierLevel::STORAGE_COLD, storage_cold},
};

if (coordinator->initialize(tiers)) {
    coordinator->setAgePolicy(access_model::AgeBasedPolicy{});
    coordinator->start();
}
```

The cache and storage implementations expose listener setters separately. The
coordinator does not implement `EvictionListener` or `PromotionListener`;
provide host-owned bridge objects that forward their callbacks to the
coordinator's public event methods. Register the bridges only after successful
initialization, and unregister them before destroying either the bridges or
coordinator. See the [integration sequence](../../src/access_model/MODULE_GUIDE.md).

## Configuration and build

- `createAccessCoordinator(worker_count, max_queue_depth)` configures the worker
  count and pending-task limit. The defaults are four workers and a queue depth
  of 10,000; zero values are rejected with `std::invalid_argument`.
- `AgeBasedPolicy` exposes its thresholds as direct fields; there is no nested
  `AgeBasedPolicy::Config` type.
- `AccessModelMetrics` exposes counters and latency histograms, including
  current and configured pending-task queue depth. The module does not export
  Prometheus metrics or expose worker-utilization telemetry.

Configure a repository build with tests enabled, then build and run a focused
module test:

```bash
cmake -S . -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target module_access_model_test_access_coordinator_focused
ctest --test-dir build -R 'module_access_model_test_access_coordinator_focused' --output-on-failure
```

The standalone static library target defined by
`src/access_model/CMakeLists.txt` is named `themis_access_model` when that target
is used by the build.

## Installation

The module does not define a separate install/export rule. Use the root
ThemisDB build and packaging flow to distribute these headers and link the
appropriate project targets.

## Related module docs

- [Cache tier integration](../../src/cache/README.md)
- [Storage tier integration](../../src/storage/README.md)

**AI provenance:** Exact model backend not verifiable from final document text alone.
