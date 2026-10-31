---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# include chaos module

Public header surface for in-process chaos/fault injection.

## Header Entry-Point

- `include/chaos/chaos_framework.h`

## Public API

### Types

- `FaultType`
- `FaultSpec`
- `ActiveFault`
- `ChaosScheduleEntry`
- `WakeStrategy`
- `ChaosSchedulerConfig`

### Classes

- `FaultInjector`
  - `injectFault`, `recoverFault`, `isFaultActive`
  - `getActiveFaults`, `activeFaultCount`, `clearAllFaults`
  - `registerEventCallback`
- `ChaosScheduler`
  - `schedule`, `scheduleIn`
  - `start`, `stop`, `isRunning`
  - `pendingCount`, `clearPending`

## Installation

Headers are included with ThemisDB. For example, ensure your target can include
the project include directory:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

```cpp
#include "chaos/chaos_framework.h"

themis::chaos::FaultInjector injector;
injector.injectFault({themis::chaos::FaultType::NODE_FAILURE, "node-1"});
```

## Configuration Notes

- `FaultSpec::duration = 0ms` creates a permanent fault until recovery.
- `FaultSpec::probability` is intended to be within `[0.0, 1.0]`. The runtime
  rejects ordinary out-of-range values but its current comparisons do not
  reject NaN; callers should supply a finite value.
- `ChaosSchedulerConfig::tick_interval` controls wake/check cadence.
- `ChaosSchedulerConfig::wake_strategy` controls polling vs condvar wakeups.

## Limits

- Header APIs model logical fault state only; they do not perform real infrastructure sabotage.
- Scheduler state and fault registry are process-local and non-persistent.

## Related Docs

- Module implementation overview: [`../../src/chaos/README.md`](../../src/chaos/README.md)
- Architecture: [`../../src/chaos/ARCHITECTURE.md`](../../src/chaos/ARCHITECTURE.md)
- Security: [`../../src/chaos/SECURITY.md`](../../src/chaos/SECURITY.md)
- Audit: [`../../src/chaos/AUDIT.md`](../../src/chaos/AUDIT.md)
- Roadmap: [`../../src/chaos/ROADMAP.md`](../../src/chaos/ROADMAP.md)
- Future enhancements: [`../../src/chaos/FUTURE_ENHANCEMENTS.md`](../../src/chaos/FUTURE_ENHANCEMENTS.md)
- Performance expectations: [`../../src/chaos/PERFORMANCE_EXPECTATIONS.md`](../../src/chaos/PERFORMANCE_EXPECTATIONS.md)
