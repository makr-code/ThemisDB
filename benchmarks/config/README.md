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

# Config Benchmarks

## Scope and build targets

This directory contains resolver, migration-scanner, and release-gate benchmark
sources. Targets are registered in `benchmarks/CMakeLists.txt`:

- `bench_config_path_resolver`
- `bench_config_migration_scanner`
- `bench_config_release_gates`

## What the sources measure

- Resolver: path cache, mapping, fallback, deprecation aggregation, metrics
  scrape and concurrent cache hits.
- Migration scanner: extension filtering, scanning one file, and generated
  1,000-/10,000-file trees.
- Release-gate-named source: resolver, schema, encrypted-store, watcher-constant
  and metrics-placeholder cases.

Names and labels in benchmark code are not measured results. The 1K/10K scan
labels do not implement a threshold assertion; the watcher case reports
constants and the metrics-overhead case is explicitly a placeholder. See
[`../../src/config/PERFORMANCE_EXPECTATIONS.md`](../../src/config/PERFORMANCE_EXPECTATIONS.md)
for evidence requirements.

The release-gate source now serializes its fixture and calls
`ConfigSchemaValidator::validateFromString()`, matching the public API.
Compilation remains unverified: the standard community configure requires
RocksDB, while the diagnostic configure used for this check requires `fmt`.
Target registration and a corrected call site do not prove that the benchmark
compiles or provide executable release evidence.
