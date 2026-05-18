# Vulkan Reference Pipeline Plan (ANN + LLM) - 2026-05-17

## Goal
Build a reference Vulkan-oriented benchmark pipeline analogous to LoRA runtime validation, starting with ANN/GPU vector index and adding explicit backend evidence in benchmark output.

## Scope
- Update `benchmarks/bench_gpu_vector_index.cpp` to:
  - re-enable Vulkan benchmark paths (instead of CPU-only comment blocks)
  - keep CPU baselines
  - add backend evidence counters/labels (`active_backend`, `gpu_active`)
  - fail-safe skip when Vulkan backend cannot initialize
- Run targeted benchmark build + smoke execution.
- Update `PERFORMANCE_EXPECTATIONS.md` with the new reference-pipeline status and measured CPU-vs-Vulkan comparison if available.

## Acceptance Criteria
1. Benchmark target compiles on `windows-bench-release`.
2. `bench_gpu_vector_index --benchmark_list_tests` contains Vulkan benchmark entries.
3. Vulkan benchmarks either:
   - run and emit backend evidence (`active_backend=VULKAN`, `gpu_active=1`), or
   - skip with explicit reason (no silent fallback).
4. Performance doc is updated with current result and audit status.

## Validation
- Build: `cmake --build --preset windows-bench-release --target bench_gpu_vector_index --parallel 4`
- List tests: `bench_gpu_vector_index.exe --benchmark_list_tests=true`
- Smoke: filtered run of CPU + Vulkan search benchmarks with JSON output.
