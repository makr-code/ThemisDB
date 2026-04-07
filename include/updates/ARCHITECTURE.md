<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/updates/ -->

# Updates Module — Public Header Architecture

## Overview

The `include/updates/` headers expose ThemisDB's zero-downtime update pipeline.
The subsystem handles blue/green deployment, canary rollouts, hot reload, schema
migration with automated testing, coordinated multi-node sequencing, preflight
health checks, tenant-aware scheduling, delta patching, and full update lifecycle
state management.  Implementation details live in `../../src/updates/`.

## Design Principles

- **Zero-downtime by default** — every deployment strategy (blue/green, canary,
  hot-reload) is designed so at least one instance always serves traffic.
- **Preflight gates** — `PrefightHealthCheck` must pass before any update proceeds;
  failures abort the pipeline, not the cluster.
- **Schema safety** — `SchemaMigrationTester` validates migrations against a shadow
  copy before they touch production data.
- **Tenant isolation** — `TenantUpdateScheduler` ensures updates respect per-tenant
  maintenance windows and SLAs.
- **Observability** — `UpdateHistoryLogger` and `NotificationWebhook` provide
  end-to-end audit trails and external alerting without coupling to business logic.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `blue_green_deployment.h` | `BlueGreenDeployment` | Atomic blue→green traffic switch |
| `canary_rollout.h` | `CanaryRollout`, `CanaryConfig` | Percentage-gated canary traffic routing |
| `cluster_update_manager.h` | `ClusterUpdateManager` | Cluster-wide update orchestration |
| `coordinated_update_manager.h` | `CoordinatedUpdateManager` | Multi-node sequenced update coordination |
| `delta_update_engine.h` | `DeltaUpdateEngine` | Binary delta patch application |
| `dependency_resolver.h` | `DependencyResolver` | Update dependency graph resolution |
| `hot_reload_engine.h` | `HotReloadEngine` | Live code/config reload without restart |
| `in_place_schema_migrator.h` | `InPlaceSchemaMigrator` | In-place DDL schema evolution |
| `manifest_database.h` | `ManifestDatabase` | Persistent update manifest store |
| `notification_webhook.h` | `NotificationWebhook` | Outbound webhook notifications |
| `parallel_downloader.h` | `ParallelDownloader` | Concurrent artifact download |
| `preflight_health_check.h` | `PreflightHealthCheck` | Pre-update health gate |
| `release_manifest.h` | `ReleaseManifest` | Immutable release descriptor |
| `schema_migration.h` | `SchemaMigration`, `MigrationRunner` | Schema migration definitions and runner |
| `schema_migration_tester.h` | `SchemaMigrationTester` | Shadow-copy migration validation |
| `tenant_update_scheduler.h` | `TenantUpdateScheduler` | Per-tenant maintenance-window scheduling |
| `update_history_logger.h` | `UpdateHistoryLogger` | Append-only update audit log |
| `update_state_machine.h` | `UpdateStateMachine` | FSM governing update lifecycle states |
| `updates_config.h` | `UpdatesConfig` | Centralised updates configuration bag |

## Notes

- `updates_config.h` and `release_manifest.h` are value-type headers safe to
  include in any layer without pulling in heavy dependencies.
- `update_state_machine.h` defines the canonical state graph; all other managers
  must honour state transitions it exposes.

---
*Implementation in `../../src/updates/`*
