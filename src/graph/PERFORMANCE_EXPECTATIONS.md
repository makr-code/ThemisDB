# PERFORMANCE_EXPECTATIONS - src/graph

## Scope

- Module: src/graph
- This file defines measurable graph module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_graph_traversal.cpp
  - benchmarks/bench_graph_query_optimizer.cpp
  - benchmarks/bench_tensor_fingerprint_graph.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| GRP-1 | baseline graph traversal operations remain bounded across sparse/dense inputs | GraphTraversalBenchmarkFixture/BFSTraversal, GraphTraversalBenchmarkFixture/DFSTraversal, GraphTraversalBenchmarkFixture/SparseEdgeAddition, GraphTraversalBenchmarkFixture/DenseNeighborQuery |
| GRP-2 | advanced traversal and analytics helper paths remain bounded | GraphTraversalBenchmarkFixture/ShortestPathTraversal, GraphTraversalBenchmarkFixture/DegreeCentrality, GraphTraversalBenchmarkFixture/ConnectedComponents, GraphTraversalBenchmarkFixture/DiameterEstimation |
| GRP-3 | general traversal direction/depth/result-shape paths remain bounded | GeneralTraversalBenchmarkFixture/GeneralTraversalOutbound, GeneralTraversalBenchmarkFixture/GeneralTraversalInbound, GeneralTraversalBenchmarkFixture/GeneralTraversalAny, GeneralTraversalBenchmarkFixture/GeneralTraversalDepthFilter, GeneralTraversalBenchmarkFixture/GeneralTraversalLargeResults |
| GRP-4 | optimizer plan generation and cached planning remain bounded | GraphQueryOptimizerBenchmarkFixture/PlanGeneration_ShortestPath, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_KHopNeighborhood, GraphQueryOptimizerBenchmarkFixture/PlanGeneration_WithCache |
| GRP-5 | optimizer execution algorithms remain bounded | GraphQueryOptimizerBenchmarkFixture/BFS_Execution, GraphQueryOptimizerBenchmarkFixture/DFS_Execution, GraphQueryOptimizerBenchmarkFixture/Dijkstra_Execution, GraphQueryOptimizerBenchmarkFixture/Bidirectional_Execution, GraphQueryOptimizerBenchmarkFixture/Statistics_Collection |
| GRP-6 | parallel and incremental optimizer traversal paths remain bounded | GraphQueryOptimizerBenchmarkFixture/MultiSourceBFS, GraphQueryOptimizerBenchmarkFixture/MultiSourceDFS, GraphQueryOptimizerBenchmarkFixture/MultiSourceBFS_ThreadScaling, GraphQueryOptimizerBenchmarkFixture/IncrementalBFS_OnGraphChange, GraphQueryOptimizerBenchmarkFixture/IncrementalBFS_MultiQuery_FanOut, ParallelTraversalBenchmarkFixture/MultiSourceBFS_Sequential, ParallelTraversalBenchmarkFixture/MultiSourceBFS_FanOutParallel, ParallelTraversalBenchmarkFixture/MultiSourceDFS |
| GRP-7 | tensor fingerprint graph insert/find/neighbor/control paths remain bounded | BM_TFG_Insert_Throughput, BM_TFG_Insert_SingleNode, BM_TFG_FindSimilar, BM_TFG_Neighbours, BM_TFG_ConcurrentReads, BM_TFG_NodeCount, BM_TFG_ExportPersistedGraph |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GRG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| GRG-2 | graph hot-path p99 <= release threshold | p99 from mapped graph benchmark cases |
| GRG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional graph benchmark scenarios are introduced.

## Sourcecode Verification (Module: graph/performance)

- Verified benchmark sources:
  - benchmarks/bench_graph_traversal.cpp
  - benchmarks/bench_graph_query_optimizer.cpp
  - benchmarks/bench_tensor_fingerprint_graph.cpp
- Verified mapping surfaces:
  - traversal and optimizer plan/execution paths
  - parallel/incremental traversal and thread-scaling paths
  - tensor fingerprint graph insertion/similarity/adjacency/export paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.