---
Author: "ThemisDB Team"
Created: "2026-08-05"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Benchmarks for `prompt_engineering`

The benchmark sources are registered by the repository-level [`../CMakeLists.txt`](../CMakeLists.txt) when `THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark is available:

- `bench_prompt_engineering.cpp` registers cases for template lifecycle, validation, versioning, feedback, optimization, evaluation, metrics, and concurrent writes.
- `bench_rewrite_engine.cpp` currently uses local synthetic rewrite helpers; it does not call the module's `RewriteEngine` or `RewriteRuleLoader`. Its timings are not measurements of those production implementations.

## Build and run

Configure the repository with benchmarks enabled, then build and run the registered targets:

```bash
cmake --preset <existing-preset> -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build <build-directory> --target bench_prompt_engineering bench_rewrite_engine
<build-directory>/bin/bench_prompt_engineering --benchmark_format=json
<build-directory>/bin/bench_rewrite_engine --benchmark_format=json
```

The targets are only available when the Google Benchmark dependency is found. Benchmark source comments contain proposed microbenchmark thresholds; they are not evidence of measured release performance or a completed release gate.
