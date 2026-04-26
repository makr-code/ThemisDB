> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# failover module

Status: production-ready failover orchestration and disaster recovery workflows.

## Implemented Components
- `AutoFailoverManager` in `src/failover/auto_failover_manager.cpp`
- `DisasterRecoveryManager` in `src/failover/disaster_recovery_manager.cpp`
- Public APIs in `include/failover/auto_failover_manager.h` and `include/failover/disaster_recovery_manager.h`

## Runtime Behavior
- Continuous health checks and queued failover execution.
- Quorum and split-brain prevention hooks.
- Disaster-recovery plan execution pipeline with step-level outcomes.

## Current Scope
- Orchestration logic implemented with replication and fencing integrations.
- Recovery step hooks available for test and custom execution paths.

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/failover/README.md`](../../include/failover/README.md) for the public API.
