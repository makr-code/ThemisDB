---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
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

Tests live in `tests/config/`. The directory's `CMakeLists.txt` creates one
focused executable per `test_*.cpp`, with target names
`module_config_<test-file-stem>_focused`; CTest names are
`<test-file-stem>_config_FocusedTests`. For example:

```sh
cmake --build --preset linux-release --target module_config_test_config_path_resolver_focused
ctest --preset linux-release -R test_config_path_resolver_config_FocusedTests --output-on-failure
```

Select the matching target for the changed concern. The module test directory
also includes migration scanner, LRU boundary, metrics, and hardening tests;
the hardening files cover resolver/validator and watcher/store edges.

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
6. Run performance benchmarks for hot-path changes and document the run profile
   and baseline rather than assuming thresholds have been measured.

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
