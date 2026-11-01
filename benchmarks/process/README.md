---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
Source_Level: level 1
SOT_Domain: module-behavior
Canonical_Sources: benchmarks/process/CMakeLists.txt; benchmarks/process/*.cpp; src/process/PERFORMANCE_EXPECTATIONS.md; DOCUMENTATION_GOVERNANCE.md
Related_Issue: "#7368"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

# Process Benchmark Registration

**AI provenance:** Exact model backend not verifiable from final document text alone.

This page describes CMake registration, not benchmark validity, performance, or pass status. Previous versions included stale gate totals, target names, commands, thresholds, and validation claims; none are asserted as current evidence.

## Sources registered for the process module

[`CMakeLists.txt`](CMakeLists.txt) calls `themis_add_standard_benchmark()` for the following sources when `THEMIS_BUILD_BENCHMARKS` is enabled:

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

Other `bench_*.cpp` files in this directory are not automatically registered by this file unless referenced by another CMake path. In particular, do not infer module registration from a source file's presence.

## Interpretation cautions

- Gate labels and benchmark names in individual `.cpp` files must be checked against the code; older README tables do not define the current registry.
- A benchmark may use simulation, mocks, or simplified operations. Inspect its body before interpreting timings as production implementation performance.
- Source registration and benchmark compilation do not demonstrate a passing threshold or release baseline.
- The registered CMake target names match the source stems listed above. The `community-debug` preset enables benchmarks and uses system packages; Google Benchmark and the other configured dependencies must be available.

  ```sh
  cmake --preset community-debug
  cmake --build --preset community-debug --target bench_process_parser_gates
  ./build-community-debug/bin/bench_process_parser_gates --benchmark_list_tests=true
  ```

- The Process performance comparator currently discovers zero expectation rows because `src/process/PERFORMANCE_EXPECTATIONS.md` has no `## Specific Expectations` section. The four PM-BM numeric targets in `bench_process_dedicated_gates.cpp` are comments over in-process stubs, not enforced performance thresholds.
- The separate release coverage audit requires 95% CTest registration for non-critical modules. Process was mapped at 14/14 (100%) in both the latest audit report and the preceding successful workflow run; these are coverage results, not benchmark execution.
- The tracked mining JSON is a 2026-04-15 historical sample, not a current baseline or performance pass. The latest visible weekly performance run failed during configure before benchmark execution. See [performance evidence](../../src/process/PERFORMANCE_EXPECTATIONS.md) for run links and scope.
- The process source `process_light_retriever.cpp` currently leaves resource counters at zero and does not enforce the header-declared retrieval budgets; benchmark results must not be used to imply those guarantees.

## Related documentation

- [`src/process/PERFORMANCE_EXPECTATIONS.md`](../../src/process/PERFORMANCE_EXPECTATIONS.md)
- [`tests/process/CMakeLists.txt`](../../tests/process/CMakeLists.txt)
- [`src/process/ROADMAP.md`](../../src/process/ROADMAP.md)
