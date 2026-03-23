<!-- Status: current | validated: 2026-03-22 -->

# Updates Module — Public Header Changelog

> Full implementation changelog: [`../../src/updates/CHANGELOG.md`](../../src/updates/CHANGELOG.md)
> Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

---

## [1.5.0] — 2026-03-12

### Added
- `tenant_update_scheduler.h` — `TenantUpdateScheduler` with per-tenant maintenance-window API.
- `schema_migration_tester.h` — `SchemaMigrationTester` shadow-copy validation framework.

### Changed
- `canary_rollout.h` — `CanaryConfig` now enforces percentage range [0, 100] at construction.

---

## [1.4.0] — 2025-11-20

### Added
- `coordinated_update_manager.h` — `CoordinatedUpdateManager` for multi-node sequenced updates.
- `preflight_health_check.h` — blocking `PreflightHealthCheck` gate with `HealthCheckResult` enum.

---

## [1.3.0] — 2025-08-10

### Added
- `update_state_machine.h` — `UpdateStateMachine` FSM with enforced transition guards.
- `update_history_logger.h` — append-only `UpdateHistoryLogger`.
- `notification_webhook.h` — `NotificationWebhook` outbound alerting.

---

## [1.2.0] — 2025-05-01

### Added
- `hot_reload_engine.h` — `HotReloadEngine` for live reload without process restart.
- `delta_update_engine.h` — `DeltaUpdateEngine` for binary delta patch application.
- `parallel_downloader.h` — `ParallelDownloader` for concurrent artifact fetch.

---

## [1.1.0] — 2025-02-14

### Added
- `canary_rollout.h` — `CanaryRollout` and `CanaryConfig`.
- `dependency_resolver.h` — `DependencyResolver` update dependency graph.
- `manifest_database.h` — `ManifestDatabase` persistent manifest store.
- `release_manifest.h` — immutable `ReleaseManifest` value type.

---

## [1.0.0] — 2024-09-01

### Added
- Initial public headers: `blue_green_deployment.h`, `cluster_update_manager.h`,
  `in_place_schema_migrator.h`, `schema_migration.h`, `updates_config.h`.
