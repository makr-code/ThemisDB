---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
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

This page separates source registrations, source-commented targets, configured comparison expectations, and observed run evidence. These are distinct facts; none alone establishes Process performance or release readiness.

## Registered benchmark sources

[`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt) registers these standard benchmarks when `THEMIS_BUILD_BENCHMARKS` is enabled:

The current CMake file contains **12 benchmark target registrations**. Those 12 source files contain **88 Google Benchmark registration macro declarations** (87 `BENCHMARK(...)` calls and one `BENCHMARK_REGISTER_F(...)` call). This is a source-syntax count; parameterized cases and repetitions can produce a different number of runtime result rows. Neither count is a count of performance gates.

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

- **Configured Process thresholds:** `benchmarks/compare_expectations.py` reads rows under a module's `## Specific Expectations` section. This file has no such section, so the comparator currently discovers **zero Process expectation targets** and applies no Process-specific numeric threshold. The weekly workflow's overall result cannot be attributed to Process in this state.
- **Source-commented targets:** `bench_process_dedicated_gates.cpp` lists four PM-BM targets: spawn p95 < 1 ms; terminate p95 < 500 µs; signal delivery ≥ 1 M/s; and four-thread spawn throughput ≥ 200 k/s. These are comments, not comparator inputs or runtime assertions. The benchmark uses in-process stub manager/signal-bus classes; it does not exercise OS process operations. Its Google Benchmark repetition aggregates do not calculate the named p95/p99 percentiles.
- **Release coverage gate (separate from performance):** `.github/workflows/gate-release-benchmark-validation.yml` requires 100% source-to-CMake-to-CTest coverage for listed critical modules and 95% for other discovered modules. Process is not in the critical list. The report maps 14 Process benchmark targets and records 14/14 CTest registrations (100%). Run [#19](https://github.com/makr-code/ThemisDB/actions/runs/41001268780) passed overall on 2026-10-30 with Process at 14/14. Latest run [#21](https://github.com/makr-code/ThemisDB/actions/runs/41271872461), 2026-11-01, also reports Process at 14/14 but fails overall because `training/bench_lora_data_paths.cpp` is unregistered. This is registration coverage, not benchmark execution or performance.
- **Historical result artifact:** [`artifacts/nightly/bench_process_mining.json`](../../artifacts/nightly/bench_process_mining.json) contains 22 results from 2026-04-15 (Release, four CPUs, one repetition per result). It has no source revision identifier or associated threshold-comparison report, so it is historical sample data, not a current baseline or a pass record.
- **Latest visible weekly performance run:** [run 40257720669](https://github.com/makr-code/ThemisDB/actions/runs/40257720669), dated 2026-10-26, failed during CMake configure because RocksDB was not found. Build, benchmark execution, and report jobs were skipped; this run provides no Process performance result.
- **Current performance baseline and Process performance-pass status:** not established by the current source, historical artifact, or latest visible weekly performance run. A reproducible baseline requires a source SHA, build and hardware details, raw output, and an explicit threshold-to-case mapping. The coverage runs above must not be presented as performance passes.
- **Run command and binary location:** the root `community-debug` preset enables benchmarks; the root CMake configuration places runtime targets under `build-community-debug/bin/`. See the [benchmark README](../../benchmarks/process/README.md) for a focused command.
- **Resource limits:** Do not use this page to infer bounded latency or resource use. In particular, `ProcessLightRetriever::retrieve()` does not currently enforce its header-declared resource budgets; see [implementation notes](IMPLEMENTATION.md).

## Related sources

- [`benchmarks/process/CMakeLists.txt`](../../benchmarks/process/CMakeLists.txt)
- [Process benchmark catalog](../../benchmarks/process/README.md)
- [Module roadmap](ROADMAP.md) · [Production requirements](PRODUCTION_REQUIREMENTS.md)
