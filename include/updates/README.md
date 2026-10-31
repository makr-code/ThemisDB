# ThemisDB Updates Public Headers

This directory declares the C++ interfaces for the updates module. The headers
are the authoritative source for signatures, defaults, ownership, and
thread-safety; this page is an index, not a second API specification.

## Public interface map

| Area | Headers |
|---|---|
| Update lifecycle and contracts | [`update_state_machine.h`](update_state_machine.h), [`updates_api_contract.h`](updates_api_contract.h), [`updates_diagnostics.h`](updates_diagnostics.h), [`updates_diagnostic_emitter.h`](updates_diagnostic_emitter.h), [`updates_operator_diagnostics.h`](updates_operator_diagnostics.h), [`updates_edge_case_handler.h`](updates_edge_case_handler.h) |
| Releases, files, and patching | [`release_manifest.h`](release_manifest.h), [`manifest_database.h`](manifest_database.h), [`delta_update_engine.h`](delta_update_engine.h), [`dependency_resolver.h`](dependency_resolver.h), [`parallel_downloader.h`](parallel_downloader.h) |
| Applying and rolling out updates | [`hot_reload_engine.h`](hot_reload_engine.h), [`canary_rollout.h`](canary_rollout.h), [`blue_green_deployment.h`](blue_green_deployment.h), [`cluster_update_manager.h`](cluster_update_manager.h), [`coordinated_update_manager.h`](coordinated_update_manager.h), [`tenant_update_scheduler.h`](tenant_update_scheduler.h) |
| Migration and preflight | [`schema_migration.h`](schema_migration.h), [`schema_migration_tester.h`](schema_migration_tester.h), [`in_place_schema_migrator.h`](in_place_schema_migrator.h), [`preflight_health_check.h`](preflight_health_check.h) |
| Supporting services | [`build_verifier.h`](build_verifier.h), [`hardware_telemetry.h`](hardware_telemetry.h), [`notification_webhook.h`](notification_webhook.h), [`update_history_logger.h`](update_history_logger.h), [`updates_config.h`](updates_config.h) |

`batch5_safety_helpers.h` contains helper/test-oriented safety utilities; it
does not define the update engine API.

## Integration cautions

- `DeltaUpdateEngine` explicitly is not thread-safe. See its header before
  sharing an instance or changing patch-application concurrency.
- `HotReloadEngine::Config` controls signature verification, backups, dry-run,
  health-check rollback, and optional history logging. The caller must inspect
  operation results and retain rollback identifiers.
- `UpdatesConfig` defaults its update, auto-update, hot-reload, notification,
  canary, and telemetry enablement switches to `false`; inspect the declaration
  and parsing implementation for exact configuration behavior.
- `UpdateStateMachine` records control state. A state checkpoint is not a copy
  of installed files and does not replace file backup/rollback handling.
- Parsing a manifest or configuration is not by itself proof that an artifact
  is authentic or that a deployment is authorized.

## Usage

Include the specific public header needed by the integration and follow its
declaration comments. This index intentionally does not repeat call signatures
or provide copied code samples. The integration-oriented summary is
[`../../src/updates/API.md`](../../src/updates/API.md).

## Installation

There is no module-specific installation procedure documented here. Use the
repository's build and packaging instructions and verify how the consumer target
receives the project include directories.

## Related documentation

- Module behavior and implementation: [`../../src/updates/README.md`](../../src/updates/README.md)
- API summary: [`../../src/updates/API.md`](../../src/updates/API.md)
- Architecture: [`../../src/updates/ARCHITECTURE.md`](../../src/updates/ARCHITECTURE.md)
- Build and focused-test registration: [`../../tests/updates/CMakeLists.txt`](../../tests/updates/CMakeLists.txt)
- Benchmark registration: [`../../benchmarks/updates/CMakeLists.txt`](../../benchmarks/updates/CMakeLists.txt)

Do not use copied examples from old versions of this page as executable
integration guidance. Confirm each call against the current header declarations
and implementation.
