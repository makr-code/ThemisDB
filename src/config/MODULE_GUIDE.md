---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Config Module Developer Guide

## Start here

The module supports config path mapping/resolution, JSON/YAML schema
validation, file-change notification, metrics and audit surfaces, and an
in-memory encrypted store. Read [`ARCHITECTURE.md`](ARCHITECTURE.md) for
ownership boundaries, [`API.md`](API.md) for caller semantics, and
[`IMPLEMENTATION.md`](IMPLEMENTATION.md) before changing an implementation.
The frozen behavioral constraints are in
[`../../include/config/config_contract.h`](../../include/config/config_contract.h).

## Build and focused tests

The repository root registers focused tests in `tests/CMakeLists.txt`.
Configure the repository with a supported preset, then build a test target and
select its registered CTest name. For example:

```sh
cmake --preset linux-release
cmake --build --preset linux-release --target test_config_path_resolver_focused
ctest --preset linux-release -R ConfigPathResolverFocusedTests --output-on-failure
```

Other root-registered target / CTest pairs are:

| Target | CTest name |
|---|---|
| `test_config_file_watcher_focused` | `ConfigFileWatcherFocusedTests` |
| `test_config_schema_validator_focused` | `ConfigSchemaValidatorFocusedTests` |
| `test_config_migration_scanner_focused` | `ConfigMigrationScannerFocusedTests` |
| `test_config_coverage_focused` | `ConfigCoverageFocusedTests` |
| `test_config_metrics_scrape_focused` | `MetricsScrapeFocusedTests` |
| `test_config_encrypted_store_focused` | `ConfigEncryptedStoreFocusedTests` |
| `test_config_hardening_resolver_validator_focused` | `ConfigHardeningResolverValidatorFocusedTests` |
| `test_config_hardening_watcher_store_focused` | `ConfigHardeningWatcherStoreFocusedTests` |

Select the matching target for the changed concern. The per-directory
`tests/config/CMakeLists.txt` uses a different loop-based naming scheme; the
commands above follow the root test registrations. `test_lru_boundary_focused.cpp`
is present in the directory, but is not registered in the inspected root
`tests/CMakeLists.txt`.

## Change checklist

1. Keep the public declaration under `include/config/` and implementation
   details under `src/config/`.
2. Preserve failure distinctions: optional absence, structured resolver
   exceptions, validation diagnostics, watcher startup failure, and explicit
   encryption/authentication errors are not interchangeable.
3. Add or extend focused tests for input bounds, invalid or missing resources,
   race/churn behavior, and error paths affected by the change.
4. Review path and secret exposure. Do not log plaintext config values or
   publish serialized encrypted-store snapshots.
5. Update the relevant module docs when behavior or contracts change; keep the
   `ROADMAP.md` / `FUTURE_ENHANCEMENTS.md` status aligned with their own scope.
6. For performance-sensitive changes, use the actual benchmark targets listed
   in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); report the
   run profile and baseline. Source labels alone do not prove a threshold passed.

## Common integration edges

- `validateWithSchemaFile()` uses resolver mapping for the schema path but
  takes the config file path directly.
- Resolver hot reload only clears its path cache. The application must decide
  when to reload and apply validated settings.
- File watcher events cover `.yaml`, `.yml`, and `.json`; other extensions are
  not watched by this implementation.
- `ConfigAuditLog` is a bounded in-process log and is not durable compliance
  storage.

Operational procedures are in [`RUNBOOK.md`](RUNBOOK.md) and
[`OPERATIONS.md`](OPERATIONS.md).
