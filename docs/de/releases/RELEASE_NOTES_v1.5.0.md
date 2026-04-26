# ThemisDB v1.5.0 — Release Aggregation

**Release Date:** 2026-02-03 (stable) · 2026-03-03 (aggregation updated)
**Type:** Feature Release
**Previous Version:** v1.4.1-dev-alpha
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
| [#3420](https://github.com/makr-code/ThemisDB/pull/3420) | **updates** | Update history log: persistent audit trail of who applied which update, from/to version, and timestamp |
| [#3421](https://github.com/makr-code/ThemisDB/pull/3421) | **updates** | Blue/green deployment support (Phase 4): zero-downtime major upgrades via traffic routing and automatic rollback |
| [#3430](https://github.com/makr-code/ThemisDB/pull/3430) | **updates** | fix: repair `hot_reload_engine.h` doc-comment syntax; mark blue/green deployment complete in module ROADMAP |
| [#3422](https://github.com/makr-code/ThemisDB/pull/3422) | **replication / updates** | `CoordinatedUpdateManager`: multi-node replication-safe update sequencing ensuring all replicas apply updates in consensus order |
| [#3424](https://github.com/makr-code/ThemisDB/pull/3424) | **chimera** | CI benchmark baseline: `run_ci_benchmarks` integration tests seed repeatable benchmark baselines for the Chimera adapter layer |
| [#3425](https://github.com/makr-code/ThemisDB/pull/3425) | **gpu** | Multi-node GPU cluster coordination marked production-ready; device management and topology discovery hardened |
| [#3426](https://github.com/makr-code/ThemisDB/pull/3426) | **performance** | Memory pressure monitor (Phase 3): wired into build system with dedicated test suite; triggers GC / eviction under configurable pressure thresholds |
| [#3427](https://github.com/makr-code/ThemisDB/pull/3427) | **query** | Per-query resource limits (`max_rows`, `max_memory_bytes`, `timeout_ms`) exposed via HTTP API and enforced at execution time |
| [#3428](https://github.com/makr-code/ThemisDB/pull/3428) | **replication** | CRDT library expansion: `FLAG_EW` (Enable-Wins Flag) and `FLAG_DW` (Disable-Wins Flag) types added alongside existing set types |
| [#3434](https://github.com/makr-code/ThemisDB/pull/3434) | **voice** | Real-time meeting transcription with action-item extraction: integrates STT pipeline with meeting-session management and NLP action extraction |
| [#3435](https://github.com/makr-code/ThemisDB/pull/3435) | **performance** | PMU cache-miss analysis: CPU performance-counter instrumentation wired into build system and test suite |
| [#3437](https://github.com/makr-code/ThemisDB/pull/3437) | **performance / ci** | Cross-module performance regression detection in CI: automated comparison of benchmark results against stored baselines |
| [#3438](https://github.com/makr-code/ThemisDB/pull/3438) | **security / updates** | HSM-backed `SigningService` for update bundle signing: replaces soft-key signing path with PKCS#11 hardware-backed operations |
| [#3439](https://github.com/makr-code/ThemisDB/pull/3439) | **voice** | Voice module unit-test coverage raised above 80% target |
| [#3442](https://github.com/makr-code/ThemisDB/pull/3442) | **voice** | Dedicated STT latency and TTS generation-speed benchmarks (`bench_voice_stt_tts.cpp`) |
| [#3444](https://github.com/makr-code/ThemisDB/pull/3444) | **voice** | Language detection and auto-locale switching: automatic BCP-47 locale resolution from audio stream characteristics |
| [#3445](https://github.com/makr-code/ThemisDB/pull/3445) | **rpc** | Replace all stub RPC service methods with real RocksDB-backed implementations (GET) |
| [#3446](https://github.com/makr-code/ThemisDB/pull/3446) | **rpc** | Unit tests for RPC GET operation with real RocksDB backend; CMake parse-error fixes |
| [#3447](https://github.com/makr-code/ThemisDB/pull/3447) | **rpc** | RPC PUT/INSERT operations with full transaction support |
| [#3448](https://github.com/makr-code/ThemisDB/pull/3448) | **rpc** | RPC DELETE with cascade logic and referential integrity enforcement |
| [#3449](https://github.com/makr-code/ThemisDB/pull/3449) | **rpc** | RPC Batch Read/Write operations; all remaining stubs removed |
| [#3450](https://github.com/makr-code/ThemisDB/pull/3450) | **rpc** | Integration tests covering all RPC paths (GET, PUT, DELETE, Batch) |
| [#3453](https://github.com/makr-code/ThemisDB/pull/3453) | **security** | Production safety guard for `TimestampAuthority` stub: hard-fails at startup if RFC 3161 real implementation is absent in production builds |
| [#3454](https://github.com/makr-code/ThemisDB/pull/3454) | **security** | Wire PKCS#11 HSM production path end-to-end: fix duplicate source file, harden stub gate, add HSM CI coverage |
| [#3456](https://github.com/makr-code/ThemisDB/pull/3456) | **security** | RFC 3161 TSA compliant API interface/wrapper design |
| [#3457](https://github.com/makr-code/ThemisDB/pull/3457) | **security** | RFC 3161 TSA request/response handling with full `TSAConfig` support (FreeTSA / DigiCert / Sectigo backends) |
| [#3458](https://github.com/makr-code/ThemisDB/pull/3458) | **security** | PKCS#11 C++ wrapper interface (`Pkcs11Provider`) with slot/session/key-object management |
| [#3461](https://github.com/makr-code/ThemisDB/pull/3461) | **security** | PKCS#11 token initialization and configuration (`Pkcs11Initializer`) |
| [#3462](https://github.com/makr-code/ThemisDB/pull/3462) | **security** | HSM production failsafe: hard rejection of stub HSM at runtime in `ENTERPRISE` and `HYPERSCALER` builds |
| [#3463](https://github.com/makr-code/ThemisDB/pull/3463) | **security / observability** | Audit log data-loss protection: `fsync` on write, log rotation with configurable size/age limits, secondary mirror path |
| [#3464](https://github.com/makr-code/ThemisDB/pull/3464) | **sharding** | Hardware migration support: stable `NodeIdentity` persistence across reboots/hardware replacements; safe endpoint replacement without data loss |

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

## ✨ Additional Features (Post-Initial Aggregation)

The following features were merged after the initial v1.5.0 feature set was defined. They are included in v1.5.0.

### Updates Module — History Log, Blue/Green Deployment, CoordinatedUpdateManager (PRs #3420, #3421, #3422, #3430, #3433)

> **Files:** `include/updates/update_history_log.h`, `include/updates/blue_green_manager.h`, `include/updates/coordinated_update_manager.h`

- `UpdateHistoryLog` (#3420): Persistent audit trail of all update operations — records operator identity, source/target version, timestamp, and outcome. Retained across restarts.
- Blue/Green Deployment (#3421, #3430): Zero-downtime major upgrades. Traffic is shifted to a parallel "green" environment while "blue" serves live traffic; automatic rollback on failure. Roadmap item `[x]` `blue-green deployment support` marked complete.
- `CoordinatedUpdateManager` (#3422): Multi-node replication-safe update sequencing using Raft-compatible consensus ordering. Ensures all replicas apply updates in the same order and prevents split-brain during rolling upgrades.
- Schema migration regression tests (#3433): `validateMigration` test suite added; Stubs:1 path in schema migration testing framework resolved.

### Voice Module — Language Detection, Meeting Transcription, Benchmarks (PRs #3434, #3439, #3442, #3444)

> **Files:** `include/voice/language_detector.h`, `include/voice/meeting_transcriber.h`, `benchmarks/bench_voice_stt_tts.cpp`

- Language Detection (#3444): Automatic BCP-47 locale resolution from audio stream characteristics; switches transcription model and tokenizer locale on-the-fly. Supports 40+ languages.
- Real-Time Meeting Transcription (#3434): Speaker-diarized transcription with action-item extraction using NLP rule engine. Emits structured `MeetingSession` records with participant turn boundaries and extracted tasks.
- Voice Unit-Test Coverage (#3439): Test coverage for voice module raised above 80% production threshold.
- STT/TTS Benchmarks (#3442): Dedicated Google Benchmark suite measuring STT latency (p50/p95/p99) and TTS generation speed across engine configurations.

### GPU Module — Multi-Node Cluster Coordination (PR #3425)

- Multi-node GPU cluster coordination declared **production-ready**: device management, topology discovery, and health-check probes hardened; memory-safety issues from pre-production phase resolved.

### Performance Module — Memory Pressure Monitor & PMU Cache-Miss Analysis (PRs #3426, #3435, #3437)

> **Files:** `include/performance/memory_pressure_monitor.h`, `include/performance/pmu_cache_miss_analyzer.h`

- Memory Pressure Monitor (#3426): Monitors process RSS and system-level free-memory headroom; triggers configurable callbacks (cache eviction, GC, reject-new-writes) at configurable pressure thresholds. Wired into build system with test suite.
- PMU Cache-Miss Analysis (#3435): CPU performance-counter instrumentation using `perf_event_open` (Linux) / VTune-compatible counters; exposes L1/L2/LLC miss rates per query execution. Wired into build and test pipeline.
- Cross-Module Performance Regression Detection in CI (#3437): Automated benchmark comparison stage; builds store result JSON under `benchmarks/baselines/`; CI job fails if any metric regresses beyond configurable tolerance.

### Query Module — Per-Query Resource Limits (PR #3427)

> **Files:** `include/query/query_resource_limiter.h`, `src/server/http_server.cpp`

- `QueryResourceLimiter` enforces per-request ceilings: `max_rows` (result set size), `max_memory_bytes` (working-set allocation), `timeout_ms` (wall-clock deadline).
- Limits configurable per-query via HTTP request header `X-Themis-Resource-Limits` (JSON) or globally via `config/core/config.yaml`.
- Returns HTTP 429 (resource limit exceeded) or 408 (timeout) with structured error body.

### Replication Module — CRDT FLAG Types (PR #3428)

> **Files:** `include/replication/crdt_flag.h`

- `FLAG_EW` (Enable-Wins Flag): concurrent enable and disable → enable wins; semantically correct for permission grants.
- `FLAG_DW` (Disable-Wins Flag): concurrent enable and disable → disable wins; semantically correct for feature kill-switches and circuit breakers.
- Both types integrate with existing `CRDTManager` merge/serialisation pipeline.

### Security Module — Full PKCS#11 HSM, RFC 3161 TSA, Audit Log Hardening (PRs #3453–#3463)

> **Files:** `include/security/pkcs11_provider.h`, `include/security/pkcs11_initializer.h`, `include/security/timestamp_authority.h`, `include/security/audit_log.h`

- **PKCS#11 C++ Wrapper** (#3458): `Pkcs11Provider` encapsulates slot enumeration, session lifecycle, key-object management, and CKM mechanism dispatch. Tested against SoftHSMv2 and Thales Luna.
- **PKCS#11 Token Initialization** (#3461): `Pkcs11Initializer` handles token provisioning, PIN management, and key-object lifecycle for first-time HSM setup.
- **HSM Production Failsafe** (#3462): `ENTERPRISE` and `HYPERSCALER` builds hard-fail at startup when stub HSM provider is detected; prevents accidental production deployments without hardware key storage.
- **PKCS#11 Production Path** (#3454): Duplicate source file removed; stub guard hardened; HSM CI coverage extended to include PKCS#11 signing/verification round-trips against SoftHSMv2.
- **RFC 3161 TSA Interface** (#3456): Compliant API interface for `TimestampAuthority`; abstracts FreeTSA, DigiCert, and Sectigo backends behind a uniform `requestTimestamp()` / `verifyTimestamp()` contract.
- **RFC 3161 TSA Implementation** (#3457): Full request/response serialisation and verification using OpenSSL ASN.1 primitives. `TSAConfig` selects backend, retry policy, and certificate pinning.
- **TSA Production Guard** (#3453): `TimestampAuthority` stub hard-fails at startup in production builds; development builds log a suppressible warning.
- **Audit Log Data-Loss Protection** (#3463): Writes are `fsync`-ed immediately; log rotation triggered at configurable size/age limits; secondary mirror path for dual-write redundancy. Addresses potential data loss under crash scenarios.

### Sharding Module — Hardware Migration Support (PR #3464)

> **Files:** `include/sharding/node_identity.h`, `src/sharding/node_identity.cpp`

- `NodeIdentity` persisted to stable storage (LMDB sidecar); survives reboots and hardware component replacements without triggering unnecessary shard rebalancing.
- Safe endpoint replacement API: `NodeIdentity::updateEndpoint()` atomically updates the network address and notifies the cluster without data movement.
- Cluster controller reconciliation: nodes with matching `NodeIdentity` but changed endpoint are recognised as the same logical node; no false `NODE_LOST` events.

### Chimera Module — CI Benchmark Baseline (PR #3424)

> **Files:** `tests/chimera/test_chimera_ci_benchmarks.cpp`, `benchmarks/baselines/chimera_baseline.json`

- `run_ci_benchmarks` integration-test target seeds a repeatable baseline under `benchmarks/baselines/chimera_baseline.json`.
- Benchmark results are committed and compared in CI to detect adapter-layer performance regressions.

### Security / Updates — HSM-Backed SigningService (PR #3438)

> **Files:** `include/security/hsm_signing_service.h`, `src/security/hsm_signing_service.cpp`

- `HsmSigningService` replaces the soft-key signing path in update bundle signing with PKCS#11 hardware-backed operations.
- Integrates with `Pkcs11Provider`; supports RSA-PSS and ECDSA signing mechanisms.
- Update bundles now carry a hardware-attested signature; rejected by the update verifier if signed with a soft key in production builds.

### RPC Layer — Full Production Implementation (PRs #3445–#3450)

> **Files:** `src/rpc/rpc_service.cpp`, `tests/rpc/`

- All stub RPC service methods replaced with real RocksDB-backed implementations:
  - **GET** (#3445, #3446): key lookup with optional consistency-level parameter.
  - **PUT / INSERT** (#3447): transactional write with conflict detection.
  - **DELETE** (#3448): cascade delete with referential integrity checks across column families.
  - **Batch Read/Write** (#3449): atomic multi-key operations with all-or-nothing semantics.
- Full integration-test coverage across all RPC paths (#3450).

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

### QA Status per PR Group

| PR(s) | Module | Tests | Docs | Security Review | Performance Baseline | QA Flag |
|-------|--------|-------|------|-----------------|----------------------|---------|
| #3049 | geo | ✅ | ✅ | ✅ | ✅ baselines/ | ✅ |
| #3050 | security | ✅ | ✅ | ✅ signed off | — | ✅ |
| #3051 | gpu | ✅ | ✅ | ✅ signed off | — | ✅ |
| #1383 | acceleration | ⚠️ PR open | ⚠️ partial | ⚠️ pending | — | ⚠️ **Needs QA sign-off** |
| #1384 | acceleration | ⚠️ PR open | ⚠️ partial | ⚠️ pending | — | ⚠️ **Needs QA sign-off** |
| #1390 | acceleration | ⚠️ PR open | ✅ | ⚠️ pending | — | ⚠️ **Needs QA sign-off** |
| #3420 | updates | ✅ | ✅ | — | — | ✅ |
| #3421, #3430 | updates | ✅ | ✅ | — | — | ✅ |
| #3422 | replication/updates | ✅ | ✅ | — | — | ✅ |
| #3424 | chimera | ✅ | ✅ | — | ✅ baseline seeded | ✅ |
| #3425 | gpu | ✅ | ✅ | ✅ | — | ✅ |
| #3426 | performance | ✅ | ✅ | — | — | ✅ |
| #3427 | query | ✅ | ✅ | — | — | ✅ |
| #3428 | replication | ✅ | ✅ | — | — | ✅ |
| #3434, #3439 | voice | ✅ >80% | ✅ | ⚠️ audio security audit WIP (#3443) | — | ⚠️ **Audio security audit pending** |
| #3435, #3437 | performance | ✅ | ✅ | — | ✅ | ✅ |
| #3438 | security/updates | ✅ | ✅ | ✅ | — | ✅ |
| #3442 | voice | ✅ | ✅ | — | ✅ STT/TTS baselines | ✅ |
| #3444 | voice | ✅ | ✅ | — | — | ✅ |
| #3445–#3450 | rpc | ✅ integration | ✅ | — | — | ✅ |
| #3453–#3462 | security (HSM/TSA) | ✅ | ✅ | ✅ | — | ✅ |
| #3463 | security/observability | ✅ | ✅ | ✅ | — | ✅ |
| #3464 | sharding | ✅ | ✅ | — | — | ✅ |

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
| RFC 3161 Timestamp Authority | [x] | #3453, #3456, #3457 |
| FAISS Quantizer Integration | [x] | v1.5.0 |
| Update history log (who/when/from/to version) | [x] | #3420 |
| Blue/green deployment support for zero-downtime upgrades | [x] | #3421, #3430 |
| Multi-node replication-safe update sequencing | [x] | #3422 |
| Multi-node GPU cluster coordination production-ready | [x] | #3425 |
| Memory pressure monitor (Phase 3) | [x] | #3426 |
| Per-query resource limits (max_rows, max_memory_bytes, timeout_ms) | [x] | #3427 |
| CRDT FLAG_EW and FLAG_DW types | [x] | #3428 |
| Real-time meeting transcription with action-item extraction | [x] | #3434 |
| PMU cache-miss analysis and cross-module regression CI | [x] | #3435, #3437 |
| HSM-backed SigningService for update bundles | [x] | #3438 |
| Voice language detection and auto-locale switching | [x] | #3444 |
| RPC layer: all stubs replaced with real database-backed implementations | [x] | #3445–#3450 |
| PKCS#11 HSM production path (wrapper, init, failsafe, CI) | [x] | #3454, #3458, #3461, #3462 |
| Audit log data-loss protection (fsync, rotation, mirror) | [x] | #3463 |
| Sharding hardware migration support / NodeIdentity persistence | [x] | #3464 |
| Chimera CI benchmark baseline | [x] | #3424 |

---

## 🚩 PRs / Features Requiring Further Clarification or QA

The following items are included in the v1.5.0 PR aggregation but require additional action before the final release sign-off:

| Item | Issue | Action Required |
|------|-------|-----------------|
| #1383 CUDA ANN + geospatial kernels | PR still open | Merge and run full GPU regression suite; confirm acceptance tests pass |
| #1384 Vulkan compute shader pipeline | PR still open | Merge; shader integrity verification (#1384 roadmap item) to be completed |
| #1390 Cross-backend L2 distance validation | PR still open | Merge; confirm numeric parity test baseline committed |
| Voice audio security audit | #3443 WIP | Complete audio pipeline security audit before final release |
| Voice SIP/WebRTC integration | #3431 WIP | Descoped from v1.5.0 unless completed; target v1.6.0 |
| Build modularisation | #3429 WIP | Descoped from v1.5.0 unless completed; target v1.6.0 |
| Chimera adapter capabilities matrix | #3436 WIP | Documentation-only; low risk; can follow in patch |
| Pre-flight health checks update | #3441 WIP | Confirm coverage; no blocking dependency on release |

---

## 🔗 Related Documentation

- [RELEASE_TARGET_v1.5.0.md](./RELEASE_TARGET_v1.5.0.md) — Pre-release target definition: deliverables, scope, completion criteria, QA/performance requirements, and out-of-scope items
- [CHANGELOG.md](../../../CHANGELOG.md) — Full change log
- [roadmap.md](../../../roadmap.md) — Top-level project roadmap
- [src/geo/ROADMAP.md](../../../src/geo/ROADMAP.md) — Geo module roadmap
- [src/gpu/ROADMAP.md](../../../src/gpu/ROADMAP.md) — GPU module roadmap
- [src/security/ROADMAP.md](../../../src/security/ROADMAP.md) — Security module roadmap
- [docs/FUTURE_GPU_SUPPORT.md](../../FUTURE_GPU_SUPPORT.md) — GPU support v2.x roadmap
- [docs/GPU_MASTER_TRACKING.md](../../GPU_MASTER_TRACKING.md) — GPU implementation tracking

---

*This document was produced as part of the v1.5.0 Release Aggregation (Issue: makr-code/ThemisDB#3070) and serves as a template for future release cycles. Last updated: 2026-03-03.*
