---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-20
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - ../../benchmarks/graph/
  - ../../benchmarks/tensor/bench_tensor_fingerprint_graph.cpp
  - ../../benchmarks/CMakeLists.txt
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# PERFORMANCE_EXPECTATIONS - src/graph

> The target IDs and hard-gate rows below are retained from an older documentation baseline.
> They are expectations, not evidence that a benchmark was run or that a release threshold is
> currently configured. Benchmark case mappings are corrected to current sources below.

## Scope

- Module: src/graph
- This file defines measurable graph module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - `benchmarks/graph/bench_graph_traversal.cpp`
  - `benchmarks/graph/bench_graph_query_optimizer.cpp`
  - `benchmarks/graph/bench_graph_dedicated_gates.cpp`
  - `benchmarks/graph/bench_graph_release_gates.cpp`
  - `benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| GRP-1 | baseline graph traversal operations remain bounded across sparse/dense inputs | GraphTraversalBenchmarkFixture/BFSTraversal, GraphTraversalBenchmarkFixture/DFSTraversal, GraphTraversalBenchmarkFixture/SparseEdgeAddition, GraphTraversalBenchmarkFixture/DenseNeighborQuery |
| GRP-2 | advanced traversal and analytics helper paths remain bounded | GraphTraversalBenchmarkFixture/ShortestPathTraversal, GraphTraversalBenchmarkFixture/DegreeCentrality, GraphTraversalBenchmarkFixture/ConnectedComponents, GraphTraversalBenchmarkFixture/DiameterEstimation |
| GRP-3.1 | in-process BFS fixture p95 latency <= 100 us | BM_GraphBFS_P95 |
| GRP-3.2 | in-process Dijkstra fixture p95 latency <= 200 us | BM_GraphDijkstra_P95 |
| GRP-3.3 | four-worker in-process BFS throughput >= 5000 ops/s; items_per_second = ops/s, where one item is one traversal | BM_GraphParallelBFS_Throughput |
| GRP-3.4 | in-process constrained BFS fixture p99 latency <= 200 us | BM_GraphConstrainedBFS_P99 |
| GRP-4 | optimizer plan generation and cached planning remain bounded | GraphQueryOptimizerBenchmarkFixture/PlanGeneration_ShortestPath, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_KHopNeighborhood, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_WithCache |
| GRP-5 | optimizer execution algorithms remain bounded | GraphQueryOptimizerBenchmarkFixture/BFS_Execution, GraphQueryOptimizerBenchmarkFixture/DFS_Execution, GraphQueryOptimizerBenchmarkFixture/Dijkstra_Execution, GraphQueryOptimizerBenchmarkFixture/Bidirectional_Execution, GraphQueryOptimizerBenchmarkFixture/Statistics_Collection |
| GRP-6 | parallel and incremental optimizer traversal paths remain bounded | `GraphQueryOptimizerBenchmarkFixture/MultiSourceBFS`, `GraphQueryOptimizerBenchmarkFixture/MultiSourceDFS`, `GraphQueryOptimizerBenchmarkFixture/MultiSourceBFS_ThreadScaling`, `GraphQueryOptimizerBenchmarkFixture/IncrementalBFS_OnGraphChange`, `GraphQueryOptimizerBenchmarkFixture/IncrementalBFS_MultiQuery_FanOut` |
| GRP-7 | tensor fingerprint graph insert/find/neighbor/control paths remain bounded | `benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`: `BM_TFG_Insert_Throughput`, `BM_TFG_Insert_SingleNode`, `BM_TFG_FindSimilar`, `BM_TFG_Neighbours`, `BM_TFG_ConcurrentReads`, `BM_TFG_NodeCount`, `BM_TFG_ExportPersistedGraph` |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GRG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| GRG-2 | graph hot-path p99 <= release threshold | p99 from mapped graph benchmark cases |
| GRG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional graph benchmark scenarios are introduced.
- `benchmarks/graph/CMakeLists.txt` uses plain `file(GLOB)` without `CONFIGURE_DEPENDS`.
  Adding/removing matching benchmark files may not trigger CMake regeneration; rerun configure
  before building the updated inventory.

The dedicated gates use Google Benchmark JSON cases: `BM_GraphBFS_P95` (p95 ≤ 100 µs),
`BM_GraphDijkstra_P95` (p95 ≤ 200 µs), `BM_GraphParallelBFS_Throughput` (≥ 5000 ops/s),
and `BM_GraphConstrainedBFS_P99` (p99 ≤ 200 µs). They measure deterministic in-process graph
fixtures, not the production graph engine. Percentile counters are emitted as `p95_real_time`
or `p99_real_time`; the throughput case counts one processed item per traversal.

## Sourcecode Verification (Module: graph/performance)

- Verified benchmark sources:
  - `benchmarks/graph/bench_graph_traversal.cpp`
  - `benchmarks/graph/bench_graph_query_optimizer.cpp`
  - `benchmarks/graph/bench_graph_dedicated_gates.cpp`
  - `benchmarks/graph/bench_graph_release_gates.cpp`
  - `benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`
- Verified mapping surfaces:
  - traversal and optimizer plan/execution paths
  - parallel/incremental traversal and thread-scaling paths
  - tensor fingerprint graph insertion/similarity/adjacency/export paths
- Result:
  - Case mappings above were checked against current Google Benchmark declarations.
  - The focused CTest benchmark and its setup/postprocess fixtures passed on 2026-11-20.
    This is execution evidence for the fixture only, not a production-engine result.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
