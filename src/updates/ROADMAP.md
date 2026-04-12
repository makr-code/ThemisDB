<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- validated: 2026-04-06 | Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

# Updates Module Roadmap

## Current Status
v1.8.0 – Production-ready zero-downtime update and migration system. HotReloadEngine, release manifest management, schema migration framework, digital signature verification, automatic backup, rollback, binary delta updates, canary rollout, CanaryDeployment (progressive rollout with latency/metric monitoring, A/B testing, auto-rollback), blue/green deployment, multi-node coordinated updates, notification webhooks, dry-run migration preview, pre-flight health checks, parallel file downloads, dependency resolution engine, and rollback checkpoint API are all implemented. Build system audit complete (March 2026): all 16 source files registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake; 10 standalone focused test targets added.

## Completed ✅
- [x] HotReloadEngine – atomic file replacement with fsync and all-or-nothing semantics
- [x] Resume-capable downloads from GitHub releases
- [x] Automatic backup (rollback points) before applying changes
- [x] CMS/PKCS#7 signature validation with X.509 certificate verification
- [x] Progress tracking via callback API
- [x] Dry-run mode (test upgrade without applying)
- [x] Platform support: Windows, Linux, macOS
- [x] Schema migration framework with versioned migrations
- [x] Version compatibility checking and upgrade path resolution
- [x] Rollback capability with multiple restore points (`listRollbackPoints`)
- [x] Incremental and full migration strategies
- [x] Dependency tracking for complex migrations
- [x] Update scheduling and notification system
- [x] Delta (binary diff) updates to reduce download size (PR: #2488)
- [x] Canary rollout mode (update a fraction of nodes first) (PR: #2587)
- [x] Canary Deployments – full `CanaryDeployment` builder API with latency/memory/CPU/disk I/O metrics, A/B testing, traffic splitting, auto-rollback on p99 breach (Issue: #4046)
  - Implemented: `CanaryDeployment` class in `include/updates/canary_rollout.h`, `src/updates/canary_rollout.cpp`
  - API: `setVersion`, `setStages`, `setErrorRateThreshold`, `setLatencyThreshold`, `deploy`, `onStageComplete`, `onRollback`, `reportLatency`, `reportMemoryUsage`, `reportCpuUsage`, `reportDiskIO`, `recordCustomMetric`, `enableABTesting`, `isCanaryRequest`, `isControlRequest`, `isNodeInCanaryGroup`, `getMetricsSnapshot`
  - Metrics: `LatencyStats` (p50/p95/p99), `CanaryMetricsSnapshot` (all telemetry), `ABTestConfig`
  - Tests: 50 focused tests added to `tests/test_canary_rollout.cpp` (84 total)
  - CI: `.github/workflows/04-release_canary-deployments-ci.yml`
- [x] In-place schema migration without data copy for additive changes (Issue: #2480)
- [x] Schema migration testing framework (apply to staging before production) (Issue: #2487)
- [x] Migration dry-run with detailed change preview (Issue: #2481)
  - Implemented: `InPlaceSchemaMigrator::preview()` → `MigrationChangePreview` (added/removed/modified columns, is_additive flag)
  - Tests: 10+ unit tests in `tests/test_in_place_schema_migrator.cpp` (Phase 7 suite)
- [x] Notification webhooks (Slack, PagerDuty) on update success/failure (Issue: #2482)
  - Implemented: `include/updates/notification_webhook.h`, `src/updates/notification_webhook.cpp`
  - Tests: 40 unit tests in `tests/test_notification_webhook.cpp` via injectable `HttpSendFunc`
- [x] Update pre-flight health checks (disk space, memory, dependency versions) (Issue: #2490)
  - Implemented: `include/updates/preflight_health_check.h`, `src/updates/preflight_health_check.cpp`
  - `IHealthCheck` abstract interface + `DiskSpaceChecker`, `MemoryHeadroomChecker`, `DependencyVersionChecker`, `PreflightHealthChecker`
  - Injectable providers for full testability (no real filesystem/sysinfo required)
  - Completes in ≤ 2 s; disk check enforces ≥ 2× bundle size of free space
  - Tests: 35+ unit tests in `tests/test_preflight_health_check.cpp`
- [x] Build system audit: all 16 src/updates/*.cpp registered in cmake/CMakeLists.txt and cmake/ModularBuild.cmake; standalone focused test targets exist for all independently testable modules
- [x] 10 standalone focused test targets added to tests/CMakeLists.txt (UpdatesProduction, BlueGreen, CanaryRollout, BinaryDeltaPatches, CoordinatedUpdateManager, InPlaceSchemaMigrator, NotificationWebhook, PreflightHealthCheck, SchemaMigrationTester, ParallelFileDownloads)
- [x] CI/CD workflow for automatic patch generation: `.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml`
- [x] Parallel file downloads – configurable concurrency, bandwidth throttling, priority queue, resume support (Issue: #128)
  - Implemented: `include/updates/parallel_downloader.h`, `src/updates/parallel_downloader.cpp`
  - API: `DownloadTask`, `DownloadResult`, `DownloadBatchStats`, `ParallelDownloader`
  - Tests: 29 focused tests in `tests/test_parallel_file_downloads.cpp`
  - CI: `.github/workflows/02-feature-modules_storage_parallel-file-downloads-ci.yml`
- [x] Dependency resolution engine – topological sort, cycle detection, version constraints, conflict resolution, backfill (Issue: #216)
  - Implemented: `include/updates/dependency_resolver.h`, `src/updates/dependency_resolver.cpp`
  - API: `Dependency`, `UpdateStep`, `DependencyConflict`, `ResolutionResult`, `DependencyResolver`
  - Tests: 49 focused tests in `tests/test_dependency_resolution_engine.cpp`
  - CI: `.github/workflows/02-feature-modules_dependency-resolution-engine-ci.yml`
- [x] ManifestDatabase::deleteManifest() – deletes associated binary files after RocksDB entry commit; tombstone guard prevents orphaned files on crash (v1.8.0)
  - Implemented: `src/updates/manifest_database.cpp` (tombstone key + filesystem::remove per associated file)
  - Tests: `tests/test_manifest_database_file_deletion.cpp`
  - CI: `.github/workflows/02-feature-modules_storage_manifest-database-file-deletion-ci.yml`
- [x] Cluster-wide rolling update via `ClusterUpdateManager` – leader-last ordering, health checks per node, automatic rollback on failure, cancellation (v1.7.0)
  - Implemented: `include/updates/cluster_update_manager.h`, `src/updates/cluster_update_manager.cpp`
  - Tests: `tests/test_distributed_cluster_updates.cpp`
  - CI: `.github/workflows/06-infrastructure_distributed_distributed-cluster-updates-ci.yml`

## In Progress 🚧
<!-- No items currently in progress -->

## Planned Features 📋

### Short-term (Next 3-6 months)
<!-- All short-term items are complete — see Completed ✅ section above -->

### v1.8.0 – Completed ✅
- [x] Multi-tenant update scheduling – per-tenant maintenance windows, blackout periods, priority tiers (critical/normal/low), tenant consent, and per-tenant rollback (Issue: #262)
  - Implemented: `include/updates/tenant_update_scheduler.h`, `src/updates/tenant_update_scheduler.cpp`
  - API: `MaintenanceWindow`, `BlackoutPeriod`, `UpdatePolicy`, `UpdatePriority`, `TenantUpdateStatus`, `TenantUpdateScheduler`
  - Tests: 37 focused tests in `tests/test_multi_tenant_update_scheduling.cpp`
  - CI: `.github/workflows/02-feature-modules_multi-tenant-update-scheduling-ci.yml`
- [x] Rollback checkpoint API on `UpdateStateMachine` (v1.8.0)
  - Implemented: `CheckpointId`/`Checkpoint` types in `include/updates/update_state_machine.h`; methods in `src/updates/update_state_machine.cpp`
  - API: `createCheckpoint(description)` → `CheckpointId`, `rollbackToCheckpoint(id)`, `listCheckpoints()`, `clearCheckpoints()`, `setHistoryLogger()`
  - `UpdateHistoryLogger` integration: emits `checkpoint_created` and `checkpoint_rollback` audit entries
  - Tests: 16 focused tests in `CheckpointTest` suite (`tests/test_updates_production.cpp`)

### Long-term (6-12 months)
- [!] Kubernetes operator integration (rolling update coordination) (Issue: #2483)

## Implementation Phases

### Phase 1: Hot Reload & Schema Migration Framework (Status: Completed ✅)
- [x] `HotReloadEngine` – atomic file replacement with fsync and all-or-nothing semantics
- [x] Resume-capable downloads from GitHub releases
- [x] Automatic backup (rollback points) before applying changes
- [x] CMS/PKCS#7 signature validation with X.509 certificate verification
- [x] Progress tracking via callback API and dry-run mode
- [x] Platform support: Windows, Linux, macOS
- [x] Schema migration framework with versioned migrations
- [x] Version compatibility checking and upgrade path resolution
- [x] Rollback capability with multiple restore points (`listRollbackPoints`)
- [x] Incremental and full migration strategies with dependency tracking
- [x] Update scheduling and notification system

### Phase 2: Delta Updates & Canary Rollout (Status: Completed ✅)
- [x] Delta (binary diff) updates to reduce download size
- [x] Canary rollout mode (update a fraction of nodes first)
- [x] Update pre-flight health checks (disk space, memory, dependency versions)

### Phase 3: In-Place Migration & Notification Webhooks (Status: Completed ✅)
- [x] In-place schema migration without data copy for additive changes
- [x] Migration dry-run with detailed change preview (`InPlaceSchemaMigrator::preview()`)
- [x] Notification webhooks (Slack, PagerDuty) on update success/failure
  - API: `UpdateEvent` enum, `UpdateEventPayload`, `SlackConfig`, `PagerDutyConfig`, `NotificationWebhook`
  - Slack: color-coded attachments (good/warning/danger), structured fields, injectable sender
  - PagerDuty: Events API v2, trigger/resolve actions, dedup_key per version, severity mapping
  - Error handling: per-channel failures logged, both channels always attempted, returns false on any failure
  - Tests: injectable `HttpSendFunc` stub, no network required; 40 unit tests
  - Performance: 10 s HTTP timeout, 5 s connect timeout, <100 ms overhead
- [x] Automatic rollback on post-update health check failure
- [x] Update history log (who, when, from/to version)

### Phase 4: Kubernetes & Blue/Green Deployment (Status: In Progress 🚧)
- [ ] Kubernetes operator integration (rolling update coordination)
- [x] Blue/green deployment support (run two versions simultaneously)
- [x] Multi-node coordinated update with replication-safe sequencing
- [x] Update bundle signing with hardware-backed keys (HSM)
- [x] Schema migration testing framework (apply to staging before production)

### Phase 5: Parallel Downloads & Performance (Status: Completed ✅)
- [x] Parallel file downloads with configurable concurrency (Issue: #128)
- [x] Bandwidth throttling via token-bucket algorithm
- [x] Priority queue: critical files (executables) downloaded first
- [x] Per-file resume support via HTTP Range requests
- [x] Dependency resolution engine: topological sort, cycle detection, version constraints, conflict resolution, backfill (Issue: #216)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (DeltaUpdateEngine: 29 tests; InPlaceSchemaMigrator+preview: 31 tests; NotificationWebhook: 40 tests; PreflightHealthChecker: 35 tests; CoordinatedUpdateManager: 25 tests; ParallelDownloader: 29 tests; DependencyResolver: 49 tests; CheckpointAPI: 17 tests; module total: 255 tests)
- [x] Integration tests (applyDelta end-to-end: generate → apply → hash verify → atomic install; InPlaceSchemaMigrator: apply → version verify → history check)
- [?] Performance benchmarks (migration duration, downtime measurement)
- [x] Security audit (path traversal in update bundles fixed; `isSafePath` guard in `applyDelta`; InPlaceSchemaMigrator: metadata-only, no data access, no path operations; PreflightHealthChecker: injectable providers, no privilege escalation; ParallelDownloader: hash verification, corrupt file auto-removal)
- [x] Documentation complete (full API documentation in `delta_update_engine.h`, `in_place_schema_migrator.h`, `notification_webhook.h`, `preflight_health_check.h`, `parallel_downloader.h`)
- [x] API stability guaranteed (all new APIs are additive; no existing API changed)
- [x] Automatic patch generation in CI/CD (`.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml` validates the generatePatch() API and CI/CD generation use-cases)

## Known Issues & Limitations
- HotReloadEngine is single-threaded; concurrent updates are not allowed.
- Download network protocols (HTTP/S) are handled by the utils module.
- Concurrent update prevention uses filesystem locks; cross-node coordination is provided by `CoordinatedUpdateManager` (transport-agnostic via injected callbacks).

## Breaking Changes
- `HotReloadEngine::Config` is stable from v1.x; new optional fields only.
- Migration version numbering scheme is fixed; no re-sequencing allowed after initial deployment.
