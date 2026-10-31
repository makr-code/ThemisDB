---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: benchmarks/process/CMakeLists.txt; benchmarks/process/*.cpp; include/process/process_light_retriever.h; src/process/process_light_retriever.cpp; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Module Performance Expectations

**AI provenance:** Exact model backend not verifiable from final document text alone.

This page describes the current benchmark registration surface only. Previous drafts listed different gate totals, targets, pass states, baselines, and benchmark names; those assertions are not retained as current release evidence. No current performance result or release gate is claimed here.

## Registered benchmark sources

[`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt) registers these standard benchmarks when `THEMIS_BUILD_BENCHMARKS` is enabled:

The current CMake file contains **12 benchmark target registrations**. Those 12 source files contain **88 Google Benchmark registration macro declarations** (87 `BENCHMARK(...)` calls and one `BENCHMARK_REGISTER_F(...)` call). These are different counting units; neither number establishes a count of release/performance gates, enforced thresholds, or passing results.

- `bench_process_import_retrieval.cpp`
- `bench_process_mining.cpp`
- `bench_process_retrieval.cpp`
- `bench_process_release_gates.cpp`
- `bench_process_concurrency_gates.cpp`
- `bench_process_determinism_gates.cpp`
- `bench_process_diagnostics_overhead.cpp`
- `bench_process_parser_gates.cpp`
- `bench_process_linker_gates.cpp`
- `bench_process_retriever_gates.cpp`
- `bench_process_advanced_workflows.cpp`
- `bench_process_dedicated_gates.cpp`

Registration does not prove that a benchmark source measures production behavior, that a named gate is enforced, or that any run passed. For example, inspect the benchmark implementation before interpreting simulated or stubbed operations as module performance.

## Interpretation and follow-up

- **Current measured baseline:** TODO — record hardware, build configuration, benchmark binary/revision, raw output, and percentile methodology.
- **Gate count, thresholds, and pass/fail policy:** TODO — unresolved. Reconcile benchmark declarations with current benchmark semantics and release automation before describing a gate count or any threshold as enforced.
- **Run command/target names:** TODO — resolve from the root build configuration and `themis_add_standard_benchmark` implementation for the selected preset; this page does not prescribe an unverified command.
- **Resource limits:** Do not use this page to infer bounded latency or resource use. In particular, `ProcessLightRetriever::retrieve()` does not currently enforce its header-declared resource budgets; see [implementation notes](IMPLEMENTATION.md).

## Related sources

- [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt)
- [Process benchmark catalog](../../benchmarks/process/README.md)
- [Module roadmap](ROADMAP.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md)
