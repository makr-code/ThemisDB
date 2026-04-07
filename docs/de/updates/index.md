# Updates-Modul

<!-- Status: current | validated: 2026-04-06 | Primary: ../../../src/updates/ | ../../../include/updates/ -->
<!-- Links: ../../../src/updates/README.md · ../../../src/updates/ROADMAP.md · PRIMARY_SOURCES.md · missing-implementations.md -->

**Stand:** 6. April 2026  
**Version:** v1.8.0 (Production-ready)  
**Kategorie:** Zero-Downtime-Updates / Schema-Migration / Deployment

---

## Übersicht

Das Updates-Modul ist ThemisDB's **Zero-Downtime-Update- und Migrations-System**. Es ermöglicht sichere, atomare Software-Updates, Schema-Migrationen und koordinierte Cluster-Upgrades ohne Datenverlust oder nennenswerte Downtime.

**Operative Features (v1.8.0):**
- Atomarer Hot-Reload mit `fsync` + `rename(2)` / `MoveFileExW` Garantien
- CMS/PKCS#7 Signaturvalidierung mit X.509-Zertifikatskette
- Binäre Delta-Updates (ZSTD_DICT, VCDIFF/RFC 3284) zur Bandbreitenreduktion
- Canary Rollout mit Latenz-/Fehlerquoten-Überwachung und Auto-Rollback
- Blue/Green Deployment für simultanen Betrieb zweier Versionen
- In-place Schema-Migration ohne Datenkopie (additive Änderungen)
- Multi-Node koordinierte Updates mit Replikations-sicherer Sequenzierung
- Cluster-weite Rolling Updates via `ClusterUpdateManager` (Leader-zuletzt)
- Notification Webhooks (Slack, PagerDuty) bei Update-Ereignissen
- Pre-flight Health Checks (Speicherplatz, Arbeitsspeicher, Abhängigkeitsversionen)
- Parallele Dateidownloads mit Token-Bucket-Bandbreitenbegrenzung
- Dependency Resolution Engine (topologische Sortierung, Konfliktlösung)
- Multi-Tenant Update-Scheduling mit Wartungsfenstern und Blackout-Perioden
- ManifestDatabase mit atomarer Datei-Löschung beim Eintrag-Entfernen

**Status:** 🟢 Production-ready (v1.8.0)

---

## Komponenten-Übersicht

| Komponente | Quelldatei | Header | Status |
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

| Workflow | Pfad | Abgedeckte Komponente |
|---|---|---|
| binary-delta-patches-ci | `.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml` | DeltaUpdateEngine |
| canary-deployments-ci | `.github/workflows/04-release_canary-deployments-ci.yml` | CanaryDeployment |
| parallel-file-downloads-ci | `.github/workflows/02-feature-modules_storage_parallel-file-downloads-ci.yml` | ParallelDownloader |
| dependency-resolution-engine-ci | `.github/workflows/02-feature-modules_dependency-resolution-engine-ci.yml` | DependencyResolver |
| multi-tenant-update-scheduling-ci | `.github/workflows/02-feature-modules_multi-tenant-update-scheduling-ci.yml` | TenantUpdateScheduler |
| manifest-database-file-deletion-ci | `.github/workflows/02-feature-modules_storage_manifest-database-file-deletion-ci.yml` | ManifestDatabase |
| distributed-cluster-updates-ci | `.github/workflows/06-infrastructure_distributed_distributed-cluster-updates-ci.yml` | ClusterUpdateManager |

---

## Test-Abdeckung

| Testdatei | Test-Target | Beschreibung |
|---|---|---|
| `test_updates_production.cpp` | UpdatesProductionFocusedTests | Vollständiger Integrations-Lifecycle |
| `test_canary_rollout.cpp` | CanaryRolloutFocusedTests | 84 Tests inkl. CanaryDeployment, A/B, Metriken |
| `test_blue_green_deployment.cpp` | BlueGreenDeploymentFocusedTests | Blue/Green Deployment |
| `test_binary_delta_patches.cpp` | BinaryDeltaPatchesFocusedTests | Delta-Generierung, Checksummen, Fallback |
| `test_coordinated_update_manager.cpp` | CoordinatedUpdateManagerFocusedTests | 25 Tests: Multi-Node-Koordination |
| `test_in_place_schema_migrator.cpp` | — | In-Place-Migration, Preview, Rollback |
| `test_notification_webhook.cpp` | NotificationWebhookFocusedTests | 40 Tests: Slack/PagerDuty via HttpSendFunc |
| `test_preflight_health_check.cpp` | PreflightHealthCheckFocusedTests | 35+ Tests: Disk/Speicher/Abhängigkeitscheck |
| `test_schema_migration_tester.cpp` | SchemaMigrationTesterFocusedTests | Staging-Validierung |
| `test_parallel_file_downloads.cpp` | ParallelFileDownloadsFocusedTests | 29 Tests: Parallelität, Throttling, Resume |
| `test_dependency_resolution_engine.cpp` | DependencyResolutionEngineFocusedTests | 49 Tests: Topo-Sort, Cycle Detection |
| `test_multi_tenant_update_scheduling.cpp` | MultiTenantUpdateSchedulingFocusedTests | 37 Tests: Wartungsfenster, Prioritäten |
| `test_manifest_database_file_deletion.cpp` | — | Tombstone-Schutz, Datei-Löschung |
| `test_distributed_cluster_updates.cpp` | DistributedClusterUpdatesFocusedTests | Cluster-Rolling-Update |

---

## Quicklinks

- [Primäre Dokumentation (README)](../../../src/updates/README.md)
- [ROADMAP.md](../../../src/updates/ROADMAP.md)
- [ARCHITECTURE.md](../../../src/updates/ARCHITECTURE.md)
- [FUTURE_ENHANCEMENTS.md](../../../src/updates/FUTURE_ENHANCEMENTS.md)
- [SECURITY.md](../../../src/updates/SECURITY.md)
- [CHANGELOG.md](../../../src/updates/CHANGELOG.md)
- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md) — Vollständiges Primärquellen-Inventar
- [missing-implementations.md](./missing-implementations.md) — Reality-Check-Befunde

---

## Bekannte Einschränkungen

Detaillierter Report: **[missing-implementations.md](./missing-implementations.md)**

Kurzfassung (Stand 2026-03-20):
- README.md referenzierte 4 nicht existierende Dateien (behoben in diesem PR)
- ROADMAP.md CI-Workflow-Pfade fehlten Unterverzeichnis-Präfixe (behoben in diesem PR)
- ROADMAP.md zählte "9 standalone test targets" obwohl 10 in der Liste (behoben in diesem PR)
- Kubernetes Operator-Integration noch offen (`[!]` in ROADMAP.md Phase 4)

---

*Generiert am 2026-03-20 · Branch: `develop` · Autor: Copilot*
