---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - CMakeLists.txt
  - ../../CMakePresets.json
  - ../../benchmarks/CMakeLists.txt
  - ../../cmake/CMakeLists.txt
  - ../../src/graph/PERFORMANCE_EXPECTATIONS.md
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# benchmarks/graph

This directory currently contains six benchmark sources:
`bench_gnn_embeddings.cpp`, `bench_graph_dedicated_gates.cpp`,
`bench_graph_query_optimizer.cpp`, `bench_graph_release_gates.cpp`,
`bench_graph_traversal.cpp`, and `bench_pagerank.cpp`.

The local [`CMakeLists.txt`](CMakeLists.txt) is gated by `THEMIS_BUILD_BENCHMARKS` and uses plain
`file(GLOB)` to discover `bench_*.cpp` for `themis_add_standard_benchmark` (without
`CONFIGURE_DEPENDS`). The parent
[`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt) also has explicit graph targets;
the effective target registration should be checked there. A benchmark source or target does not
prove that a benchmark ran or met a threshold.

`bench_graph_dedicated_gates.cpp` defines four source-level measurements: GR-BM-01 BFS p95
(≤100 µs), GR-BM-02 Dijkstra p95 (≤200 µs), GR-BM-03 parallel BFS throughput (≥5,000
traversals/s), and GR-BM-04 constrained-query p99 (≤200 µs). The source prints PASS/WARN text
against these gates; those thresholds and messages are not evidence of a passing run.

Because the file list is obtained by a plain glob, adding/removing a matching benchmark source
may not trigger CMake regeneration automatically. Re-run CMake configure for the selected build
directory/preset before building the updated benchmark inventory.

## Configure and run

The `community-debug` preset enables benchmarks, but inherits `THEMIS_BUILD_TESTS=OFF`. For a
benchmark-only configuration (use a fresh binary directory when a clean configure is required),
make the benchmark option explicit:

```bash
cmake --preset community-debug -DTHEMIS_BUILD_TESTS=OFF -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset community-debug --target bench_graph_dedicated_gates
ctest --preset community-debug -R "^bench_graph_dedicated_gates$" --output-on-failure
```

The preset requires Linux/macOS, GCC or Clang, Ninja, and its system development packages (see
[`SETUP.md`](../../SETUP.md)). Benchmark targets require the Google Benchmark CMake package;
without it, `benchmarks/CMakeLists.txt` warns and returns before registering targets. The
benchmark CTest post-processing registration also requires a Python 3 interpreter. These are
configuration/run instructions, not evidence that the gate passed.

See [`src/graph/PERFORMANCE_EXPECTATIONS.md`](../../src/graph/PERFORMANCE_EXPECTATIONS.md) for
the source map and measurement requirements.
