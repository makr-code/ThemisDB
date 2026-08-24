# GPU Vector Index Vulkan Tuning Plan (2026-05-17)

## Context
- Current benchmark evidence on RTX 3060 shows strong Vulkan gains for BatchSearch but regressions for Search and IndexBuild.
- Goal is a first optimization iteration with measurable impact and no behavior regression.

## Scope
- `src/index/gpu_vector_index_vulkan.cpp`
- `benchmarks/bench_gpu_vector_index.cpp`
- `PERFORMANCE_EXPECTATIONS.md` (results update if metrics change)

## Implementation Steps
1. Optimize Vulkan upload path in `uploadVectors` to avoid repeated per-vector dynamic appends.
2. Optimize Vulkan query flattening path in `searchBatchIndices` similarly.
3. Adjust IndexBuild benchmark timing window to measure build/upload work separately from backend initialization/shutdown overhead.
4. Rebuild benchmark target and rerun focused Search/IndexBuild CPU-vs-Vulkan benchmark.
5. Record updated deltas in performance expectations.

## Acceptance Criteria
- Benchmark target builds successfully.
- Focus benchmark runs with Vulkan backend active and produces JSON.
- Search/IndexBuild deltas are reproducibly updated in docs.
- No fallback/stub path is silently enabled.

## Test Scope
- Build target: `bench_gpu_vector_index`
- Run: focused benchmark (`BM_Search_(CPU|VULKAN)`, `BM_IndexBuild_(CPU|VULKAN)`)
