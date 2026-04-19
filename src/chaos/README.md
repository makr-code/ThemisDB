> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

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

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/chaos/README.md`](../../include/chaos/README.md) for the public API.
