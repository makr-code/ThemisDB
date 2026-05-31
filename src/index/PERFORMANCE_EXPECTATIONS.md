# PERFORMANCE_EXPECTATIONS - src/index

## Scope

- Module: src/index
- This file defines measurable index module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_vector_search.cpp
  - benchmarks/bench_gpu_vector_index.cpp
  - benchmarks/bench_index_rebuild.cpp
  - benchmarks/bench_spatial_index.cpp
  - benchmarks/bench_binary_quantization.cpp
  - benchmarks/bench_approximate_radius_search.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| IDXP-1 | core vector search and insert throughput paths remain bounded | BM_VectorSearch_efSearch, BM_VectorInsert_Batch100, BM_L2Distance_1000_512, BM_CosineDistance_1000_512, BM_TopK_5000_50 |
| IDXP-2 | GPU vector index build and search pathways remain bounded | BM_IndexBuild_CPU, BM_IndexBuild_VULKAN, BM_Search_CPU, BM_Search_VULKAN, BM_BatchSearch_CPU, BM_BatchSearch_VULKAN, BM_ANN_ExplicitVulkanPath, BM_DistanceMetric_L2, BM_DistanceMetric_Cosine |
| IDXP-3 | secondary/fulltext/range/sparse index rebuild paths remain bounded | RebuildFixture/Rebuild_Regular_Email, RebuildFixture/Rebuild_Composite_CityAge, RebuildFixture/Rebuild_Range_Salary, RebuildFixture/Rebuild_Sparse_Nickname, RebuildFixture/Rebuild_TTL_ExpiresAt, RebuildFixture/Rebuild_Fulltext_Bio, RebuildFixture/ReindexEntireTable |
| IDXP-4 | spatial index build/query behavior remains bounded against linear baseline | BM_RTree_BulkLoad, BM_RTree_Intersects, BM_LinearScan_Intersects, BM_RTree_Contains, BM_RTree_IncrementalInsert |
| IDXP-5 | binary quantization pipelines remain bounded for encode/decode/distance workloads | BM_BinaryQuant_Training, BM_BinaryQuant_Encode, BM_BinaryQuant_BatchEncode, BM_BinaryQuant_Decode, BM_BinaryQuant_HammingDistance, BM_BinaryQuant_BatchHammingDistance, BM_BinaryQuant_AsymmetricDistance, BM_BinaryQuant_CompressionRatio, BM_BinaryQuant_EndToEnd |
| IDXP-6 | approximate radius search behavior remains bounded across dataset and target variations | BM_RadiusSearch_DatasetSize, BM_RadiusSearch_RadiusVariation, BM_RadiusSearch_BatchSearch, BM_RadiusSearch_TargetCount, BM_RadiusSearch_EstimateCount, BM_RadiusSearch_SearchById, BM_RadiusSearch_Metrics |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| IDXG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| IDXG-2 | index hot-path p99 <= release threshold | p99 from mapped index benchmark cases |
| IDXG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional index benchmark scenarios are introduced.

## Sourcecode Verification (Module: index/performance)

- Verified benchmark sources:
  - benchmarks/bench_vector_search.cpp
  - benchmarks/bench_gpu_vector_index.cpp
  - benchmarks/bench_index_rebuild.cpp
  - benchmarks/bench_spatial_index.cpp
  - benchmarks/bench_binary_quantization.cpp
  - benchmarks/bench_approximate_radius_search.cpp
- Verified mapping surfaces:
  - vector and GPU-vector search/build paths
  - rebuild and spatial index paths
  - quantization and radius-search paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.