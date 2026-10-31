# Geo performance benchmark map

## Scope

This page maps benchmark sources for the integrated geo implementation; it does
not report measured performance or declare that a target has passed. Benchmark
sources are under `benchmarks/geo/`. The
optional external Geo plugin build was not benchmark-audited here.

## Registered benchmark sources

| Source | Cases / scope | Important interpretation |
|---|---|---|
| `bench_geo_cpu_gpu.cpp` | CPU exact/approximate and GPU backend `batchIntersects`, `exactIntersects`, geodesic distance, and buffer cases | GPU cases skip if the backend is unavailable. Run output alone must be checked to determine whether GPU execution occurred. |
| `bench_geo_cuda_kernel_gating.cpp` | CUDA-gated Haversine, point-in-polygon, and Vincenty batch calls, plus CPU baseline cases | GPU cases skip when CUDA is unavailable. The current `computeVincentyBatch()` implementation reports CPU fallback, so its benchmark is not evidence of GPU Vincenty execution. The source labels are targets, not a result comparison. |
| `bench_geo_release_gates.cpp` | `BM_GRG01_PointInPolygon`, `BM_GRG02_BboxQuery`, `BM_GRG03_GeoJsonParse`, `BM_GRG04_HaversineDistance`, `BM_GRG05_SpatialJoin`, `BM_GRG06_BackendSelection` | These are deterministic, CPU-only mock implementations, not production geo-path measurements. Their source labels describe p99 target values; they do not prove a target passed. |
| `bench_spatial_index.cpp` | `BM_RTree_BulkLoad`, `BM_RTree_Intersects`, `BM_RTree_Contains`, `BM_RTree_IncrementalInsert`, plus a linear-scan comparison | Uses `GeoRTree`; input sizes and complexity settings are declared in the benchmark source. |
| `bench_spatial_join.cpp` | `BM_SpatialJoin_First1000`, `BM_SpatialJoin_AllPairs`, `BM_SpatialJoin_IndexBuild` | Uses the production join API; benchmark comments describe fixture sizes and caps. The fixture/threshold is not a universal service-level objective. |
| `bench_geo_dbscan.cpp` | `BM_GeoDBSCAN_CPU`, `BM_GeoDBSCAN_GPU` | The GPU configuration requests GPU use, but the result must be inspected to distinguish GPU execution from fallback. |
| `bench_geojson_parse.cpp` | `BM_GeoJSONParse_MultiPolygon_100k` | Builds and validates typed geometry from generated GeoJSON; it is not the same workload as the minimal parser mock in the release-gate file. |

`benchmarks/geo/CMakeLists.txt` globs `bench_*.cpp` when
`THEMIS_BUILD_BENCHMARKS` is enabled and Google Benchmark is available. The
spatial-index and spatial-join sources are excluded there because they retain
their existing top-level benchmark target registration. Registration is not
evidence that a benchmark was built or run.

## Interpreting results

For comparisons, record the repository revision, build profile/compiler,
hardware, relevant geo build flags, benchmark arguments, selected backend, and
the complete run output/manifest. Compare the same case and setup against its
baseline. A configured target, benchmark label, or CPU fallback is not a
measurement of GPU performance.

There is no current benchmark source in this directory for `GeoFaissKnn`.
`src/geo/EMBEDDING.md` describes a candidate
accuracy/latency test plan; treat it as a proposal, not measured evidence.

## Source references

- Benchmark definitions and target registration:
  `benchmarks/geo/`.
- Geo benchmark CMake:
  `benchmarks/geo/CMakeLists.txt`.
- Build and runtime configuration:
  `cmake/features/GeoFeatures.cmake` and `src/geo/CMakeLists.txt`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
