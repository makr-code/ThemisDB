---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Module Performance Evidence

## Purpose

Inventory current config-related benchmark code and build targets. This page
does not assert measured performance, threshold compliance, or release gating.

## Benchmarks

| Source | CMake target | Coverage visible in source |
|---|---|---|
| `benchmarks/config/bench_config_path_resolver.cpp` | `bench_config_path_resolver` | Resolver cache hit/miss, fallback, mapping, aggregation, cache-rate, metrics scrape and concurrent-hit cases |
| `benchmarks/config/bench_config_migration_scanner.cpp` | `bench_config_migration_scanner` | Extension check, single-file scanning, synthetic 1K and 10K trees |
| `benchmarks/config/bench_config_release_gates.cpp` | `bench_config_release_gates` | Labeled resolver, validation, store, watcher-constant and metrics placeholder cases |

Target registration is in `benchmarks/CMakeLists.txt`. The resolver mapping
benchmark cases include `BM_MapLegacyToNew` and `BM_MapLegacyToNew_Miss`.
The migration-scanner 1K/10K labels state expected timings, but the code does
not assert those thresholds. In `bench_config_release_gates.cpp`, watcher
measurement observes contract constants and the metrics-overhead case is
explicitly a placeholder. No `release_gate_manifest_config.json` exists in the
scoped benchmark directory.

**Buildability status:** the release-gate source now calls
`validateFromString(config_.dump(), false, schema_)`, matching the declared
validator API. A standard `community-release` configure was blocked because
RocksDB is not installed; the diagnostic configure proceeded past RocksDB but
was blocked by missing `fmt`. The benchmark target has therefore not been
compiled, and buildability remains unverified in this environment.

## Evidence required before claims

- Run the relevant target in a recorded build profile on identified hardware.
- Compare results against a maintained baseline and define statistical
  acceptance criteria.
- Keep the measurement artifact and run command with any claim of a passing
  threshold.

No benchmark, build, or test was run for this documentation update.
