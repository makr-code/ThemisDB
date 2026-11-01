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
| GRP-3 | dedicated traversal and constrained-query latency/throughput gates | `bench_graph_dedicated_gates.cpp`: GR-BM-01 BFS p95, GR-BM-02 Dijkstra p95, GR-BM-03 parallel BFS throughput, GR-BM-04 constrained-query p99 |
| GRP-4 | optimizer plan generation and cached planning remain bounded | GraphQueryOptimizerBenchmarkFixture/PlanGeneration_ShortestPath, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_KHopNeighborhood, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_WithCache |
| GRP-5 | optimizer execution algorithms remain bounded | GraphQueryOptimizerBenchmarkFixture/BFS_Execution, GraphQueryOptimizerBenchmarkFixture/DFS_Execution, GraphQueryOptimizerBenchmarkFixture/Dijkstra_Execution, GraphQueryOptimizerBenchmarkFixture/Bidirectional_Execution, GraphQueryOptimizerBenchmarkFixture/Statistics_Collection |
| GRP-6 | parallel and incremental optimizer traversal paths remain bounded | `bench_graph_query_optimizer.cpp`: `MultiSourceBFS`, `MultiSourceDFS`, `MultiSourceBFS_ThreadScaling`, `IncrementalBFS_OnGraphChange`, `IncrementalBFS_MultiQuery_FanOut` |
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

The source-defined dedicated gates are GR-BM-01 (BFS p95 ≤ 100 µs), GR-BM-02 (Dijkstra p95
≤ 200 µs), GR-BM-03 (parallel BFS throughput ≥ 5,000 traversals/s), and GR-BM-04 (constrained
query p99 ≤ 200 µs). These thresholds do not state that a run passed.

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
  - Case mappings above were checked against current benchmark source declarations.
  - No benchmark was run for this documentation update; no pass or baseline claim is made.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
