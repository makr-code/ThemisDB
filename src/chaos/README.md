# chaos module

Status: production-ready fault injection and deterministic chaos scheduling.

## Implemented Components
- `FaultInjector` in `src/chaos/chaos_framework.cpp`
- `ChaosScheduler` in `src/chaos/chaos_framework.cpp`
- Public API in `include/chaos/chaos_framework.h`

## Runtime Behavior
- Supports per-node and per-fault-type injection.
- Supports finite-duration and permanent faults.
- Prunes expired faults lazily on read paths.
- Scheduler executes pending injections on a background thread.

## Current Scope
- In-process fault registry for tests and integration scenarios.
- No direct OS/network fault manipulation.