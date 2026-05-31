# PERFORMANCE_EXPECTATIONS - src/geo

## Scope

- Module: src/geo
- This file defines measurable geo module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_geo_cpu_gpu.cpp
  - benchmarks/bench_spatial_index.cpp
  - benchmarks/bench_spatial_join.cpp
  - benchmarks/bench_hybrid_vector_geo.cpp
  - benchmarks/bench_geo_dbscan.cpp
  - benchmarks/bench_geojson_parse.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| GEOP-1 | CPU/GPU batch intersects and exact intersects paths remain within release baseline budgets | BM_GeoCPUExact_BatchIntersects, BM_GeoGPU_BatchIntersects, BM_GeoCPUExact_ExactIntersects, BM_GeoGPU_ExactIntersects |
| GEOP-2 | CPU/GPU geodesic distance and ST_BUFFER paths remain bounded | BM_GeoCPUExact_GeodesicDistance, BM_GeoGPU_GeodesicDistance, BM_GeoCPUExact_StBuffer, BM_GeoGPU_StBuffer |
| GEOP-3 | spatial index build/query paths remain bounded | BM_RTree_BulkLoad, BM_RTree_Intersects, BM_RTree_Contains, BM_RTree_IncrementalInsert |
| GEOP-4 | spatial join paths remain bounded for first-batch and full-pair scenarios | BM_SpatialJoin_First1000, BM_SpatialJoin_AllPairs, BM_SpatialJoin_IndexBuild |
| GEOP-5 | hybrid vector/geo distance/filter and GeoJSON parse paths remain bounded | BM_GeoDistance_Haversine, BM_GeoPointInBoundingBox, BM_VectorGeoFiltering, BM_GeoJSONParse_MultiPolygon_100k |
| GEOP-6 | geo DBSCAN CPU/GPU paths remain bounded under benchmark profile | BM_GeoDBSCAN_CPU, BM_GeoDBSCAN_GPU |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GEOG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| GEOG-2 | geo hot-path p99 <= release threshold | p99 from mapped geo benchmark cases |
| GEOG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional geo benchmark scenarios are introduced.

## Sourcecode Verification (Module: geo/performance)

- Verified benchmark sources:
  - benchmarks/bench_geo_cpu_gpu.cpp
  - benchmarks/bench_spatial_index.cpp
  - benchmarks/bench_spatial_join.cpp
  - benchmarks/bench_hybrid_vector_geo.cpp
  - benchmarks/bench_geo_dbscan.cpp
  - benchmarks/bench_geojson_parse.cpp
- Verified mapping surfaces:
  - geo CPU/GPU intersects/distance/buffer benchmark paths
  - spatial index and spatial join benchmark paths
  - hybrid vector-geo, DBSCAN, and GeoJSON parse benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.