# PERFORMANCE_EXPECTATIONS - src/performance

## Scope

- Module: src/performance
- This file defines measurable performance module expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_core_performance.cpp
  - benchmarks/bench_storage_performance.cpp
  - benchmarks/bench_sharding_performance.cpp
  - benchmarks/bench_olap_performance.cpp
  - benchmarks/bench_mixed_precision_perf.cpp
  - benchmarks/bench_embedding_cache_performance.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| PRFP-1 | core SIMD/index performance paths remain bounded | SIMDDistanceThroughput_PERFD3, SecondaryIndexBench/BM_SecondaryIndex_BatchInsert |
| PRFP-2 | storage allocator/memory/RCU and sustained-write paths remain bounded | BM_Allocator_System_Small, BM_Allocator_Themis_Small, BM_Allocator_System_Large, BM_Allocator_Themis_Large, BM_Allocator_Mixed, BM_Memory_RegularPages_Sequential, BM_Memory_HugePages_Sequential, BM_Memory_RegularPages_Random, BM_Memory_HugePages_Random, BM_RCU_Read_SingleThread, BM_RCU_Write_WithSync, BM_RCU_Read_MultiThread, BM_Storage_SustainedWrite_NoSync, BM_Storage_SustainedWrite_Batched, BM_Storage_SustainedWrite_FullSync, BM_WAL_GroupCommit_Batch |
| PRFP-3 | sharding and distributed performance-related workflows remain bounded | ScatterGatherFixture/ScatterGatherLatency, CrossShardJoinFixture/BroadcastHashJoin, CrossShardJoinFixture/CoLocatedJoinSimulation, BM_PercolatorCommitLatency, RebalancingFixture/BatchSerializationThroughput, RebalancingFixture/BatchDeserializationThroughput, RebalancingFixture/WriteLatencyDuringMigration, RebalancingFixture/RebalancerDecisionCycle, RebalancingFixture/AntiEntropyScanThroughput, RebalancingFixture/GpuReedSolomonThroughput, RebalancingFixture/SnapshotTransfer1GB, RebalancingFixture/SnapshotCompressionRatioZstdL3, RebalancingFixture/ReplicaCatchupThroughput, GossipOverheadFixture/MessageSerialization, GossipOverheadFixture/FanoutSelection, GossipOverheadFixture/VersionVectorMerge, GossipOverheadFixture/TopologyPropagation100Nodes, MultiDCRoutingFixture/DCProximityRouting, MultiDCRoutingFixture/CrossDCLatencySimulation, BM_ConcurrentShardAccess |
| PRFP-4 | OLAP, mixed-precision, and embedding-cache performance paths remain bounded | BM_OLAP_Count, BM_OLAP_Sum, BM_OLAP_Avg, BM_OLAP_MinMax, BM_OLAP_GroupBy_SingleDim, BM_OLAP_GroupBy_TwoDim, BM_OLAP_GroupBy_ThreeDim, BM_OLAP_Filter_Equality, BM_OLAP_Filter_Range, BM_OLAP_Filter_Complex, BM_OLAP_Sort_SingleColumn, BM_OLAP_Sort_MultiColumn, BM_OLAP_Sum_Optimized, BM_OLAP_GroupBy_Optimized, BM_OLAP_ComplexQuery, BM_MixedPrecision_GPUDisabled, BM_Training_FP32, BM_Training_FP16, BM_Memory_FP32_vs_FP16, BM_TensorCore_Speedup, BM_EmbeddingCache_Store_NoIndex, BM_EmbeddingCache_Store_WithIndex, BM_EmbeddingCache_Query_NoIndex, BM_EmbeddingCache_Query_WithIndex, BM_EmbeddingCache_Query_Miss, BM_EmbeddingCache_BatchStore, BM_EmbeddingCache_BatchQuery, BM_EmbeddingCache_Eviction, BM_EmbeddingCache_SimilarityThreshold, BM_EmbeddingCache_MemoryUsage, BM_EmbeddingCache_CostSavings |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| PRFG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| PRFG-2 | performance hot-path p99 <= release threshold | p99 from mapped performance benchmark cases |
| PRFG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional performance benchmark scenarios are introduced.

## Sourcecode Verification (Module: performance/performance)

- Verified benchmark sources:
  - benchmarks/bench_core_performance.cpp
  - benchmarks/bench_storage_performance.cpp
  - benchmarks/bench_sharding_performance.cpp
  - benchmarks/bench_olap_performance.cpp
  - benchmarks/bench_mixed_precision_perf.cpp
  - benchmarks/bench_embedding_cache_performance.cpp
- Verified mapping surfaces:
  - core, storage, sharding, OLAP, mixed-precision, and embedding-cache paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.