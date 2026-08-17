# ThemisDB Updates Module

<!-- Status: Production Ready (Wave A Consistency) | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PRODUCTION_REQUIREMENTS.md -->
<!-- Wave Context: Wave A (Runtime Reliability Q3-Q4 2026) — MVCC Consistency + Concurrent Updates + Conflict Resolution -->

## Module Purpose

Production-capable release-manifest handling, state-machine-driven update orchestration, delta update behavior, cluster and tenant scheduling paths, and schema/update lifecycle control for ThemisDB. **Batch 5 enhancement focus: MVCC snapshot correctness, concurrent update coordination, conflict resolution strategies, consistency under partitions**.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| update_state_machine.cpp | update state machine behavior |
| release_manifest.cpp | release manifest behavior |
| manifest_database.cpp | manifest storage behavior |
| delta_update_engine.cpp | delta patch generation and apply behavior |
| hot_reload_engine.cpp | hot reload behavior |
| coordinated_update_manager.cpp | coordinated update behavior |
| cluster_update_manager.cpp | cluster-wide update behavior |
| tenant_update_scheduler.cpp | tenant update scheduling behavior |
| in_place_schema_migrator.cpp | in-place schema migration behavior |
| schema_migration.cpp | schema migration behavior |
| schema_migration_tester.cpp | schema migration test/dry-run behavior |
| build_verifier.cpp | build verification behavior |
| dependency_resolver.cpp | update dependency resolution behavior |
| canary_rollout.cpp | canary rollout behavior |
| notification_webhook.cpp | update notification behavior |
| parallel_downloader.cpp | parallel download behavior |
| preflight_health_check.cpp | preflight health-check behavior |
| update_history_logger.cpp | update history behavior |
| updates_config.cpp | update configuration behavior |
| blue_green_deployment.cpp | blue/green deployment behavior |
| hardware_telemetry.cpp | hardware telemetry behavior |

## Scope

In scope:
- update orchestration, manifest, rollback, and patch pipeline behavior
- schema/update lifecycle and rollout control behavior
- health-check, notification, telemetry, and scheduling behavior

Out of scope:
- unrelated business-domain logic outside update subsystem boundaries
- non-update storage/network implementations owned by other modules

## Runtime Behavior and Limits

- update transitions and rollback behavior expose explicit state outcomes.
- delta patch and manifest behavior remain deterministic and diagnosable.
- migration and rollout paths are bounded by coordination and preflight constraints.
- cluster, tenant, and telemetry integration paths remain observable.

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
- Verified behavior surfaces:
  - update state/manifest/delta/migration/rollout and scheduling paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md