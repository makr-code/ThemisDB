<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/updates/ARCHITECTURE.md -->

# Updates Module — Public Header Architecture

**Module Path:** `include/updates/`  
**Implementation:** `../../src/updates/`  
**Canonical architecture doc:** [`../../src/updates/ARCHITECTURE.md`](../../src/updates/ARCHITECTURE.md)

---

## 1. Overview

`include/updates/` defines the **public hot-reload, blue-green and canary deployments, cluster update coordination, schema migration, manifest management, hardware telemetry, and preflight checks API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/updates/ARCHITECTURE.md`](../../src/updates/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Deployment Strategies

| Header | Public Type | Purpose |
|--------|------------|---------|
| `blue_green_deployment.h` | `BlueGreenDeployment` | Blue-green deployment orchestration |
| `canary_rollout.h` | `CanaryRollout` | Local staged canary rollout |
| `hot_reload_engine.h` | `HotReloadEngine` | Download, verify, apply, and rollback operations |
| `cluster_update_manager.h` | `ClusterUpdateManager` | Cluster-wide coordinated update manager |
| `coordinated_update_manager.h` | `CoordinatedUpdateManager` | Multi-shard coordinated update sequencing |
### 2.2 Schema Migration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `schema_migration.h` | `SchemaMigration`, `MigrationContext`, `MigrationResult` | Schema migration plan and execution contracts |
| `schema_migration_tester.h` | `SchemaMigrationTester` | Automated migration test harness |
| `in_place_schema_migrator.h` | `InPlaceSchemaMigrator` | Online in-place schema migration |
| `delta_update_engine.h` | `DeltaUpdateEngine` | Incremental delta-based data updates |
### 2.3 Manifest and Packages

| Header | Public Type | Purpose |
|--------|------------|---------|
| `manifest_database.h` | `ManifestDatabase` | Release-manifest persistence and verification surfaces |
| `release_manifest.h` | `ReleaseManifest` | Release manifest format and parser |
| `dependency_resolver.h` | `DependencyResolver` | Update dependency resolution |
| `parallel_downloader.h` | `ParallelDownloader` | Parallel package download manager |
| `update_state_machine.h` | `UpdateStateMachine` | Update lifecycle state machine |
| `updates_config.h` | `UpdatesConfig` | Update system configuration |
| `tenant_update_scheduler.h` | `TenantUpdateScheduler` | Per-tenant update scheduling |
### 2.4 Telemetry and Verification

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hardware_telemetry.h` | `HardwareTelemetryReporter` | Hardware and performance telemetry |
| `build_verifier.h` | `BuildVerificationResult`, `verifyBuildSignature()`, `buildChannel()` | Compile-time build-signature verification API |
| `preflight_health_check.h` | `PreflightHealthChecker` | Pre-update health checks |
| `notification_webhook.h` | `NotificationWebhook` | Update lifecycle notification webhooks |
| `update_history_logger.h` | `UpdateHistoryLogger` | Update event history persistence |

---

## 3. Namespace Layout

Public interfaces in these headers use the `themis::updates` namespace.

---

## 4. Contract Notes

- Headers in `include/updates/` expose public interfaces; compatibility policy is governed by the project versioning policy, not by this index.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Canary membership is local/deterministic; cluster coordination is provided by separate manager interfaces.
