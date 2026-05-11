> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# chaos module

Status: production-ready in-process fault injection and deterministic chaos scheduling.

## Implemented Components

- `FaultInjector` in `src/chaos/chaos_framework.cpp`
- `ChaosScheduler` in `src/chaos/chaos_framework.cpp`
- Public API in `include/chaos/chaos_framework.h`

## Public API and Configuration

### `FaultInjector`

- `injectFault(const FaultSpec&)`: validates input and registers/replaces the active fault for `target_node_id + FaultType`.
- `recoverFault(node)` / `recoverFault(node, type)`: clears active faults.
- `isFaultActive(...)`, `activeFaultCount()`, `getActiveFaults()`: read-side status APIs.
- `registerEventCallback(cb)`: callback on inject/recover events.

`FaultSpec` configuration fields:

- `type`: `NODE_FAILURE`, `NETWORK_PARTITION`, `LEADER_CRASH`, `DELAYED_RESPONSE`, `DISK_FAILURE`, `RANDOM_FAILURE`, `DISASTER_RECOVERY_DRILL`
- `target_node_id`: mandatory non-empty node id
- `duration`: `0ms` means permanent until manual recovery
- `probability`: required range `[0.0, 1.0]`
- `description`: free-form annotation

### `ChaosScheduler`

- `ChaosSchedulerConfig::tick_interval` (default `10ms`)
- `ChaosSchedulerConfig::wake_strategy`:
  - `FIXED_TICK`: deterministic periodic polling
  - `CONDVAR`: wait/notify with lower stop and scheduling latency
- `schedule(...)`, `scheduleIn(...)`, `start()`, `stop()`, `pendingCount()`, `clearPending()`

## Runtime Behavior

- Faults are scoped to process memory and keyed by `node_id + fault_type`.
- `injectFault` rejects empty node ids and out-of-range probabilities.
- Expiry is lazy: expired entries are removed on `getActiveFaults()` and `activeFaultCount()`.
- Scheduler fires due entries on a background worker thread.
- `stop()` stops the worker and does not fire remaining pending entries.

## Error Cases and Limits

- `ChaosScheduler` construction throws `std::invalid_argument` when injector is null.
- No direct OS, network, or disk sabotage is performed (simulation only).
- Callback functions execute in caller/recovery path and should be non-blocking.
- Default implementation has no persistence or cluster-wide distribution.

## Installation

This module is built as part of ThemisDB:

```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
```

## Usage

```cpp
#include "chaos/chaos_framework.h"
using namespace themis::chaos;

auto injector = std::make_shared<FaultInjector>("demo");
injector->injectFault(FaultSpec{FaultType::NODE_FAILURE, "node-a", std::chrono::seconds(2)});

ChaosScheduler::Config cfg;
cfg.tick_interval = std::chrono::milliseconds(5);
cfg.wake_strategy = WakeStrategy::CONDVAR;

ChaosScheduler scheduler(injector, cfg);
scheduler.start();
scheduler.scheduleIn(std::chrono::milliseconds(50),
                     FaultSpec{FaultType::DISK_FAILURE, "node-b"});
scheduler.stop();
```

## Troubleshooting

- Fault not active: verify non-empty `target_node_id`, valid probability, and that duration has not expired.
- Delayed scheduling: reduce `tick_interval` or use `WakeStrategy::CONDVAR`.
- Slow shutdown: use `WakeStrategy::CONDVAR` to avoid waiting full fixed tick.
- Unexpected hangs: ensure event callbacks do not block or re-enter long critical paths.

## Related Docs

- Public headers: [`../../include/chaos/README.md`](../../include/chaos/README.md)
- Architecture: [`./ARCHITECTURE.md`](./ARCHITECTURE.md)
- Security: [`./SECURITY.md`](./SECURITY.md)
- Audit: [`./AUDIT.md`](./AUDIT.md)
- Roadmap: [`./ROADMAP.md`](./ROADMAP.md)
- Future enhancements: [`./FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Performance expectations: [`./PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md)
- Changelog: [`./CHANGELOG.md`](./CHANGELOG.md)
