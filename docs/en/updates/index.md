# Updates Module

<!-- Status: current | validated: 2026-04-06 | Primary: ../../../src/updates/ | ../../../include/updates/ -->
<!-- Links: ../../../src/updates/README.md · ../../../src/updates/ROADMAP.md · PRIMARY_SOURCES.md -->

**Date:** 20 March 2026  
**Version:** v1.8.0 (Production-ready)  
**Category:** Zero-Downtime Updates / Schema Migration / Deployment

---

## Overview

The Updates module is ThemisDB's **zero-downtime update and migration system**. It enables safe, atomic software updates, schema migrations, and coordinated cluster upgrades without data loss or significant downtime.

**Operative Features (v1.8.0):**
- Atomic hot-reload with `fsync` + `rename(2)` / `MoveFileExW` guarantees
- CMS/PKCS#7 signature validation with X.509 certificate chain
- Binary delta updates (ZSTD_DICT, VCDIFF/RFC 3284) to reduce bandwidth
- Canary rollout with latency/error-rate monitoring and auto-rollback
- Blue/green deployment for simultaneous operation of two versions
- In-place schema migration without data copy (additive changes)
- Multi-node coordinated updates with replication-safe sequencing
- Cluster-wide rolling updates via `ClusterUpdateManager` (leader-last)
- Notification webhooks (Slack, PagerDuty) on update events
- Pre-flight health checks (disk space, memory, dependency versions)
- Parallel file downloads with token-bucket bandwidth throttling
- Dependency resolution engine (topological sort, conflict resolution)
- Multi-tenant update scheduling with maintenance windows and blackout periods
- ManifestDatabase with atomic file cleanup on entry removal

**Status:** 🟢 Production-ready (v1.8.0)

---

## Component Overview

| Component | Source File | Header | Status |
|---|---|---|---|
| HotReloadEngine | `hot_reload_engine.cpp` | `hot_reload_engine.h` | ✅ Production |
| DeltaUpdateEngine | `delta_update_engine.cpp` | `delta_update_engine.h` | ✅ Production |
| CanaryDeployment | `canary_rollout.cpp` | `canary_rollout.h` | ✅ Production |
| BlueGreenDeployment | `blue_green_deployment.cpp` | `blue_green_deployment.h` | ✅ Production |
| CoordinatedUpdateManager | `coordinated_update_manager.cpp` | `coordinated_update_manager.h` | ✅ Production |
| ClusterUpdateManager | `cluster_update_manager.cpp` | `cluster_update_manager.h` | ✅ Production |
| InPlaceSchemaMigrator | `in_place_schema_migrator.cpp` | `in_place_schema_migrator.h` | ✅ Production |
| SchemaMigrationTester | `schema_migration_tester.cpp` | `schema_migration_tester.h` | ✅ Production |
| NotificationWebhook | `notification_webhook.cpp` | `notification_webhook.h` | ✅ Production |
| PreflightHealthChecker | `preflight_health_check.cpp` | `preflight_health_check.h` | ✅ Production |
| ParallelDownloader | `parallel_downloader.cpp` | `parallel_downloader.h` | ✅ Production |
| DependencyResolver | `dependency_resolver.cpp` | `dependency_resolver.h` | ✅ Production |
| TenantUpdateScheduler | `tenant_update_scheduler.cpp` | `tenant_update_scheduler.h` | ✅ Production |
| ManifestDatabase | `manifest_database.cpp` | `manifest_database.h` | ✅ Production |
| UpdateHistoryLogger | `update_history_logger.cpp` | `update_history_logger.h` | ✅ Production |
| UpdateStateMachine | `update_state_machine.cpp` | `update_state_machine.h` | ✅ Production |

---

## CI/CD Workflows

| Workflow | Path | Component |
|---|---|---|
| binary-delta-patches-ci | `.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml` | DeltaUpdateEngine |
| canary-deployments-ci | `.github/workflows/04-release_canary-deployments-ci.yml` | CanaryDeployment |
| parallel-file-downloads-ci | `.github/workflows/02-feature-modules_storage_parallel-file-downloads-ci.yml` | ParallelDownloader |
| dependency-resolution-engine-ci | `.github/workflows/02-feature-modules_dependency-resolution-engine-ci.yml` | DependencyResolver |
| multi-tenant-update-scheduling-ci | `.github/workflows/02-feature-modules_multi-tenant-update-scheduling-ci.yml` | TenantUpdateScheduler |
| manifest-database-file-deletion-ci | `.github/workflows/02-feature-modules_storage_manifest-database-file-deletion-ci.yml` | ManifestDatabase |
| distributed-cluster-updates-ci | `.github/workflows/06-infrastructure_distributed_distributed-cluster-updates-ci.yml` | ClusterUpdateManager |

---

## Test Coverage

| Test File | Test Target | Description |
|---|---|---|
| `test_updates_production.cpp` | UpdatesProductionFocusedTests | Full integration lifecycle |
| `test_canary_rollout.cpp` | CanaryRolloutFocusedTests | 84 tests incl. CanaryDeployment, A/B, metrics |
| `test_blue_green_deployment.cpp` | BlueGreenDeploymentFocusedTests | Blue/Green deployment |
| `test_binary_delta_patches.cpp` | BinaryDeltaPatchesFocusedTests | Delta generation, checksums, fallback |
| `test_coordinated_update_manager.cpp` | CoordinatedUpdateManagerFocusedTests | 25 tests: multi-node coordination |
| `test_in_place_schema_migrator.cpp` | — | In-place migration, preview, rollback |
| `test_notification_webhook.cpp` | NotificationWebhookFocusedTests | 40 tests: Slack/PagerDuty via HttpSendFunc |
| `test_preflight_health_check.cpp` | PreflightHealthCheckFocusedTests | 35+ tests: disk/memory/dependency check |
| `test_schema_migration_tester.cpp` | SchemaMigrationTesterFocusedTests | Staging validation |
| `test_parallel_file_downloads.cpp` | ParallelFileDownloadsFocusedTests | 29 tests: concurrency, throttling, resume |
| `test_dependency_resolution_engine.cpp` | DependencyResolutionEngineFocusedTests | 49 tests: topo-sort, cycle detection |
| `test_multi_tenant_update_scheduling.cpp` | MultiTenantUpdateSchedulingFocusedTests | 37 tests: maintenance windows, priorities |
| `test_manifest_database_file_deletion.cpp` | — | Tombstone guard, file cleanup |
| `test_distributed_cluster_updates.cpp` | DistributedClusterUpdatesFocusedTests | Cluster rolling update |

---

## Quick Links

- [Primary Documentation (README)](../../../src/updates/README.md)
- [ROADMAP.md](../../../src/updates/ROADMAP.md)
- [ARCHITECTURE.md](../../../src/updates/ARCHITECTURE.md)
- [FUTURE_ENHANCEMENTS.md](../../../src/updates/FUTURE_ENHANCEMENTS.md)
- [SECURITY.md](../../../src/updates/SECURITY.md)
- [CHANGELOG.md](../../../src/updates/CHANGELOG.md)
- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md) — Complete primary sources inventory

---

## Known Limitations

See **[`docs/de/updates/MISSING_IMPLEMENTATIONS.md`](../de/updates/MISSING_IMPLEMENTATIONS.md)** for the full findings report.

Summary (as of 2026-03-20, all addressed in this PR):
- README.md referenced 4 non-existent files (fixed)
- ROADMAP.md CI workflow paths were missing subdirectory prefixes (fixed)
- ROADMAP.md counted "9 standalone test targets" while listing 10 (fixed)
- Kubernetes operator integration still open (`[!]` in ROADMAP.md Phase 4)

---

*Generated 2026-03-20 · Branch: `develop`*
