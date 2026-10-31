---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Focused Tests

## Scope

These are C++ unit/focused test sources for config module behavior. There is no
`policies.test.yaml` in this directory; earlier claims about a shared YAML test
policy and test-time configuration loading were unsupported.

## Root build and CTest registrations

The repository root registers the following targets and tests in
`tests/CMakeLists.txt`:

| Build target | CTest name |
|---|---|
| `test_config_path_resolver_focused` | `ConfigPathResolverFocusedTests` |
| `test_config_file_watcher_focused` | `ConfigFileWatcherFocusedTests` |
| `test_config_schema_validator_focused` | `ConfigSchemaValidatorFocusedTests` |
| `test_config_migration_scanner_focused` | `ConfigMigrationScannerFocusedTests` |
| `test_config_coverage_focused` | `ConfigCoverageFocusedTests` |
| `test_config_metrics_scrape_focused` | `MetricsScrapeFocusedTests` |
| `test_config_encrypted_store_focused` | `ConfigEncryptedStoreFocusedTests` |
| `test_config_hardening_resolver_validator_focused` | `ConfigHardeningResolverValidatorFocusedTests` |
| `test_config_hardening_watcher_store_focused` | `ConfigHardeningWatcherStoreFocusedTests` |

Example (after configuring an appropriate preset):

```sh
cmake --build --preset linux-release --target test_config_path_resolver_focused
ctest --preset linux-release -R ConfigPathResolverFocusedTests --output-on-failure
```

Use the target/CTest pair for the changed behavior; a successful build or test
does not by itself establish the full `config_contract.h` contract.

## Test source map

- Resolver: `test_config_path_resolver.cpp`
- Parser/schema: `test_config_schema_validator.cpp`
- Watcher: `test_config_file_watcher.cpp`
- Store: `test_config_encrypted_store.cpp`
- Contract-edge suites: `test_config_hardening_resolver_validator.cpp`,
  `test_config_hardening_watcher_store.cpp`
- Metrics/audit and supporting types: `test_config_metrics_scrape.cpp`,
  `test_config_coverage.cpp`
- Migration scanner: `test_config_migration_scanner.cpp`
- `test_lru_boundary_focused.cpp` exists in the directory but was not found in
  the root test registrations inspected for this update.

See [`ARCHITECTURE.md`](ARCHITECTURE.md),
[`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md), and
[`ROADMAP.md`](ROADMAP.md) for test-layer boundaries and open verification.
