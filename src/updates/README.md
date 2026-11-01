---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Updates Module

## Module Purpose

The updates module provides C++ interfaces and implementations for release
metadata, state tracking, file and delta application, migration, rollout,
configuration, and diagnostics. This overview describes in-tree surfaces; it
does not assert release readiness or a deployment-wide consistency guarantee.

## Relevant Interfaces

| Implementation / public header | Role |
|---|---|
| `update_state_machine.cpp` / `update_state_machine.h` | state and checkpoint operations |
| `release_manifest.cpp` / `release_manifest.h` | release/file data and JSON conversion |
| `manifest_database.cpp` / `manifest_database.h` | manifest persistence and verification surfaces |
| `delta_update_engine.cpp` / `delta_update_engine.h` | delta patch generation/application |
| `hot_reload_engine.cpp` / `hot_reload_engine.h` | release download, verification, apply, and rollback |
| `coordinated_update_manager.cpp` / `coordinated_update_manager.h` | coordinated node update workflow |
| `cluster_update_manager.cpp` / `cluster_update_manager.h` | cluster update operations |
| `tenant_update_scheduler.cpp` / `tenant_update_scheduler.h` | tenant update scheduling |
| `in_place_schema_migrator.cpp`, `schema_migration.cpp`, `schema_migration_tester.cpp` | schema migration interfaces and implementation |
| `build_verifier.cpp` / `build_verifier.h` | build-signature verification |
| `dependency_resolver.cpp` / `dependency_resolver.h` | update dependency resolution |
| `canary_rollout.cpp`, `blue_green_deployment.cpp` | local canary and blue/green control |
| `notification_webhook.cpp`, `parallel_downloader.cpp`, `preflight_health_check.cpp` | supporting notification, download, and preflight surfaces |
| `update_history_logger.cpp`, `updates_config.cpp`, `hardware_telemetry.cpp` | history, configuration, and telemetry |
| `updates_diagnostics.cpp`, `updates_operator_diagnostics.cpp`, `updates_edge_case_handler.cpp` | diagnostics and edge-case classification |

## Scope

In scope:
- update orchestration, manifest, rollback, and patch pipeline behavior
- schema/update lifecycle and rollout control behavior
- health-check, notification, telemetry, and scheduling behavior

Out of scope:
- unrelated business-domain logic outside update subsystem boundaries
- non-update storage/network implementations owned by other modules

## Developer Documentation

- [Research and evaluation](RESEARCH.md) · [state and rollout model](MODELING.md) · [integration boundaries](EMBEDDING.md)
- [Implementation notes](IMPLEMENTATION.md) · [API contracts](API.md) · [Module guide](MODULE_GUIDE.md)
- [Operator runbook](RUNBOOK.md) · [Operations](OPERATIONS.md) · [Trust boundaries](TRUST.md)
- [Architecture](ARCHITECTURE.md) · [Security](SECURITY.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md)

## Runtime Behavior and Limits

- `UpdateStateMachine` rejects invalid transitions and exposes state/history and
  checkpoint operations; its checkpoints do not back up installed files.
- `DeltaUpdateEngine` validates patch paths and supplied hashes/sizes and is
  documented as not thread-safe. See the public header for exact behavior.
- Canary membership is local and deterministic; it does not coordinate peers.
  Cluster/coordinated managers provide separate APIs.
- Configuration defaults and optional behaviors are defined by
  [`updates_config.h`](../../include/updates/updates_config.h) and
  [`updates_config.cpp`](updates_config.cpp).

## Build and verification references

- Focused test sources and registration:
  [`tests/updates/`](../../tests/updates/) and
  [`tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt).
- Five benchmark sources and conditional registration:
  [`benchmarks/updates/`](../../benchmarks/updates/) and
  [`benchmarks/updates/CMakeLists.txt`](../../benchmarks/updates/CMakeLists.txt).
- The cluster-stress target is separately registered in
  [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt), with a
  conditional CTest smoke entry. Registration does not establish a successful
  run or approved performance gate.
- The benchmark map distinguishes source presence from current run results:
  [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Usage

Start with [`MODULE_GUIDE.md`](MODULE_GUIDE.md) to select a public interface,
then verify the operation's return value and failure behavior in
[`API.md`](API.md) and its header. The module does not define a universal
operator CLI or caller-side authorization policy.

## Installation

This directory is part of the repository source tree and has no separate
module-specific installation procedure. Follow the repository build/packaging
instructions for the consumer target.

## Sourcecode Verification (Module: updates/readme)

- Verified files:
  - src/updates/update_state_machine.cpp
  - src/updates/release_manifest.cpp
  - src/updates/manifest_database.cpp
  - src/updates/delta_update_engine.cpp
  - src/updates/hot_reload_engine.cpp
  - src/updates/coordinated_update_manager.cpp
  - src/updates/cluster_update_manager.cpp
  - src/updates/tenant_update_scheduler.cpp
  - src/updates/in_place_schema_migrator.cpp
  - src/updates/schema_migration.cpp
  - src/updates/schema_migration_tester.cpp
  - src/updates/build_verifier.cpp
  - src/updates/dependency_resolver.cpp
  - src/updates/canary_rollout.cpp
  - src/updates/notification_webhook.cpp
  - src/updates/parallel_downloader.cpp
  - src/updates/preflight_health_check.cpp
  - src/updates/update_history_logger.cpp
  - src/updates/updates_config.cpp
  - src/updates/blue_green_deployment.cpp
  - src/updates/hardware_telemetry.cpp
  - src/updates/updates_diagnostics.cpp
  - src/updates/updates_diagnostic_emitter.cpp
  - src/updates/updates_operator_diagnostics.cpp
  - src/updates/updates_edge_case_handler.cpp
- Verified behavior surfaces:
  - update state/manifest/delta/migration/rollout and scheduling paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md