# ThemisDB v1.5.0 — Release Aggregation

**Release Date:** 2026-02-03 (stable) · 2026-02-27 (final dev-alpha)
**Type:** Feature Release
**Previous Version:** v1.4.2
**Milestone:** v1.5.0

---

## 🎯 Overview

ThemisDB v1.5.0 is a major feature release that delivers production-ready GPU kernel sandboxing, comprehensive query-result masking, geo CPU/GPU benchmarks, full GeoJSON RFC 7946 support, advanced search components, shard repair/anti-entropy, write-amplification optimisations, and FAISS quantizer improvements. This document serves as the **official release aggregation** for v1.5.0 and as a template for future release cycles.

---

## 📦 Included Pull Requests

The table below lists all Pull Requests assigned to the **v1.5.0 milestone**, their module ownership, and a short purpose note. PRs are ordered by module.

| PR # | Module | Purpose |
|------|--------|---------|
| [#3049](https://github.com/makr-code/ThemisDB/pull/3049) | **geo** | CPU vs GPU throughput benchmarks (`bench_geo_cpu_gpu.cpp`): measures `batchIntersects`, `exactIntersects`, `geodesicDistance` across CPU-exact, CPU-approximate, and GPU (CUDA/HIP with CPU fallback) backends |
| [#3050](https://github.com/makr-code/ThemisDB/pull/3050) | **security** | `QueryMaskingPolicy`: dynamic field-level PII masking of query results before they reach the client; role-based bypass for privileged roles |
| [#3051](https://github.com/makr-code/ThemisDB/pull/3051) | **gpu** | `WASMKernelSandbox`: isolated execution environment for untrusted third-party GPU kernel blobs; whitelist + FNV-1a checksum gate, memory ceiling, wall-clock timeout |
| [#1383](https://github.com/makr-code/ThemisDB/issues/1383) | **acceleration** | CUDA ANN + geospatial kernels production-ready — batch `intersects` and distance dispatch via `GpuBatchBackend` |
| [#1384](https://github.com/makr-code/ThemisDB/issues/1384) | **acceleration** | Vulkan compute shader pipeline — initial Vulkan backend wired into `ISpatialComputeBackend` |
| [#1390](https://github.com/makr-code/ThemisDB/issues/1390) | **acceleration** | Cross-backend L2 distance consistency validation — regression test suite confirming CPU/GPU numeric parity |

---

## ✨ New Features

### GPU Module — WASM Kernel Sandbox (PR #3051)

> **Files:** `include/themis/gpu/wasm_kernel_sandbox.h`, `src/gpu/wasm_kernel_sandbox.cpp`, `tests/test_gpu_wasm_kernel_sandbox.cpp`

- `WASMKernelSandbox` provides an isolated execution environment for GPU kernel blobs submitted by third-party plugins.
- Full validation pipeline: whitelist/checksum gate → `GPUKernelValidator` → sandbox resource enforcement → backend dispatch.
- `SandboxConfig`: configurable memory ceiling (`max_memory_bytes`), wall-clock timeout (`timeout_ms`), and host-call toggle.
- `ExecutionResult`: structured result with `status`, `elapsed_ms`, and optional error message.
- Feature-gated via `THEMIS_ENABLE_WASM_SANDBOX`; falls back to `GPUKernelValidator`-only mode when WebAssembly runtime is absent.
- Test coverage: `tests/test_gpu_wasm_kernel_sandbox.cpp`.

### Security Module — QueryMaskingPolicy (PR #3050)

> **Files:** `include/security/query_masking_policy.h`, `src/security/query_masking_policy.cpp`, `tests/test_query_masking_policy.cpp`

- `QueryMaskingPolicy` applies field-level masking to JSON query-result objects before they are returned to clients.
- Three complementary strategies: (1) field-name hint masking, (2) auto-detect PII via `PIIDetector`, (3) explicitly declared field paths with `"strict"` / `"partial"` / `"none"` masking modes.
- Role-based bypass: roles listed in `Config::privileged_roles` receive the original unmasked result (default: `"admin"`).
- Thread-safe: all public methods are safe to call concurrently.
- Integrated into `QueryApiHandler` and `http_server.cpp`.
- Test coverage: `tests/test_query_masking_policy.cpp`.

### Geo Module — CPU/GPU Benchmarks (PR #3049)

> **File:** `benchmarks/bench_geo_cpu_gpu.cpp`

- Google Benchmark suite measuring `batchIntersects`, `exactIntersects`, and `geodesicDistance` across:
  - `cpu_exact` backend (Boost.Geometry or pure-C++ exact arithmetic)
  - `cpu_approx` backend (S2-geometry approximate)
  - `gpu_spatial` backend (CUDA/HIP with automatic CPU fallback)
- Batch sizes: 256, 1024, 4096 point-in-polygon pairs.
- Produces baseline CSV for regression tracking under `benchmarks/baselines/`.
- Satisfies Production Readiness Checklist item `[x] Performance benchmarks (CPU vs GPU throughput)` in `src/geo/ROADMAP.md`.

### Geo Module — Full GeoJSON RFC 7946 Support

> **Files:** `include/utils/geo/ewkb.h`, `src/utils/geo/ewkb.cpp`, `include/geo/spatial_backend.h`

- `EWKBParser::parseGeoJSON()` and `toGeoJSON()` now handle all 7 RFC 7946 geometry types: `Point`, `MultiPoint`, `LineString`, `MultiLineString`, `Polygon`, `MultiPolygon`, `GeometryCollection` (including 3D/Z variants).
- `GeometryCollection` parsed recursively up to depth 8 to prevent stack overflow on adversarial input.
- `computeMBR()` and `computeCentroid()` recurse into nested sub-geometries.
- WGS84 coordinate range validation: longitude `[-180, 180]`, latitude `[-90, 90]`; out-of-range values throw `std::runtime_error`. Compile with `-DTHEMIS_GEO_COMPAT_LAX=1` for one-release migration window.

### Geo Module — In-Memory R-Tree Spatial Index

> **Files:** `include/geo/geo_rtree.h`, `src/geo/geo_rtree.cpp`

- `GeoRTree`: in-memory R-tree index for `GeometryInfo` objects enabling sub-linear `intersects` / `contains` queries.
- With `THEMIS_GEO_BOOST_BACKEND`: uses `boost::geometry::index::rtree` with `rstar<16>` splitting strategy.
- Without Boost: falls back to O(n) linear MBR scan — semantically identical, no extra dependency.
- `bulkLoad()`: STR (Sort-Tile-Recursive) packing for 3–5× faster cold-start load versus incremental `insert()`.
- 20 unit tests covering insert, bulk-load, remove, clear, intersects/contains, memory reporting, move semantics.

### Geo Module — ST_UNION and ST_DIFFERENCE

> **Files:** `include/geo/spatial_backend.h`, `include/query/functions/geo_functions.h`, `tests/geo/test_geo_st_union_difference.cpp`, `tests/geo/test_aql_st_functions.cpp`

- `ISpatialComputeBackend::stUnion(geom1, geom2)` and `stDifference(geom1, geom2)` added to the spatial-backend interface.
- `CpuExactBackend`: Greiner-Hormann polygon clipping (ACM TOG 1998) with fast-paths for containment, disjoint, and B-inside-A cases.
- `BoostCpuExactBackend`: delegates to `boost::geometry::union_` / `boost::geometry::difference`.
- `GpuBatchBackend`: CPU fallback with audit log and metrics — same pattern as `stBuffer`.
- AQL functions `ST_UNION` and `ST_DIFFERENCE` registered and return GeoJSON geometry.
- 15 unit tests + 7 AQL-level tests.

### Search Module — 7 New Search Components

> **Files:** `include/search/`

| Component | Purpose |
|-----------|---------|
| `QueryExpander` | Synonym expansion, Levenshtein spelling correction, zero-result relaxation |
| `FuzzyMatcher` | Levenshtein, Soundex, Metaphone, N-gram (Dice) similarity |
| `FacetedSearch` | Per-field value-count facets, numeric range buckets, drill-down filtering |
| `SearchAnalytics` | Thread-safe query log; avg/p95/p99 latency, zero-result rate, top-20 queries |
| `AutocompleteEngine` | Prefix-index + popular-query suggestions, deduplicated and score-ranked |
| `LearningToRank` | Dot-product linear re-ranker; online pairwise gradient-descent training from click events |
| `MultiModalSearch` | TEXT/IMAGE/AUDIO/CUSTOM modalities; weighted RRF fusion |

Combined test coverage: 162+ tests across 7 new test files.

### Sharding Module — Shard Repair / Anti-Entropy Engine

> **File:** `include/sharding/shard_repair_engine.h`

- Background anti-entropy scan: periodic `checkDocumentHealth()` across all shards; degraded documents automatically queued for recovery.
- Repair worker thread drains job queue via `RedundancyStrategy::recoverDocument()` (RAID-5/6 + Mirror modes).
- On-demand triggers with trackable job IDs: `triggerRepair()`, `triggerFullScan()`, `triggerDocumentRepair()`.
- Per-shard `ShardHealthReport`: `HEALTHY` / `DEGRADED` / `FAILED` / `REBUILDING` + scan/repair counters.
- Prometheus metrics forwarded via `ShardingMetricsHandler`.
- Admin API endpoints: `POST /admin/repair`, `POST /admin/repair/scan`, `GET /admin/repair/{job_id}`.

### Sharding Module — Improved Reed-Solomon Erasure Decoder

- Replaced XOR-only parity with **Vandermonde matrix** systematic codec over GF(2⁸).
- Recovers up to `parity_shards` simultaneously lost chunks — enables true RAID-6 dual-parity recovery.
- Both `ReedSolomonCoder` and `CauchyReedSolomonCoder` validate `missing_indices.size() <= parity_shards`.

### Security Module — HSM Security Warning System (FIND-002)

- Startup warning banner when stub HSM provider is active.
- Periodic ERROR-level logging every 5 minutes in insecure configuration.
- Prometheus metrics: `themis_hsm_insecure_config`, `themis_hsm_provider_type`, `hsm_compliance_status`.
- CLI flag `--allow-stub-hsm` for development environments.
- Addresses FIND-002 from v1.4.1 audit (NIST SP 800-53, ISO 27001, PCI DSS, GDPR).

### Index Module — FAISS Quantizer Integration (v1.5.0)

- `ProductQuantizer`: FAISS K-means clustering for 20–30% faster training with SIMD optimisations.
- `BinaryQuantizer`: SIMD-optimised popcount (`__builtin_popcount` / `__popcnt`) for 10–15% faster Hamming distance.
- `ResidualQuantizer`: inherits FAISS acceleration from `ProductQuantizer` stages (30% faster training).
- Graceful fallback to custom implementation when FAISS unavailable.
- `getBackend()` method reports actual backend in use at runtime.

### Security Module — RFC 3161 Timestamp Authority (v1.5.0)

- Full RFC 3161 TSA client with OpenSSL cryptographic operations.
- Integration with FreeTSA, DigiCert, Sectigo TSA providers.
- eIDAS compliance for qualified electronic timestamps; LTV support for 30-year retention.
- CMake option `THEMIS_USE_OPENSSL_TSA` (default: ON); 10+ compliance tests.

### Storage Module — Write-Amplification Optimisation

- Default `memtable_size_mb` 256 MB → 512 MB: ~50% fewer L0 flushes, 30–40% lower write amplification.
- Default `max_write_buffer_number` 3 → 6: fewer write stalls under sustained load.
- `db_write_buffer_size_mb` cap added at 2 GB (was unbounded).
- `enable_async_io` now defaults to `true`; `async_io_readahead_size_mb` 64 MB → 128 MB: 2–5× faster sequential scans.

### Query Module — v1.5.x Distributed Query Optimizer

- `DistributedQueryCostModel::getShardRowCount()`: dynamic row-count estimates replacing hardcoded 10K constant.
- `calculatePredicateSelectivity()`: histogram-based selectivity estimation with column-specific heuristics.
- `measureShardLatency()`: latency integration hook for future network-aware query planning.
- Integration tests: `tests/test_optimizer_v1_5_x_integration.cpp`.

### Index Module — Multi-GPU Vector Indexing API (v2.4 scaffolding)

- `MultiGPUVectorIndex`: logical multi-device API with round-robin/hash/range/balanced partition strategies.
- Query fan-out and centralized top-k merge logic.
- Current execution uses CPU-based `GPUVectorIndex`; actual GPU offload planned for v2.5+.
- Full API guide: `docs/MULTI_GPU_VECTOR_INDEXING.md`.

### Core — Git-Like Features (PITR, Merge, Branch)

- `SnapshotManager` re-enabled: named snapshots via 5 REST endpoints.
- PITR API handler: restore by sequence, tag, or timestamp; dry-run preview.
- `MergeEngine` integrated: 3-way merge with `OURS` / `THEIRS` / `MANUAL` / `FAST_FORWARD` resolution.
- `BranchManager`: non-fast-forward merges via `MergeEngine`; conflict detection.

---

## ⚠️ Breaking Changes

| Area | Change | Migration |
|------|--------|-----------|
| **GeoJSON parsing** | Coordinates outside WGS84 range now throw `std::runtime_error` | Compile with `-DTHEMIS_GEO_COMPAT_LAX=1` for one-release window |
| **GeoJSON unknown types** | Unknown geometry types throw instead of returning empty geometry | Update clients to send valid RFC 7946 geometry types |
| **GPU Vector Index stubs** | `gpu_vector_index_cuda.cpp`, `_vulkan.cpp`, `_hip.cpp` removed from `src/index/` | See `docs/FUTURE_GPU_SUPPORT.md`; use `GPUVectorIndex` (CPU-SIMD) |

---

## 🔒 Security

- RFC 3161 TSA enables legally binding digital signatures (eIDAS).
- `QueryMaskingPolicy` prevents PII exposure in query results.
- `WASMKernelSandbox` isolates untrusted third-party GPU kernel blobs.
- HSM security warning system prevents accidental production deployment with stub provider.

---

## ✅ QA Criteria for Inclusion in v1.5.0

The following criteria must be satisfied for a PR to be included in this release:

| Criterion | Requirement |
|-----------|------------|
| **Feature tests completed** | All new code paths covered by unit and/or integration tests; no failing tests in CI |
| **Compatibility with v1.4.x** | No breaking changes beyond those listed in the Breaking Changes table; all existing API contracts maintained |
| **Documentation present** | Header-level Doxygen/comment blocks; release notes entry in CHANGELOG.md; module ROADMAP.md updated |
| **Migration guide available** | For every breaking change a migration note exists in CHANGELOG.md and this document |
| **Security review** | Security-sensitive features (crypto, masking, sandboxing) reviewed and signed off |
| **Performance baseline** | Performance-critical features benchmarked and baseline stored under `benchmarks/baselines/` |
| **Roadmap item linkage** | Every PR is traced to at least one roadmap item (`roadmap.md` or module `ROADMAP.md`) |

---

## 📋 Roadmap Traceability

| Roadmap Item | Status | PR / Issue |
|---|---|---|
| CUDA ANN + geospatial kernels production-ready | [P] | #1383 |
| Vulkan compute shader pipeline | [P] | #1384 |
| Cross-backend L2 distance consistency validation | [P] | #1390 |
| Geo CPU/GPU throughput benchmarks | [x] | #3049 |
| QueryMaskingPolicy for PII field masking | [x] | #3050 |
| WASM GPU Kernel Sandbox for third-party kernels | [x] | #3051 |
| Full GeoJSON RFC 7946 parsing | [x] | Unreleased → v1.5.0 |
| In-memory R-tree spatial index | [x] | Unreleased → v1.5.0 |
| ST_UNION / ST_DIFFERENCE geometry operations | [x] | Unreleased → v1.5.0 |
| Shard repair / anti-entropy engine | [x] | Unreleased → v1.5.0 |
| Write-amplification optimisation (RocksDB) | [x] | Unreleased → v1.5.0 |
| RFC 3161 Timestamp Authority | [x] | v1.5.0 |
| FAISS Quantizer Integration | [x] | v1.5.0 |

---

## 🔗 Related Documentation

- [CHANGELOG.md](../../../CHANGELOG.md) — Full change log
- [roadmap.md](../../../roadmap.md) — Top-level project roadmap
- [src/geo/ROADMAP.md](../../../src/geo/ROADMAP.md) — Geo module roadmap
- [src/gpu/ROADMAP.md](../../../src/gpu/ROADMAP.md) — GPU module roadmap
- [src/security/ROADMAP.md](../../../src/security/ROADMAP.md) — Security module roadmap
- [docs/FUTURE_GPU_SUPPORT.md](../../FUTURE_GPU_SUPPORT.md) — GPU support v2.x roadmap
- [docs/GPU_MASTER_TRACKING.md](../../GPU_MASTER_TRACKING.md) — GPU implementation tracking

---

*This document was produced as part of the v1.5.0 Release Aggregation (Issue: makr-code/ThemisDB#3070) and serves as a template for future release cycles.*
